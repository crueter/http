// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

module;

#include <cstring>
#include <expected>
#include <format>
#include <memory>
#include <string>
#include <utility>
#include <charconv>

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define closesocket(s) close(s)

#elif defined(WIN32_LEAN_AND_MEAN) || defined(_WIN32) || defined(WIN32)

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#else
#error "unsupported platform"
#endif

// SSL stuff
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>

export module hydrocarbon:socket;
export import :fields;

namespace hydro
{
// shared ssl context
class SSLContext {
private:
    SSL_CTX *ctx;

    SSLContext() = default;

public:
    SSLContext(const SSLContext &) = delete;
    SSLContext &operator=(const SSLContext &) = delete;

    ~SSLContext()
    {
        if (ctx) {
            SSL_CTX_free(ctx);
            ctx = nullptr;
        }
    }

    bool init()
    {
        if (!ctx) {
            ctx = SSL_CTX_new(TLS_client_method());
        }

        return ctx != nullptr;
    }

    SSL_CTX *get()
    {
        init();
        return ctx;
    }

    static SSLContext &ssl_context()
    {
        static SSLContext instance;
        return instance;
    }
};

void free_ssl(SSL *s)
{
    if (s) {
        SSL_shutdown(s);
        SSL_free(s);
    }
}

}; // namespace hydro

using RAII_SSL = std::unique_ptr<SSL, decltype(&hydro::free_ssl)>;

export namespace hydro
{

// headers bigger than this are concerning
inline constexpr auto maxHeaderSize = 16378;

// most reads
inline constexpr auto defaultReadSize = 4096;

// headers, readLine
inline constexpr auto shortReadSize = 1024;

// raii fd wrapper
class FileDescriptor {
protected:
    int m_fd = -1;

public:
    FileDescriptor() = default;
    explicit FileDescriptor(int fd) : m_fd(fd) {}
    virtual ~FileDescriptor()
    {
        if (m_fd >= 0) {
            closesocket(m_fd);
        }
    }

    FileDescriptor(const FileDescriptor &) = delete;
    FileDescriptor &operator=(const FileDescriptor &) = delete;

    FileDescriptor(FileDescriptor &&other) noexcept : m_fd(std::exchange(other.m_fd, -1)) {}

    FileDescriptor &operator=(FileDescriptor &&other) noexcept
    {
        if (this != &other) {
            if (m_fd >= 0)
                closesocket(m_fd);

            m_fd = std::exchange(other.m_fd, -1);
        }
        return *this;
    }

    FileDescriptor &operator=(int fd)
    {
        if (m_fd >= 0)
            closesocket(m_fd);

        m_fd = fd;
        return *this;
    }

    operator int() const
    {
        return m_fd;
    }

    [[nodiscard]] int release()
    {
        int temp = m_fd;
        m_fd = -1;
        return temp;
    }

    [[nodiscard]] bool isValid() const
    {
        return m_fd >= 0;
    }
};

class Socket : public FileDescriptor {
    SSL *m_ssl = nullptr;
    bool m_connected = false;

    // SSL or insecure recv.
    int64_t internal_recv(char *buf, std::size_t n)
    {
        if (!m_ssl)
            return ::recv(m_fd, buf, n, 0);

        while (true) {
            auto r = SSL_read(m_ssl, buf, n);
            if (r >= 0)
                return r;

            // ssl error
            if (!BIO_should_retry(SSL_get_rbio(m_ssl))) {
                return -1;
            }

            // otherwise keep retrying
        }
    }

    // SSL or insecure send.
    int64_t internal_send(char *buf, std::size_t n)
    {
        if (!m_ssl)
            return ::send(m_fd, buf, n, 0);

        std::size_t total = 0;
        while (total < n) {
            auto w = SSL_write(m_ssl, buf, n);

            if (w > 0)
                total += w;

            // ssl error
            else if (!BIO_should_retry(SSL_get_wbio(m_ssl))) {
                break;
            }
        }

        return total;
    }

public:
    // default constructor
    Socket() = default;

    ~Socket()
    {
        free_ssl(m_ssl);
    }

    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;

    Socket(Socket &&other) noexcept
        : FileDescriptor(std::move(other)), m_ssl(std::exchange(other.m_ssl, nullptr)),
          m_connected(std::exchange(other.m_connected, false))
    {
    }

    // Whether or not this socket has made a connection yet.
    [[nodiscard]] bool connected() const
    {
        return m_connected;
    }

    // Connect to a URL and return the connected socket.
    [[nodiscard]] std::expected<void, Error> connect(const hydro::URL &url,
                                                     const ConnectOptions &opts = {})
    {
        struct addrinfo hints = {};

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        struct addrinfo *hostinfo = nullptr;
        int err = getaddrinfo(url.hostname.c_str(), url.port_str.c_str(), &hints, &hostinfo);
        if (err != 0) {
            if (hostinfo)
                freeaddrinfo(hostinfo);

            return std::unexpected(Error::DNSFailure);
        }

        FileDescriptor fd;

        for (auto conn = hostinfo; conn != nullptr; conn = conn->ai_next) {
            int t_fd = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
            if (t_fd == -1)
                continue;

            if (::connect(t_fd, conn->ai_addr, conn->ai_addrlen) != -1) {
                fd = t_fd;
                break;
            }

            closesocket(t_fd);
        }

        if (!fd.isValid()) {
            freeaddrinfo(hostinfo);
            return std::unexpected(Error::SocketCreate);
        }

        freeaddrinfo(hostinfo);

        // tls
        RAII_SSL ssl{nullptr, &free_ssl};
        if (url.protocol == Protocol::HTTPS) {
            if (opts.disableSsl) {
                return std::unexpected(Error::ConnectFailed);
            }

            // setup
            auto ctx = SSLContext::ssl_context().get();
            if (!ctx) {
                return std::unexpected(Error::TLSCtx);
            }

            ssl.reset(SSL_new(ctx));
            if (!ssl) {
                return std::unexpected(Error::TLSInit);
            }

            // TODO: test
            if (!opts.cert.empty()) {
                // read cert
                BIO *bio = BIO_new_mem_buf(opts.cert.data(), opts.cert.size());
                if (!bio) {
                    return std::unexpected(Error::InvalidCert);
                }

                X509 *cert = PEM_read_bio_X509(bio, nullptr, nullptr, nullptr);
                if (!cert) {
                    return std::unexpected(Error::InvalidCert);
                }

                SSL_use_certificate(ssl.get(), cert);

                X509_free(cert);
                BIO_free(bio);
            } else {
                SSL_CTX_set_default_verify_paths(ctx);
            }

            SSL_set_fd(ssl.get(), fd);
            SSL_set_tlsext_host_name(ssl.get(), url.hostname.c_str());
            SSL_set_verify(ssl.get(), SSL_VERIFY_PEER, nullptr);

            if (SSL_connect(ssl.get()) <= 0) {
                return std::unexpected(Error::ConnectFailed);
            }

            if (opts.verify) {
                auto cert = SSL_get_peer_certificate(ssl.get());
                if (!cert) {
                    return std::unexpected(Error::TLSCert);
                }

                X509_free(cert);

                auto verify_res = SSL_get_verify_result(ssl.get());
                if (verify_res != X509_V_OK) {
                    return std::unexpected(Error::TLSVerify);
                }
            }
        }

        m_fd = fd.release();
        m_ssl = ssl.release();
        m_connected = true;

        return {};
    }

    // Send a formatted string to the socket.
    template <typename... Args>
    std::expected<std::size_t, Error> send(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string formatted = std::format(fmt, std::forward<Args>(args)...);
        std::size_t bytes_left = formatted.size();
        std::size_t bytes_sent = 0;
        std::size_t offset = 0;

        while (bytes_left > 0) {
            int64_t sent = internal_send(formatted.data() + offset, bytes_left);
            if (sent == 0)
                break;
            if (sent < 0)
                return std::unexpected(Error::SendFailed);

            bytes_left -= sent;
            offset += sent;
            bytes_sent += sent;
        }

        return bytes_sent;
    }

    // Receive into a buffer.
    int64_t recv(std::string &buffer, std::size_t bytes = 0)
    {
        const std::size_t max_bytes =
            bytes > 0 ? bytes : (buffer.size() > 0 ? buffer.size() - 1 : defaultReadSize);
        buffer.resize(max_bytes);

        int64_t size = internal_recv(buffer.data(), max_bytes);
        if (size > 0) {
            buffer.resize(size);
        } else {
            buffer.clear();
        }

        return size;
    }

    // Receive a single character.
    int64_t recv(char &buffer)
    {
        auto size = internal_recv(&buffer, 1);
        return size;
    }

    // Sized transfer.
    void readSized(Response &r, const UserCallback &cb)
    {
        // account for ending byte
        std::size_t len = r.contentLength();
        std::size_t read = r.body.size();
        cb({read, len - read, r.body.substr(0, read), len, r.status_code});

        std::string buf;

        int64_t size;
        while (read < len && (size = recv(buf, defaultReadSize)) > 0) {
            read += size;
            auto res = cb({read, len - read, buf, len, r.status_code});
            if (!res) {
                break;
            }
        }
    }

    // Chunked transfer.
    std::expected<void, Error> readChunked(Response &r, const UserCallback &cb)
    {
        // length is unknown
        std::size_t read = r.body.size();
        std::string buf = r.body;
        bool finished = false;

        // read a full line until \r\n, recv'ing if needed
        const auto readLine = [this, &buf]() -> std::string {
            while (true) {
                auto pos = buf.find("\r\n");

                // grab and return line, remove from buf
                if (pos != std::string::npos) {
                    auto line = buf.substr(0, pos);
                    buf.erase(0, pos + 2);
                    return line;
                }

                // need more socket data
                std::string tmp;
                if (recv(tmp, shortReadSize) <= 0)
                    break;

                buf += tmp;
            }

            return "";
        };

        while (!finished) {
            // read hex size
            auto size_str = readLine();

            if (size_str.empty())
                break;

            // strip chunk exts
            auto semi_pos = size_str.find(';');
            if (semi_pos != std::string::npos) {
                size_str = size_str.substr(0, semi_pos);
            }

            // extract chunk size as a number
            std::size_t size = 0;
            auto [ptr, ec] =
                std::from_chars(size_str.data(), size_str.data() + size_str.size(), size, 16);

            // malformed response
            if (ec != std::errc{}) {
                return std::unexpected(Error::InvalidChunk);
            }

            // end of data, aka zero size chunk
            if (size == 0) {
                while (!readLine().empty()) {
                }
                finished = true;
                break;
            }

            // now read (+2 for crlf)
            std::size_t target = size + 2;
            while (buf.size() < target) {
                std::string tmp;

                // recv needed bytes and append
                if (recv(tmp, target - buf.size()) <= 0) {
                    finished = true;
                    break;
                }

                buf += tmp;
            }

            // connection dropped mid chunk
            if (buf.size() < target) {
                return std::unexpected(Error::ReadFailed);
            }

            // get actual chunk data
            std::string data = buf.substr(0, size);
            buf.erase(0, target);

            read += size;

            // we dunno total length, so ignore those fields
            auto res = cb({read, 0, data, 0, r.status_code});
            if (!res)
                break;
        }

        return {};
    }

    // Get all response headers.
    [[nodiscard]] std::expected<Response, Error> readHeaders()
    {
        std::string data;
        std::string buf;
        buf.resize(shortReadSize);

        // Read until header block is complete.
        while (recv(buf) > 0) {
            data.append(buf);
            if (data.find("\r\n\r\n") != std::string_view::npos)
                break;

            // headers are gigantic, leave now before bad things happen.
            if (data.size() > maxHeaderSize)
                return std::unexpected(Error::HeaderTooBig);
        }

        // remaining body bytes will be left in the body field of the response
        return Response{data};
    }

    // Read the headers and body in one shot.
    std::expected<Response, Error> readAll()
    {
        std::string body;
        auto cb = [&body](const hydro::DownloadState &state) {
            body += state.buffer;
            return true;
        };

        auto ec = readCallback(cb);
        if (!ec)
            return ec;

        auto r = ec.value();
        r.body = body;
        return r;
    }

    // Read with a callback.
    std::expected<Response, Error> readCallback(const UserCallback &cb)
    {
        auto ec = readHeaders();
        if (!ec)
            return ec;

        auto r = ec.value();

        switch (r.transferMode) {
        case TransferMode::Sized:
            readSized(r, cb);
            break;
        case TransferMode::Chunked: {
            auto r_ec = readChunked(r, cb);
            if (!r_ec)
                return std::unexpected(r_ec.error());
            break;
        }
        case TransferMode::NoData:
            break;
        }

        return r;
    }
};

}; // namespace hydro

// winsock initializer
// adapted from cpp-httplib
#ifdef _WIN32
class WSInit {
public:
    WSInit()
    {
        WSADATA wsaData;
        m_isValid = (WSAStartup(0x0002, &wsaData) == 0);
    }

    ~WSInit()
    {
        if (m_isValid)
            WSACleanup();
    }

    bool m_isValid = false;
};

static WSInit wsinit_;
#endif

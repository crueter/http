module;

#include <cstring>
#include <format>
#include <print>
#include <string>

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define closesocket(s) close(s)

#elif defined(WIN32_LEAN_AND_MEAN) || defined(_WIN32) || defined(WIN32)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#else
#error "unsupported platform"
#endif

// SSL stuff
#ifndef REQUESTS_NO_TLS

#include <openssl/ssl.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <openssl/crypto.h>
#include <openssl/tls1.h>

#endif // #ifndef REQUESTS_NO_TLS

export module http:socket;
export import :fields;


export namespace http
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
            SSL_CTX_set_default_verify_paths(ctx);
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

class Socket {
    int m_fd = -1;

    SSL *m_ssl;

    ssize_t internal_recv(char *buf, std::size_t n) {
        if (m_ssl) {
            return SSL_read(m_ssl, buf, n);
        }

        return ::recv(m_fd, buf, n, 0);
    }

    ssize_t internal_send(char *buf, std::size_t n) {
        if (m_ssl) {
            return SSL_write(m_ssl, buf, n);
        }

        return ::send(m_fd, buf, n, 0);
    }

public:
    explicit Socket(int fd) : m_fd(fd) {}

    ~Socket()
    {
        if (m_fd >= 0)
            closesocket(m_fd);

        if (m_ssl) {
            SSL_shutdown(m_ssl);
            SSL_free(m_ssl);
        }
    }

    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;

    // Invalidate the other socket.
    Socket(Socket &&other) noexcept : m_fd(other.m_fd)
    {
        other.m_fd = -1;
    }

    // Close the current socket, steal the other's fd and invalidate.
    Socket &operator=(Socket &&other) noexcept
    {
        if (this != &other) {
            if (m_fd >= 0)
                closesocket(m_fd);

            m_fd = other.m_fd;
            other.m_fd = -1;
        }
        return *this;
    }

    [[nodiscard]] int get() const
    {
        return m_fd;
    }

    [[nodiscard]] bool isValid() const
    {
        return m_fd >= 0;
    }

    // Connect to a URL and return the connected socket.
    Socket(const http::URL &url)
    {
        struct addrinfo hints = {};

        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        struct addrinfo *hostinfo = nullptr;
        int err = getaddrinfo(url.hostname.c_str(), url.port_str.c_str(), &hints, &hostinfo);
        if (err != 0) {
            return;
        }

        int fd = -1;
        for (auto conn = hostinfo; conn != nullptr; conn = conn->ai_next) {
            fd = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
            if (fd == -1)
                continue;

            if (::connect(fd, conn->ai_addr, conn->ai_addrlen) != -1)
                break;

            closesocket(fd);
            fd = -1;
        }

        freeaddrinfo(hostinfo);
        m_fd = fd;

        // tls
        if (url.protocol == Protocol::HTTPS) {
            m_ssl = SSL_new(SSLContext::ssl_context().get());
            if (!m_ssl) {
                std::println(stderr, "Failed to init ssl");
                // TODO: better error logging
                return;
            }
            // TODO: custom cert

            SSL_set_fd(m_ssl, m_fd);
            SSL_set_tlsext_host_name(m_ssl, url.hostname.c_str());

            if (SSL_connect(m_ssl)) {
                SSL_set_verify(m_ssl, SSL_VERIFY_PEER, nullptr);
                auto cert = SSL_get_peer_certificate(m_ssl);
                if (!cert) {
                    std::println(stderr, "Failed to get cert");
                    // TODO: better error logging
                    return;
                }
                X509_free(cert);

                auto verify_res = SSL_get_verify_result(m_ssl);
                if (verify_res != X509_V_OK) {
                    std::println(stderr, "Failed to verify res, err={}", verify_res);
                    // TODO: better error logging
                    return;
                }
            }
        }
    }

    // Send a formatted string to the socket.
    template <typename... Args>
    void send(std::format_string<Args...> fmt, Args &&...args)
    {
        std::string formatted = std::format(fmt, std::forward<Args>(args)...);
        size_t bytes_left = formatted.size();
        size_t bytes_sent = 0;
        size_t offset = 0;

        while (bytes_left > 0) {
            ssize_t sent = internal_send(formatted.data() + offset, bytes_left);
            if (sent <= 0)
                break;

            bytes_left -= sent;
            offset += sent;
        }
    }

    // Receive into a buffer.
    ssize_t recv(std::string &buffer, std::size_t bytes = 0) {
        const std::size_t max_bytes = bytes > 0 ? bytes: (buffer.size() > 0 ? buffer.size() - 1 : 4096);
        buffer.resize(max_bytes);

        ssize_t size = internal_recv(buffer.data(), max_bytes);
        if (size > 0) {
            buffer.resize(size);
        } else {
            buffer.clear();
        }

        return size;
    }

    // Receive a single character.
    ssize_t recv(char &buffer) {
        auto size = internal_recv(&buffer, 1);
        return size;
    }

    // Get all response headers.
    Response readHeaders() {
        std::string data;
        std::string buf;
        buf.resize(1024);

        // Read until header block is complete.
        while (recv(buf) > 0) {
            data.append(buf);
            if (data.find("\r\n\r\n") != std::string_view::npos)
                break;

            // headers are gigantic, leave now before bad things happen.
            if (data.size() > 16378)
                break;
        }

        // remaining body bytes will be left in the body field of the response
        return Response{data};
    }

    // Read the headers and body in one shot.
    Response readAll() {
        std::string body;
        auto cb = [&body](const http::DownloadState &state) {
            body += state.buffer;
            return true;
        };

        auto r = readCallback(cb);

        r.body = body;

        return r;
    }

    // Sized transfer.
    void readSized(Response &r, const UserCallback &cb) {
        // account for ending byte
        std::size_t len = r.contentLength();
        std::size_t read = r.body.size();
        cb({read, len - read, r.body.substr(0, read), len, r.status_code});

        std::string buf;

        ssize_t size;
        while (read < len && (size = recv(buf, 4096)) > 0) {
            read += size;
            auto res = cb({read, len - read, buf, len, r.status_code});
            if (!res)
                break;
        }
    }

    // Chunked transfer.
    void readChunked(Response &r, const UserCallback &cb) {
        // length is unknown
        std::size_t read = r.body.size();
        std::string buf = r.body;
        bool finished = false;

        std::println("Body size {}", read);

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
                if (recv(tmp, 1024) <= 0)
                    break;
                buf += tmp;
            }

            return "";
        };

        while (!finished) {
            // read hex size
            auto size_str = readLine();

            if (size_str.empty()) break;

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
                break;
            }

            // end of data, aka zero size chunk
            if (size == 0) {
                while (!readLine().empty()) {}
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
            if (buf.size() < target)
                break;

            // get actual chunk data
            std::string data = buf.substr(0, size);
            buf.erase(0, target);

            read += size;

            // we dunno total length, so ignore those fields
            auto res = cb({read, 0, data, 0, r.status_code});
            if (!res)
                break;

        }
    }

    // Read with a callback.
    Response readCallback(const UserCallback &cb) {
        Response r = readHeaders();

        std::println("Transfer mode: {}", (int) r.transferMode);

        switch (r.transferMode) {
        case TransferMode::Sized:
            readSized(r, cb);
            break;
        case TransferMode::Chunked:
            readChunked(r, cb);
            break;
        case TransferMode::NoData:
            break;
        }

        return r;
    }
};

}; // namespace http
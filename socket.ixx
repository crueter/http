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

#define closesocket(s) close(s);

#elif defined(WIN32_LEAN_AND_MEAN) || defined(_WIN32) || defined(WIN32)

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#else
#error "unsupported platform"
#endif

export module http:socket;
export import :fields;

export namespace http
{

class Socket {
    int m_fd = -1;

public:
    explicit Socket(int fd) : m_fd(fd) {}

    ~Socket()
    {
        if (m_fd >= 0)
            closesocket(m_fd);
    }

    Socket(const Socket &) = delete;
    Socket &operator=(const Socket &) = delete;

    Socket(Socket &&other) noexcept : m_fd(other.m_fd)
    {
        other.m_fd = -1;
    }

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
            ssize_t sent = ::send(m_fd, formatted.data() + offset, bytes_left, 0);
            if (sent <= 0)
                break;

            bytes_left -= sent;
            offset += sent;
        }
    }

    // Receive into a buffer.
    ssize_t recv(std::string &buffer, std::size_t bytes = 0) {
        auto size = ::recv(m_fd, buffer.data(), bytes > 0 ? bytes : (buffer.size() - 1), 0);
        return size;
    }

    // Receive a single character.
    ssize_t recv(char &buffer) {
        auto size = ::recv(m_fd, &buffer, 1, 0);
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
        // TODO: Chunked.
        Response r = readHeaders();

        std::size_t len = r.contentLength();

        std::string buf;
        buf.resize(4096);

        ssize_t size;
        while (r.body.size() < len && (size = recv(buf)) > 0) {
            r.body.append(buf.data(), size);
        }

        return r;
    }

    // Read with a callback.
    Response readCallback(const UserCallback &cb) {
        // TODO: Chunked.
        Response r = readHeaders();

        // account for ending byte
        std::size_t len = r.contentLength() + 1;
        std::size_t read = r.body.size();
        cb({read, len - read, r.body, len, r.status_code});

        std::string buf;
        buf.resize(4096);

        ssize_t size;
        while (read < len && (size = recv(buf)) > 0) {
            read += size;
            cb({read, len - read, buf, len, r.status_code});
        }

        return r;
    }
};

}; // namespace http
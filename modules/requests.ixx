module;

#include <print>
#include <string>
#include <expected>
#include <format>
#include <string_view>

export module http;
export import :socket;

export namespace http
{

struct RequestOptions {
    Version version = Version::HTTP_1_1;
    Header headers;
    std::string body;
    UserCallback callback;
    ConnectOptions connect;
};

std::expected<Response, Error> perform(std::string_view method, const URL &url, const RequestOptions &opts = {}) {
    Socket s;

    // connect
    {
        auto ec = s.connect(url);
        if (!ec)
            return std::unexpected(ec.error());
    }

    auto headers = opts.headers;

    // construct data to send
    const auto version = opts.version == Version::HTTP_1_0 ? "HTTP/1.0" : "HTTP/1.1";
    const auto line1 = std::format("{} {} {}", method, url.route, version);

    // default ua/host
    if (!headers.contains("user-agent")) {
        headers.add("user-agent", "Hydrocarbon/0.0.1");
    }

    if (!headers.contains("host")) {
        headers.add("host", url.hostname);
    }

    // add headers
    std::string headerStr;
    for (const auto &[k, v] : headers.entries()) {
        headerStr += std::format("\r\n{}: {}", k, v);
    }

    const auto finalLine = "\r\nConnection: close\r\n\r\n";

    // send off request
    {
        auto ec = s.send("{}{}{}", line1, headerStr, finalLine);
        if (!ec)
            return std::unexpected(ec.error());
    }

    // now read
    if (opts.callback) {
        return s.readCallback(opts.callback);
    } else {
        return s.readAll();
    }
}

} // namespace http
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
        auto ec = s.connect(url, opts.connect);
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

    if (!opts.body.empty() && !headers.contains("content-length")) {
        headers.add("content-length", std::to_string(opts.body.size()));
    }

    // add headers
    std::string headerStr;
    for (const auto &[k, v] : headers.entries()) {
        headerStr += std::format("\r\n{}: {}", k, v);
    }

    std::string finalLine = "\r\nConnection: close\r\n\r\n";

    // send off request
    {
        auto ec = s.send("{}{}{}", line1, headerStr, finalLine);
        if (!ec)
            return std::unexpected(ec.error());
    }

    // send body if present
    if (!opts.body.empty()) {
        auto ec = s.send("{}", opts.body);
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

std::expected<Response, Error> get(const URL &url, const RequestOptions &opts = {}) {
    return perform("GET", url, opts);
}

std::expected<Response, Error> head(const URL &url, const RequestOptions &opts = {}) {
    return perform("HEAD", url, opts);
}

std::expected<Response, Error> post(const URL &url, const RequestOptions &opts = {}) {
    return perform("POST", url, opts);
}

std::expected<Response, Error> put(const URL &url, const RequestOptions &opts = {}) {
    return perform("PUT", url, opts);
}

std::expected<Response, Error> patch(const URL &url, const RequestOptions &opts = {}) {
    return perform("PATCH", url, opts);
}

std::expected<Response, Error> options(const URL &url, const RequestOptions &opts = {}) {
    return perform("OPTIONS", url, opts);
}

// annoying!
std::expected<Response, Error> perform_delete(const URL &url, const RequestOptions &opts = {}) {
    return perform("DELETE", url, opts);
}

} // namespace http
#include <print>

import hydro;

int main()
{
    // straight-body test
    {
        auto ec = hydro::get("https://www.google.com/");
        if (!ec) {
            std::println("Error: {}", ec.error());
        } else {
            auto r = ec.value();
            std::println("Request succeeded. Body size={}", r.body.size());
        }
    }

    // callback test
    {
        const auto func = [](const hydro::DownloadState &state) {
            std::println("Received {}/{} bytes ({} remaining)", state.bytes_received,
                         state.content_length, state.bytes_left);

            return true;
        };

        auto ec = hydro::get("https://www.google.com/", {.callback = func});
        if (!ec) {
            std::println("Error: {}", ec.error());
        } else {
            auto r = ec.value();
            std::println("Callback request succeeded.", r.body.size());
        }
    }

    // HEAD
    {
        auto ec = hydro::head("https://www.google.com/");
        if (!ec) {
            std::println("Error: {}", ec.error());
        } else {
            auto r = ec.value();
            std::println("HEAD request succeeded. Body size={}", r.body.size());
        }
    }

    // POST
    {
        hydro::RequestOptions opts;
        opts.body = "POST Request body, should be returned verbatim.";

        auto ec = hydro::post("https://httpbin.org/post", opts);
        if (!ec) {
            std::println("Error: {}", ec.error());
        } else {
            auto r = ec.value();
            std::println("POST request status={} body size={}", r.status_line, r.body.size());
        }
    }

    // PUT
    {
        hydro::RequestOptions opts;
        opts.body = "PUT Request body, should be returned verbatim.";
        opts.headers.add("X-Random-Header", "RandomValue");

        auto ec = hydro::put("https://httpbin.org/put", opts);
        if (!ec) {
            std::println("Error: {}", ec.error());
        } else {
            auto r = ec.value();
            std::println("PUT request status={} body size={}", r.status_line, r.body.size());
            std::println("Body: {}", r.body);
        }
    }

    // OPTIONS
    {
        auto ec = hydro::options("https://httpbin.org/options");
        if (!ec) {
            std::println("Error: {}", ec.error());
        } else {
            auto r = ec.value();
            std::println("OPTIONS request status={} body size={}", r.status_line, r.body.size());
            std::println("OPTIONS body: {}", r.body);
            std::println("OPTIONS returned headers:");

            for (const auto &[k, v] : r.header.entries()) {
                std::println("  {}: {}", k, v);
            }
        }
    }

    // redirect
    {
        // TODO: URL encoding
        auto ec = hydro::get("https://httpbin.org/redirect-to?url=http://httpbin.org");

        if (!ec) {
            std::println("Error: {}", ec.error());
        } else {
            auto r = ec.value();
            std::println("Request succeeded. Body size={}", r.body.size());
        }
    }
}
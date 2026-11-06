#include <print>

import hydro;

void printUrl(const hydro::URL &u)
{
    std::println("Hostname: {}", u.hostname);
    std::println("Port: {}", u.port);
    std::println("Route: {}", u.route);
    std::println("Params: {}", u.params.str);

    for (const auto &[k, v] : u.params.params) {
        std::println("  {}: {}", k, v);
    }

    std::println("Proto: {}", int(u.protocol));
}

int main()
{
    // auto r = requests_get("https://webhook.crueter.xyz/forgejo", NULL);
    // if (r) {
    //     std::println("Payload:\n{}", r->body.data);
    // }

    hydro::URL u = "https://webhook.crueter.xyz:8080/forgejo?param1=123&param2=hi";
    std::println("URL: {}", u);
    // printUrl(u);

    // // oneshot read
    // {
    //     hydro::URL example = "http://httpbin.org";
    //     hydro::Socket s(example);

    //     if (s.isValid()) {
    //         std::println("Connect to http://httpbin.org succeeded");
    //     } else {
    //         std::println("Connect to http://httpbin.org failed");
    //         return 1;
    //     }

    //     s.send("GET {} HTTP/1.1\r\n", example.route);
    //     s.send("Host: {}\r\n", example.hostname);
    //     s.send("User-Agent: curl/8.22.0\r\n");
    //     s.send("Connection: close\r\n\r\n");

    //     auto r = s.readAll();

    //     std::println("Status: {} {}", int(r.status_code), r.reason);
    //     std::println("Headers:");

    //     for (const auto &[k, v] : r.header.entries()) {
    //         std::println("  {} = {}", k, v);
    //     }

    //     // std::println("Body:");
    //     // std::println("{}", r.body);

    //     std::println("Body size: {}", r.body.size());
    // }

    // callback read
    // {
    //     hydro::URL example = "https://www.google.com";
    //     hydro::Socket s;
    //     {
    //         auto ec = s.connect(example);
    //         if (!ec) {
    //             std::println(stderr, "Error during connect: {}", ec.error());
    //             std::exit(1);
    //         }
    //     }

    //     {
    //         auto ec = s.send("GET {} HTTP/1.1\r\nHost: {}\r\nUser-Agent: curl/8.22.0\r\nConnection: close\r\n\r\n", example.route, example.hostname);
    //         if (!ec) {
    //             std::println("Error during send: {}", ec.error());
    //             std::exit(1);
    //         }
    //     }

    //     std::string body;

    //     {
    //         auto ec = s.readCallback(func);
    //         if (!ec) {
    //             std::println("Error during read: {}", ec.error());
    //             std::exit(1);
    //         }

    //         const auto &r = ec.value();

    //         std::println("Status: {} {}", int(r.status_code), r.reason);
    //         std::println("Headers:");

    //         for (const auto &[k, v] : r.header.entries()) {
    //             std::println("  {} = {}", k, v);
    //         }
    //     }

    //     // std::println("Body:");
    //     // std::println("{}", r.body);

    //     // std::println("Body size: {}", r.body.size());
    // }

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

            // std::println("Body fragment:\n{}", state.buffer);

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
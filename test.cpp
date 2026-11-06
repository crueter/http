#include <print>

import http;

void printUrl(const http::URL &u)
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

    http::URL u = "https://webhook.crueter.xyz:8080/forgejo?param1=123&param2=hi";
    std::println("URL: {}", u);
    // printUrl(u);

    // // oneshot read
    // {
    //     http::URL example = "http://httpbin.org";
    //     http::Socket s(example);

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
    {
        http::URL example = "https://www.google.com";
        http::Socket s;
        {
            auto ec = s.connect(example);
            if (!ec) {
                std::println(stderr, "Error during connect: {}", ec.error());
                std::exit(1);
            }
        }

        {
            auto ec = s.send("GET {} HTTP/1.1\r\nHost: {}\r\nUser-Agent: curl/8.22.0\r\nConnection: close\r\n\r\n", example.route, example.hostname);
            if (!ec) {
                std::println("Error during send: {}", ec.error());
                std::exit(1);
            }
        }

        std::string body;
        const auto func = [](const http::DownloadState &state) {
            std::println("Received {}/{} bytes ({} remaining)", state.bytes_received,
                         state.content_length, state.bytes_left);

            std::println("Body fragment:\n{}", state.buffer);

            return true;
        };

        {
            auto ec = s.readCallback(func);
            if (!ec) {
                std::println("Error during read: {}", ec.error());
                std::exit(1);
            }

            const auto &r = ec.value();

            std::println("Status: {} {}", int(r.status_code), r.reason);
            std::println("Headers:");

            for (const auto &[k, v] : r.header.entries()) {
                std::println("  {} = {}", k, v);
            }
        }

        // std::println("Body:");
        // std::println("{}", r.body);

        // std::println("Body size: {}", r.body.size());
    }
}
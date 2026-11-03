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
    printUrl(u);

    // oneshot read
    {
        http::URL example = "http://httpbin.org";
        http::Socket s(example);

        if (s.isValid()) {
            std::println("Connect to example.com succeeded");
        } else {
            std::println("Connect to example.com failed");
            return 1;
        }

        s.send("GET {} HTTP/1.1\r\n", example.route);
        s.send("Host: {}\r\n", example.hostname);
        s.send("User-Agent: curl/8.22.0\r\n");
        s.send("Connection: close\r\n\r\n");

        auto r = s.readAll();

        std::println("Status: {} {}", int(r.status_code), r.reason);
        std::println("Headers:");

        for (const auto &[k, v] : r.header.entries()) {
            std::println("  {} = {}", k, v);
        }

        // std::println("Body:");
        // std::println("{}", r.body);

        std::println("Body size: {}", r.body.size());
    }

    // callback read
    {
        http::URL example = "http://httpbin.org";
        http::Socket s(example);

        if (s.isValid()) {
            std::println("Connect to example.com succeeded");
        } else {
            std::println("Connect to example.com failed");
            return 1;
        }

        s.send("GET {} HTTP/1.1\r\n", example.route);
        s.send("Host: {}\r\n", example.hostname);
        s.send("User-Agent: curl/8.22.0\r\n");
        s.send("Connection: close\r\n\r\n");

        const auto func = [](const http::DownloadState &state) {
            std::println("Received {}/{} bytes ({} remaining)", state.bytes_received,
                         state.content_length, state.bytes_left);
        };

        auto r = s.readCallback(func);

        std::println("Status: {} {}", int(r.status_code), r.reason);
        std::println("Headers:");

        for (const auto &[k, v] : r.header.entries()) {
            std::println("  {} = {}", k, v);
        }

        // std::println("Body:");
        // std::println("{}", r.body);

        // std::println("Body size: {}", r.body.size());
    }
}
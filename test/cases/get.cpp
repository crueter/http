// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <string>
#include <print>
#include <format>

import hydrocarbon;
import tests;
import jacinth;

void test_http() {
    const hydro::URL url = "http://httpbin.org/get?param1=value1&param2=value2";
    auto ec = hydro::get(url);
    tests::check_err("httpbin.org/get request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("returned method", doc.get<std::string>("method"), "GET");
    tests::check_match("returned URL", doc.get<std::string>("url"), std::format("{}", url));
    tests::check_match("returned status code", ec->status_code, hydro::Status::Ok);

    const auto args = doc.get("args");
    tests::check_match("param1", args.get<std::string>("param1"), "value1");
    tests::check_match("param2", args.get<std::string>("param2"), "value2");
}

void test_get() {
    const hydro::URL url = "https://httpbin.org/get?param1=value1&param2=value2";
    auto ec = hydro::get(url);
    tests::check_err("HTTPS httpbin.org/get request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("HTTPS returned method", doc.get<std::string>("method"), "GET");
    tests::check_match("HTTPS returned URL", doc.get<std::string>("url"), std::format("{}", url));
    tests::check_match("HTTPS returned status code", ec->status_code, hydro::Status::Ok);

    const auto args = doc.get("args");
    tests::check_match("HTTPS param1", args.get<std::string>("param1"), "value1");
    tests::check_match("HTTPS param2", args.get<std::string>("param2"), "value2");
}

void test_drip() {
    // TODO: test callback cancel
    std::string body;
    const auto func = [&body](const hydro::DownloadState &state) {
        std::println("Received {}/{} bytes ({} remaining)", state.bytes_received,
                        state.content_length, state.bytes_left);

        body += state.buffer;

        return true;
    };

    static std::size_t bytes = 10;
    const auto url = std::format("https://httpbin.org/drip?delay=0.5&numbytes={}&duration=1", bytes);
    auto ec = hydro::get(url, {.callback = func});

    tests::check_err("httpbin.org/drip request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    tests::check_match("drip body size", body.size(), bytes);
}

int main() {
    test_http();
    test_get();
    test_drip();

    tests::exit();
}
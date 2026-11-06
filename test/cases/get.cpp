// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <string>
#include <format>

import hydrocarbon;
import tests;
import jacinth;

void test_get() {
    const hydro::URL url = "https://httpbin.org/get?param1=value1&param2=value2";
    auto ec = hydro::get(url);
    tests::check_err("httpbin.org/get request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    auto doc = jacinth::doc::read(ec->body);
    tests::check_match("returned method", doc.get<std::string>("method"), "GET");
    tests::check_match("returned URL", doc.get<std::string>("url"), std::format("{}", url));

    const auto args = doc.get("args");
    tests::check_match("param1", args.get<std::string>("param1"), "value1");
    tests::check_match("param2", args.get<std::string>("param2"), "value2");
}

void test_drip() {
    static std::size_t bytes = 10;
    const auto url = std::format("https://httpbin.org/drip?delay=0.5&numbytes={}&duration=1", bytes);
    auto ec = hydro::get(url);

    tests::check_err("httpbin.org/drip request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    tests::check_match("drip body size", ec->body.size(), bytes);
}

int main() {
    test_get();
    test_drip();
}
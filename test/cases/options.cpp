// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

import hydrocarbon;
import tests;
import jacinth;

void test_options() {
    const hydro::URL url = "https://httpbin.org/options";

    auto ec = hydro::options(url);
    tests::check_err("OPTIONS request succeeds", ec, hydro::Error::Success);

    if (!ec) return;

    tests::check_match("access-control-allow-methods header",
                       ec->headers.get("access-control-allow-methods").value_or(""),
                       "GET, POST, HEAD, PUT, DELETE, PATCH, QUERY, OPTIONS");
}

int main() {
    test_options();

    tests::exit();
}
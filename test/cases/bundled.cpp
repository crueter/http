// SPDX-FileCopyrightText: Copyright 2026 crueter
// SPDX-License-Identifier: MIT

#include <string>

#include "cert.h"

import hydrocarbon;
import tests;

void test_bundled_cert() {
    hydro::RequestOptions opts;
    opts.connect.cert = std::string{kCert};

    const hydro::URL url = "https://httpbin.org/get";
    auto ec = hydro::head(url, opts);
    tests::check_err("HEAD request succeeds", ec, hydro::Error::Success);
}

int main() {
    test_bundled_cert();

    tests::exit();
}
# SPDX-FileCopyrightText: Copyright 2026 crueter
# SPDX-License-Identifier: MIT

get_target_property(launcher hydrocarbon CXX_COMPILER_LAUNCHER)
if ("${launcher}" MATCHES "ccache")
    set_target_properties(hydrocarbon PROPERTIES
        CXX_COMPILER_LAUNCHER ""
        C_COMPILER_LAUNCHER "")
endif()
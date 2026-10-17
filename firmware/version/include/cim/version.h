/*
 * Copyright (c) 2026, The CIM Contributors
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CIM_VERSION_H
#define CIM_VERSION_H

/*
 * Version of the CIM platform this firmware was built with, from
 * `git describe --tags --always --dirty` of the CIM repository at build time:
 * "v0.1.0" on a release tag, "v0.1.0-5-g1a2b3c4" after it, with "-dirty" for
 * uncommitted changes. "unknown" if git was not available.
 */
extern const char cim_version[];

#endif

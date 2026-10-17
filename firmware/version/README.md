# Version

`cim_version` (`cim/version.h`) is the version of the CIM platform a firmware was built with. It is generated at every build from `git describe --tags --always --dirty` of the CIM repository, so it follows new commits and tags without reconfiguring:

| Build from | `cim_version` |
|---|---|
| tag `v0.1.0` | `v0.1.0` |
| 5 commits after the tag | `v0.1.0-5-g1a2b3c4` |
| uncommitted changes | `v0.1.0-5-g1a2b3c4-dirty` |
| no git available | `unknown` |

`cim_add_app()` links the library into every app. To set the version by hand, configure with `-DCIM_VERSION=<string>`.

The version is available to code as `cim_version`, to the log (the example app prints it at startup) and to the commissioning shell as the `version` command. A release is built from its tag by CI, so the version of a released firmware is exactly the tag ([REL-07](../../docs/conventions.md#rel-07-releases-are-built-by-ci-from-the-tag-and-report-their-version-must)).

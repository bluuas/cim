# Conventions

The rules for working on CIM. [CONTRIBUTING.md](../CONTRIBUTING.md) describes the workflow; this page holds the rules it follows.

Every rule has an ID, so a review can cite it (`GIT-04`). IDs are never reused: a dropped rule leaves its number retired. **MUST** and **MUST NOT** have no exceptions. **SHOULD** can be broken with a reason stated in the PR. Each rule gives its reason and, where there is one, what enforces it. A rule without an enforcement is checked in review only.

To change a rule, open a PR that edits this page and gives the reason.

## Git

### GIT-01 Nobody commits directly to `main` (MUST NOT)

Every change reaches `main` through a pull request.

*Why:* `main` stays reviewable and always passes CI. *Enforced by:* branch protection on `main`.

### GIT-02 Branches are named `<type>/<short-description>` (MUST)

The type is one of the following. Use only lowercase `a-z`, `0-9` and hyphens, with no consecutive, leading or trailing hyphens. Dots are allowed in release versions only.

| Type                  | Use for                                    |
|-----------------------|--------------------------------------------|
| `feat/` or `feature/` | new features                               |
| `fix/` or `bugfix/`   | bug fixes                                  |
| `hotfix/`             | urgent fixes                               |
| `release/`            | release preparation, e.g. `release/v1.2.0` |
| `chore/`              | everything else: docs, CI, tooling         |

Examples: `feat/tcan-api`, `fix/rx-overflow`, `chore/contributing`. The naming follows [Conventional Branch](https://conventionalbranch.org/).

*Why:* the prefix tells a reviewer what kind of change to expect, and a long branch list sorts by intent.

### GIT-03 A branch holds one change and is deleted after the merge (SHOULD)

*Why:* long-lived branches are where conflicts come from. *Enforced by:* merged branches are deleted automatically.

### GIT-04 Commit messages and PR titles follow Conventional Commits (MUST)

Format: `<type>(<optional scope>)!: <description>`, following [Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/).

| Type       | Use for                                                  |
|------------|----------------------------------------------------------|
| `feat`     | a new feature                                            |
| `fix`      | a bug fix                                                |
| `docs`     | documentation only                                       |
| `ci`       | CI configuration (GitHub Actions)                        |
| `build`    | build system, CMake, devcontainer, dependencies          |
| `refactor` | code change that neither fixes a bug nor adds a feature  |
| `test`     | adding or fixing tests                                   |
| `perf`     | performance improvement                                  |
| `style`    | formatting, no code change                               |
| `chore`    | anything else                                            |

Common scopes are `firmware`, `boards`, `tcan`, `bootloader`, `tools`, `hardware`, `docs` and `github`.

*Why:* with squash merges the PR title is the commit message on `main`, so `git log --oneline` reads as a changelog. *Enforced by:* the `pr-title` check for PR titles.

### GIT-05 The subject is imperative, lowercase and at most 72 characters (MUST)

Write "add", not "added", and no trailing period.

*Enforced by:* the `pr-title` check.

### GIT-06 The body says why, not what (SHOULD)

The diff shows what changed. Use the body for the reason, the alternative you rejected or the datasheet page you worked from.

### GIT-07 Breaking changes are marked (MUST)

Put `!` after the type or scope, or add a `BREAKING CHANGE:` footer that says what to do.

```
feat(bootloader)!: switch to new image header format

BREAKING CHANGE: apps must be rebuilt with the new linker script.
```

### GIT-08 Every PR closes or references an issue (MUST)

Put `Closes #<issue>` in the PR description, or `Refs #<issue>` if the issue stays open.

*Why:* the issue holds the discussion and the milestone; the PR holds the change.

### GIT-09 PRs are squash merged (MUST)

*Why:* one commit per PR keeps the history of `main` linear and each commit green. *Enforced by:* squash is the only merge method enabled, and `main` requires a linear history.

### GIT-10 A PR is up to date and green before it is merged (MUST)

The required checks are `firmware (cim_proto_v7-debug)`, `firmware (cim_proto_v7-release)`, `host-tests`, `consumer` and `tools-tests`.

*Enforced by:* branch protection on `main`.

### GIT-11 `main` is never force-pushed or deleted (MUST NOT)

*Why:* rewriting shared history breaks every other clone. *Enforced by:* branch protection on `main`.

## History

### HIS-01 Build artifacts and binaries are not committed (MUST NOT)

This covers `.elf`, `.bin`, `.uf2`, `.hex`, `.map`, `.zip`, `.o`, `.a`, `.so`, `.exe` and any added file over 1 MiB. Firmware binaries are published as release assets.

*Why:* git keeps every version of a binary forever, and removing one means rewriting history. *Enforced by:* `.gitignore` and the `no-binaries` check.

### HIS-02 Line endings are LF, decided by `.gitattributes` (MUST)

`* text=auto eol=lf` applies on every OS. `.bat`, `.cmd` and `.ps1` keep CRLF. Vendor files that must stay byte-identical (the TI driver files in `firmware/drivers/tcan4x5x/ti/`) are marked `-text`.

*Why:* the rule is committed, so CI and every clone get the same bytes. *Enforced by:* `.gitattributes`.

## Releases

### REL-01 Versions follow Semantic Versioning and are tagged `vMAJOR.MINOR.PATCH` (MUST)

For example `v1.2.3`. The `v` belongs to the tag, not to the version.

### REL-02 The change decides which part is incremented (MUST)

| Part    | Increment for                                                                 |
|---------|-------------------------------------------------------------------------------|
| `MAJOR` | incompatible change to the hardware, the CAN protocol or the API              |
| `MINOR` | new backward-compatible feature                                               |
| `PATCH` | bug fix without a new feature                                                 |

*Why:* the number answers "can I update without reflashing or changing the board?".

### REL-03 A tag is created only after the change is merged into `main` (MUST)

*Why:* the tag is what CI publishes, and a release of the wrong commit cannot be taken back. *Enforced by:* the `release` workflow fails if the tagged commit is not on `main`.

### REL-04 Tags are annotated, with a one-line description (MUST)

*Why:* an annotated tag has an author, a date and a message, and `git describe` treats it differently from a lightweight one.

### REL-05 A pushed tag is never moved or reused (MUST NOT)

If a release is wrong, publish the next `PATCH`.

### REL-06 A tag with a suffix is a pre-release (MUST)

For example `v1.2.0-rc.1`. *Enforced by:* the `release` workflow publishes it as a pre-release.

## Boards

### BRD-01 Board revisions are numbered and independent of the firmware version (MUST)

Boards are named `proto_v7`, and the firmware has its own version (REL-01). The `PICO_BOARD` name carries the board.

### BRD-02 Every release states the boards it supports (MUST)

*Enforced by:* the `release` workflow adds the list to the release notes. Update it when a board is added.

### BRD-03 Dropping support for a board is a `MAJOR` change (MUST)

*Why:* users of that board cannot update to the release.

## Documentation

### DOC-01 Links are relative to the repository, as on GitHub (MUST)

For example `../config/README.md`. The site rewrites them: links to pages stay links, links to source files point to GitHub.

### DOC-02 New Markdown files are added to `mkdocs.yml` (MUST)

A file outside `docs/` goes into `extra.repo_docs` and `nav`; a file inside `docs/` only into `nav`.

### DOC-03 Diagrams are Mermaid code blocks, not images (MUST)

### DOC-04 Notes are GitHub alerts (MUST)

Use `> [!NOTE]`, `[!TIP]`, `[!IMPORTANT]`, `[!WARNING]` or `[!CAUTION]`; the site renders them as admonitions.

### DOC-05 The docs build without warnings (MUST)

*Enforced by:* `mkdocs build --strict` in CI, which fails on broken links. Preview with `mkdocs serve` (in the devcontainer, or after `pip install -r docs/requirements.txt`).

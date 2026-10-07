# Contributing to CIM

Thanks for your interest in CIM! This page describes how work is organised and what a contribution should look like.

## Workflow

1. **Start from an issue.** Every change belongs to a GitHub issue. Pick an existing one or open a new one using a template (bug, task, or decision). Larger work is grouped into [milestones](https://github.com/bluuas/cim/milestones).
2. **Create a branch** from `main` (see [Branch names](#branch-names)).
3. **Open a pull request** against `main`:
   - The PR title follows [Conventional Commits](#commit-messages), because it becomes the commit message on `main`.
   - Put `Closes #<issue>` in the description.
   - Fill in the checklist: CI, hardware test, docs.
4. **CI must pass.** Every PR builds the firmware for all boards. `main` is protected and only accepts PRs that are up to date and green.
5. **Squash merge.** Each PR becomes a single commit on `main`, and the branch is deleted afterwards.

Design decisions (e.g. protocols, architecture) are discussed in an issue labelled `decision` and recorded as an ADR in `docs/adr/`.

## Branch names

We follow [Conventional Branch](https://conventionalbranch.org/):

```
<type>/issue-<number>-<short-description>
```

| Type                 | Use for                             |
|----------------------|-------------------------------------|
| `feat/` or `feature/` | new features                        |
| `fix/` or `bugfix/`   | bug fixes                           |
| `hotfix/`            | urgent fixes                        |
| `release/`           | release preparation, e.g. `release/v1.2.0` |
| `chore/`             | everything else: docs, CI, tooling  |

Rules:
- Only lowercase `a-z`, `0-9` and hyphens. Dots are allowed only in release versions.
- No consecutive, leading or trailing hyphens.
- No underscores or spaces.

Examples: `feat/issue-6-tcan-api`, `fix/issue-42-rx-overflow`, `chore/issue-24-contributing`.

## Commit messages

We follow [Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/):

```
<type>(<optional scope>)!: <description>

<optional body>

<optional footer(s)>
```

| Type       | Use for                                          |
|------------|--------------------------------------------------|
| `feat`     | a new feature                                    |
| `fix`      | a bug fix                                        |
| `docs`     | documentation only                               |
| `ci`       | CI configuration (GitHub Actions)                |
| `build`    | build system, CMake, devcontainer, dependencies  |
| `refactor` | code change that neither fixes a bug nor adds a feature |
| `test`     | adding or fixing tests                           |
| `perf`     | performance improvement                          |
| `style`    | formatting, no code change                       |
| `chore`    | anything else                                    |

Common scopes are `firmware`, `boards`, `tcan`, `bootloader`, `tools`, `hardware`, `docs` and `github`.

- Write the description in the imperative ("add", not "added"), in lowercase and without a trailing period.
- Mark breaking changes with `!` after the type/scope, or with a `BREAKING CHANGE:` footer.
- Reference issues in the footer: `Closes #12` or `Refs #12`.

Examples:

```
feat(tcan): add hardware ID filters
fix(boards): correct red LED pin on proto v7
docs: describe bench setup for CAN tests

feat(bootloader)!: switch to new image header format

BREAKING CHANGE: apps must be rebuilt with the new linker script.
Closes #14
```

## Building and testing

See the [README](README.md#building) for building with the devcontainer. Firmware changes should be tested on real hardware where possible. State the board you used in the PR checklist.

## Documentation

Documentation lives next to the code: every module has a `README.md`, decisions are ADRs in `docs/adr/`, and cross-cutting pages are in `docs/`. The [documentation site](https://bluuas.github.io/cim/) is built from these files by mkdocs, see `mkdocs.yml`.

- Write links relative to the repository, as on GitHub (e.g. `../config/README.md`). The site rewrites them: links to pages stay links, links to source files point to GitHub.
- A new Markdown file outside `docs/` is added to `extra.repo_docs` and to `nav` in `mkdocs.yml`; a file inside `docs/` only to `nav`.
- Draw diagrams as [Mermaid](https://mermaid.js.org/) code blocks, not as images.
- Write notes as [GitHub alerts](https://docs.github.com/en/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax#alerts) (`> [!NOTE]`, `[!TIP]`, `[!IMPORTANT]`, `[!WARNING]`, `[!CAUTION]`); the site renders them as admonitions.
- `mkdocs build --strict` runs in CI and fails on broken links. Preview with `mkdocs serve` (in the devcontainer, or after `pip install -r docs/requirements.txt`).

## License

By contributing, you agree that your contributions are licensed under the [BSD 3-Clause License](LICENSE).

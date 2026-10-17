# Contributing to CIM

How work on CIM is organised. The rules behind it, with IDs you can cite in a review, are in [Conventions](docs/conventions.md).

## Workflow

1. **Start from an issue.** Every change belongs to a GitHub issue. Pick an existing one or open a new one using a template (bug, task, or decision). Work is grouped into [milestones](https://github.com/bluuas/cim/milestones) named like the release that ships it (`v0.2.0`), and board spins are parent issues ([TRK-01](docs/conventions.md#trk-01-a-milestone-is-a-release-version-named-like-its-tag-must), [TRK-02](docs/conventions.md#trk-02-a-board-spin-is-a-parent-issue-with-sub-issues-not-a-milestone-must)).
2. **Create a branch** from `main`, named `<type>/<short-description>` ([GIT-02](docs/conventions.md#git-02-branches-are-named-typeshort-description-must)).
3. **Open a pull request** against `main`:
   - The title follows Conventional Commits, because it becomes the commit message on `main` ([GIT-04](docs/conventions.md#git-04-commit-messages-and-pr-titles-follow-conventional-commits-must)).
   - Put `Closes #<issue>` in the description.
   - Fill in the checklist: CI, hardware test, docs.
4. **CI must pass.** Every PR builds the firmware, builds the docs site, runs the host and tool tests, checks the title and rejects added binaries. `main` only accepts PRs that are up to date and green.
5. **Squash merge.** Each PR becomes a single commit on `main`, and the branch is deleted afterwards.

Design decisions (e.g. protocols, architecture) are discussed in an issue labelled `decision` and recorded as an ADR in `docs/adr/`.

## Releasing

Each release has a `Release vX.Y.Z` checklist issue in its milestone. Tag a commit that is already on `main`, using the next version ([REL-02](docs/conventions.md#rel-02-the-change-decides-which-part-is-incremented-must)):

```bash
git switch main && git pull
git log --oneline -3          # check that the expected commits are there
git tag -a v1.2.0 -m "v1.2.0 - short description"
git push origin v1.2.0
```

The `release` workflow then checks that the tagged commit is on `main`, builds the release presets for all boards and publishes a GitHub release with one zip (`.uf2` and `.elf`) per board. A tag with a suffix such as `v1.2.0-rc.1` is published as a pre-release.

## Building and testing

See the [README](README.md#building) for building with the devcontainer. Firmware changes should be tested on real hardware where possible. State the board you used in the PR checklist.

## Documentation

Documentation lives next to the code: every module has a `README.md`, decisions are ADRs in `docs/adr/`, and cross-cutting pages are in `docs/`. The [documentation site](https://bluuas.github.io/cim/) is built from these files by mkdocs, see `mkdocs.yml`. The writing rules are `DOC-01` to `DOC-08` in [Conventions](docs/conventions.md).

## License

By contributing, you agree that your contributions are licensed under the [BSD 3-Clause License](LICENSE).

# Architecture Decision Records

Design decisions: what was decided, why, and which alternatives were rejected. Each decision starts as a GitHub issue labelled `decision`; the ADR is added through a pull request.

| ADR | Title | Status |
|---|---|---|
| [0001](0001-pc-can-link.md) | PC ↔ CAN link | Accepted |
| [0002](0002-device-protocol.md) | Device and bootloader protocol | Accepted |

## Format

File name: `NNNN-short-title.md`. Sections:

- **Status:** Proposed, Accepted, Superseded by NNNN, or Rejected
- **Context:** the problem and the constraints
- **Options:** the alternatives that were considered
- **Decision:** what was chosen and why
- **Consequences:** what follows from the decision, including the downsides
- **Open points:** what still needs to be clarified

ADRs are not rewritten once accepted. If a decision changes, a new ADR supersedes the old one.

# Agent Execution & Project Rules

## General Behavior
- Break complex requests into a concrete task checklist before touching code.
- Keep git diffs minimal, clean, and focused strictly on the user's objective.
- Always verify changes by building or testing inside the terminal sandbox before marking a task complete.

## Feature Implementation
- Mirror surrounding architectural patterns, naming conventions, and file structures.
- Enforce strict typing and explicitly handle edge cases (e.g., null pointers, missing fields, async rejections).
- Never introduce new external dependencies without explicit user confirmation.

## Reverse Engineering & Auditing
- Map execution paths, hook points, and state transitions explicitly before proposing code changes.
- Trace memory layouts, binary structures, or implicit API contracts carefully.
- Document all reverse-engineering discoveries under `docs/` rather than altering production source files.

## Code Review & Quality Assurance
- Review every modified file against neighboring module conventions.
- Flag any unhandled error paths, memory safety issues, or performance regressions.
- Structure review feedback clearly by severity: `[Critical]`, `[Warning]`, and `[Suggestion]`.
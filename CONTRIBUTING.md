# Contribution Guidelines

fooyin is a free and open-source project. Contributions are encouraged through code, bug reports, documentation, or user support.

## Making Changes

- Keep changes focused and relevant.
- Avoid mixing unrelated changes in a single pull request.
- Ensure the project builds successfully before committing.

### Formatting

All code **must be formatted using Clang-format** with the repository’s `.clang-format` configuration.

    clang-format -i <files>

Do not submit code that conflicts with the formatting rules.

## Commits

- Keep commits small and logically grouped.
- Use clear, descriptive commit messages.

### Commit message format

- Prefix commits with a component tag in square brackets (e.g. `[core]`, `[gui]`, `[plugin]`).
- Keep the first line concise, with no trailing period.
- Add a detailed description after a blank line for non-trivial changes.

Format:

    [tag] Short summary

    Optional detailed explanation of what changed and why.

Example:

    [core] Fix playback state desync

    Resolves an issue where pausing during buffer underrun caused incorrect state transitions.

    Fixes #123

## Code Review

- All changes are reviewed before merging.
- Address feedback directly in follow-up commits.
- Keep discussions focused on technical correctness and maintainability.

## Issues

- Search existing issues and the CHANGELOG.md before opening a new one.
- Provide clear steps to reproduce bugs.
- Include logs, screenshots, or system details when relevant.

## Security

- Never include credentials, secrets, or sensitive data in commits or issues.
- Report security concerns privately to the maintainer.

## Licensing

By contributing, you agree that your contributions will be licensed under the same license as fooyin.

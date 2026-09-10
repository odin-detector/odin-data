# Contributing

Thank you for your interest in contributing to this project.

## Who Should Contribute?

We are interested in contributions from existing users of odin-data who have practical experience of using the software against a detector system. Contributions that make speculative improvements without identifying a clear user benefit are unlikely to be accepted.

Before proposing a change, contributors must:

- Have used odin-data in a real-world context against a detector system
- Have reviewed the existing issues and PRs to see if the problem has already been raised or is already being addressed
- Understand the problem the change is intended to solve and be prepared to explain the expected user benefit
- Be able to test their change against a detector system

Please note that this repo is configured such that only repository collaborators can open pull requests.

## Workflow

- Open an issue describing:
    - The change and why it is needed
    - Who you are, your motivations, and at which facility you work
    - Which detector system/s you are using with odin-data
- A maintainer will review your proposal and, if the change is accepted, add you as a repository collaborator
- Create a feature branch from the master branch
- Follow the setup steps detailed in README.md
- Make your changes in that branch
- Ensure the clang formatter has been run on changed code
- Submit a pull request referencing the issue. Do not commit directly to the master branch

## Code style
- Follow the existing coding style and structure
- Run the formatter (clang-format) before submitting code changes
- Keep classes and functions small and focused

## Tests
- All new code must include accompanying tests
- Existing tests must continue to pass
- Use Boost for C++ tests and pytest for Python tests

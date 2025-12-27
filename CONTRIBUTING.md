# Contributing to tulpar

Thank you for your interest in contributing to tulpar!

## Getting Started

1. Fork the [repository](https://github.com/NurOS-Linux/tulpar)
2. Clone your fork:
   ```bash
   git clone https://github.com/NurOS-Linux/tulpar.git
   cd tulpar
   ```
3. Set up the development environment (see [README.md](README.md) for dependencies)

## Development Workflow

### Building

```bash
meson setup build --buildtype=debug
meson compile -C build
```

### Running Tests

```bash
meson test -C build
```

## Submitting Changes

1. Create a new branch for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. Make your changes and commit them:
   ```bash
   git add .
   git commit -m "feat: add your feature description"
   ```

3. Push to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

4. Open a Pull Request at https://github.com/NurOS-Linux/tulpar

## Commit Message Format

We follow [Conventional Commits](https://www.conventionalcommits.org/):

- `feat:` - new feature
- `fix:` - bug fix
- `docs:` - documentation changes
- `refactor:` - code refactoring
- `test:` - adding or updating tests
- `chore:` - maintenance tasks

## Code Style

- Use consistent indentation (4 spaces)
- Follow existing code patterns in the project
- Write clear, descriptive variable and function names
- Add comments for complex logic

## Reporting Issues

When reporting issues at https://github.com/NurOS-Linux/tulpar/issues, please include:

- Operating system and version
- Steps to reproduce the problem
- Expected vs actual behavior
- Relevant error messages or logs

## License

By contributing, you agree that your contributions will be licensed under the [GPL-3.0](LICENSE) license.

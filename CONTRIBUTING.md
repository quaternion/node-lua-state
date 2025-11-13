# Contributing to lua-state

Thank you for your interest in contributing to lua-state! This document provides guidelines and information for contributors.

## Quick Start for Contributors

```bash
git clone https://github.com/quaternion/node-lua-state.git
cd node-lua-state
npm i
npm run test
```

## Development Setup

### Option 1: Dev Container (Recommended)

This project includes a dev container configuration for a consistent development environment with all necessary tools and dependencies preinstalled.

**Prerequisites:**

- [Docker](https://www.docker.com/get-started)
- [VS Code](https://code.visualstudio.com/) with [Dev Containers extension](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers)

**Setup:**

1. Clone the repository:

   ```bash
   git clone https://github.com/quaternion/node-lua-state.git
   cd node-lua-state
   ```

2. Open in VS Code and use Command Palette: `Dev Containers: Reopen in Container`

### Option 2: Local Development

**Prerequisites:**

- **Node.js 18+** - Required runtime
- **tar** - System tool for extracting archives (or install via npm)
- **C++ build environment** - For compiling native addons:
  - Linux: `build-essential` (Ubuntu/Debian) or equivalent
  - macOS: Xcode Command Line Tools
  - Windows: Visual Studio Build Tools

**Installation:**

1. Clone the repository:

   ```bash
   git clone https://github.com/quaternion/node-lua-state.git
   cd node-lua-state
   ```

2. Install dependencies and build the native addon:

   ```bash
   npm i
   ```

## Development Workflow

### Code Quality

This project uses [Biome](https://biomejs.dev/) for code formatting and linting:

- **Format code**: `npm run format`
- **Check code**: `npm run lint`

All code must pass linting checks before submission.

### Testing

Run the test suite:

```bash
npm run test
```

Tests are written using Node.js built-in test runner and cover JavaScript integration with the native addon.

### Benchmarking

Run performance benchmarks:

```bash
npm run bench
```

This helps ensure changes don't negatively impact performance.

### Building Native Binaries

During local development, prefer running:

```bash
./bin/lua-state.js install --force [options]
```

rather than `npx lua-state install`, as npx may trigger duplicate builds. See the [README](README.md) for detailed build options.

## Pull Request Process

1. **Fork** the repository and create a feature branch:

   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make changes** following the code style and guidelines

3. **Test thoroughly**:

   - Run `npm run test` to ensure all tests pass
   - Run `npm run lint` to check code quality
   - Run `npm run bench` to verify performance

4. **Update documentation** if needed (README, types, etc.)

5. **Commit** with clear, descriptive messages:

   ```bash
   git commit -m "feat: add new feature description"
   ```

6. **Push** to your fork and create a **Pull Request**

### Commit Message Format

Follow conventional commit format:

- `feat:` - New features
- `fix:` - Bug fixes
- `docs:` - Documentation changes
- `style:` - Code style changes
- `refactor:` - Code refactoring
- `test:` - Test additions/changes
- `chore:` - Maintenance tasks

## Code Guidelines

### TypeScript

- Use TypeScript for all new code
- Provide proper type annotations
- Type definitions live in `types/` and are published with the package.

### C++ Code

- Follow the existing code style in `src/`
- Use modern C++ features compatible with the target compilers
- Ensure thread-safety for Node.js integration

### JavaScript/Node.js

- This project uses CommonJS; avoid ESM imports unless necessary.
- Follow Node.js best practices
- Handle errors properly with detailed messages

## Testing Guidelines

- Write tests for new features
- Test edge cases and error conditions
- Ensure cross-platform compatibility
- Test with different Lua versions when applicable

## Reporting Issues

When reporting bugs:

1. Use the issue templates when available
2. Include your environment (OS, Node.js version, Lua version)
3. Provide minimal reproduction steps
4. Include error messages and stack traces

## Security

- Report security vulnerabilities privately to the maintainers
- Do not commit sensitive information
- Follow secure coding practices

## License

By contributing, you agree that your contributions will be licensed under the same MIT License as the project.

## Getting Help

- Check existing issues and documentation first
- Join discussions in GitHub issues
- Ask questions in pull request comments

Thank you for contributing to lua-state! 🎉

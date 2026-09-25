# Student Setup Guide

This repository is public. You can download and build the game without a GitHub account.

## 1. Install Git

- **macOS:** Open Terminal and run `git --version`. If prompted, install Apple's command-line tools.
- **Windows:** Install [Git for Windows](https://git-scm.com/download/win).
- **Linux:** Install Git with your distribution's package manager, such as `sudo apt install git` on Ubuntu.

## 2. Download the project

Open Terminal on macOS/Linux or Git Bash on Windows, choose where you keep school projects, and run:

```sh
git clone https://github.com/sandrofouche/isometric-snake.git
cd isometric-snake
```

You now have your own local copy. Cloning does not give you permission to change the public repository.

## 3. Build on macOS

Install [Homebrew](https://brew.sh/) if needed, then run:

```sh
brew install raylib pkg-config
make
make run
```

## 4. Build on Windows or Linux

Follow the platform instructions in [README.md](README.md). The CMake build downloads raylib automatically.

## 5. Get future updates

From inside the project folder:

```sh
git pull
```

Commit or copy your own work before pulling so you do not accidentally lose changes.

## Contributing changes

For class assignments, follow the workflow your instructor specifies. The usual public-repository workflow is:

1. Sign in to GitHub and open the repository page.
2. Select **Fork** to create a copy under your account.
3. Clone your fork instead of the instructor's repository.
4. Create a branch for your work: `git switch -c short-feature-name`.
5. Commit and push your changes.
6. Open a pull request back to the instructor's repository.

Never commit passwords, access tokens, private keys, or generated `build` files.

## Troubleshooting

- `make: command not found`: install the compiler/build tools for your operating system.
- `raylib was not found`: install raylib and `pkg-config`, or use the CMake build.
- The window does not open: check the terminal for the first error message and include it when asking for help.


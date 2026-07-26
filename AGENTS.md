# Repository Guidelines

# Project instructions

- Guide me step by step.
- Make the implementation very simple not an advansed code and logic
- Do not write implementation code unless I explicitly request it.
- Explain the concept before the implementation.
- Follow the existing project architecture.
- Run tests after making changes.
- Never modify unrelated files.

## Project Structure & Module Organization

Codexion is a C/POSIX-threads simulation. Production sources live in `src/`, with shared declarations in `includes/codexion.h`. Keep parsing, scheduling, timing, logging, coder, monitor, initialization, and cleanup logic in separate `.c` files as those components are added. `subject.txt` is the authoritative specification; `subject_explanation.txt` contains planning notes, not additional requirements. The `learning/` directory is scratch material and its compiled `a.out` is not part of the deliverable. Add automated checks under `tests/` rather than mixing them into `src/`.

## Build, Test, and Development Commands

The root `Makefile` is currently empty. Maintain it with the standard targets `all`, `clean`, `fclean`, and `re`, producing `./codexion`. Compilation must use `-Wall -Wextra -Werror -pthread` and include headers with `-Iincludes`.

Typical commands after the Makefile is implemented:

```sh
make
./codexion 5 800 200 200 200 3 10 fifo
make re
make fclean
```

Use `valgrind --leak-check=full ./codexion ...` when available to check allocations and synchronization cleanup. Do not commit generated binaries or object files.

## Coding Style & Naming Conventions

Follow the existing 42-style C layout: tabs for indentation, guarded headers, `snake_case` functions and variables, and `t_` prefixes for typedefs (`t_config`, `t_scheduler`). Use `e_` and `s_` tags for enums and structs. Keep functions focused, pass simulation state explicitly, and never introduce global variables. Shared mutable data must be accessed under its documented mutex; do not use `volatile` as synchronization.

## Testing Guidelines

No test framework exists yet. Add small C test programs or shell-driven integration cases under `tests/`. Name tests by behavior, such as `test_parse_numbers.c` or `test_single_coder.sh`. Cover invalid arguments, integer overflow, one coder, FIFO/EDF tie-breaking, dongle cooldown, successful completion, burnout timing (within 10 ms), and repeated stress runs. Run tests with both schedulers and inspect output for interleaved or post-stop messages.

## Commit & Pull Request Guidelines

Existing history uses short descriptive messages but has no consistent convention. Prefer concise imperative commits such as `add overflow-safe argument parsing` or `fix EDF request cancellation`. Keep each commit buildable and focused. Pull requests should summarize behavior, list commands and scenarios tested, link relevant issues, and call out concurrency invariants or timing trade-offs. Include representative logs for scheduling or output changes; screenshots are unnecessary for this CLI project.

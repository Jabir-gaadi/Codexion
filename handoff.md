# Codexion Project Handoff

## Project objective

Implement the mandatory **Codexion** project in C using POSIX threads.

The program must simulate coders arranged in a circle. Each coder is represented by a thread and repeatedly attempts to compile, debug, and refactor. Compiling requires the coder to hold two adjacent USB dongles simultaneously.

The executable receives eight mandatory arguments:

```text
./codexion number_of_coders time_to_burnout time_to_compile \
    time_to_debug time_to_refactor number_of_compiles_required \
    dongle_cooldown scheduler
```

Verified mandatory requirements from the attached subject:

- Global variables are forbidden.
- Each coder must be represented by a POSIX thread.
- There are as many dongles as coders, except that the single-coder case has one dongle.
- Each dongle's state must be protected by a mutex.
- A coder needs two dongles simultaneously to compile.
- A released dongle is unavailable until its cooldown expires.
- Dongles must arbitrate competing requests using `fifo` or `edf`.
- FIFO serves requests in arrival order.
- EDF uses `last_compile_start + time_to_burnout` as the deadline.
- Equal EDF deadlines require a deterministic tie-breaker.
- A custom priority queue implemented as a heap is mandatory.
- A separate monitor thread must detect burnout and stop the simulation.
- A burnout message must be printed within 10 ms of actual burnout, with minimal testing tolerance for hardware and OS scheduling.
- Logging must be serialized.
- The simulation stops when a coder burns out or every coder reaches `number_of_compiles_required`.
- All allocated memory must be freed.
- The program must compile with `-Wall -Wextra -Werror -pthread`.

## Current architecture

The following architecture is planned but has not been verified as implemented:

```text
codexion/
├── Makefile
├── includes/
│   └── codexion.h
└── srcs/
    ├── main.c
    ├── parsing.c
    ├── time_utils.c
    ├── logging.c
    ├── heap.c
    ├── dongle.c
    ├── acquire.c
    ├── coder.c
    ├── monitor.c
    ├── simulation.c
    ├── init.c
    └── cleanup.c
```

Planned responsibilities:

| File | Planned responsibility |
|---|---|
| `main.c` | Orchestrate parsing, initialization, thread startup, joins, cleanup, and exit status |
| `parsing.c` | Validate the eight arguments and construct the configuration |
| `time_utils.c` | Centralize timestamp, deadline, and timed-wait conversions |
| `logging.c` | Serialize required output and prevent normal logs after terminal burnout |
| `heap.c` | Implement the custom binary heap and FIFO/EDF comparison policy |
| `dongle.c` | Manage individual dongle state, release, and cooldown |
| `acquire.c` | Coordinate safe and fair acquisition of two dongles |
| `coder.c` | Implement the coder thread state machine |
| `monitor.c` | Detect burnout and global successful completion |
| `simulation.c` | Planned location for global simulation control, thread startup/join, and stop handling |
| `init.c` | Allocate and initialize simulation resources with partial-failure handling |
| `cleanup.c` | Destroy initialized synchronization objects and free memory |

The exact synchronization architecture for `acquire.c`, shutdown wake-ups, and monitor notification is not finalized.

## Files implemented

No Codexion project source files are currently verified in the workspace.

The only implementation supplied in the conversation is a draft helper function named:

```c
int parse_numbers(const char *str, long long max, long long *result);
```

This function was pasted in the conversation but has not been verified in an actual `parsing.c` file or compiled.

A separate supporting course file exists in the workspace:

```text
posix_threads_codexion_course.md
```

It is educational material, not part of the Codexion executable.

## Work completed

### Subject analysis

The project requirements and authorized functions were reviewed.

### Proposed configuration types

The following types were proposed:

```c
typedef enum e_scheduler
{
    SCHED_FIFO,
    SCHED_EDF
}   t_scheduler;
```

The proposed configuration fields are:

```text
number_of_coders               → int
time_to_burnout                → long long
time_to_compile                → long long
time_to_debug                  → long long
time_to_refactor               → long long
number_of_compiles_required    → int
dongle_cooldown                → long long
scheduler                      → t_scheduler
```

These declarations have been discussed but are not verified in a source file.

### Parsing helper design

The intended responsibility of `parse_numbers` is defined:

- Reject a null input string.
- Reject a null result pointer.
- Reject an empty string.
- Reject a negative maximum.
- Accept digits only.
- Detect overflow before multiplication and addition.
- Store the parsed value through `*result` only after the complete string is valid.
- Return `1` on success and `0` on every failure.
- Leave the decision about whether zero is valid to the caller.

The overflow condition currently used is:

```c
value > (max - digit) / 10
```

This check occurs before:

```c
value = value * 10 + digit;
```

### Architecture review

Several incomplete or unsafe acquisition designs were identified and rejected:

- Odd/even coder acquisition order may reduce the classic circular-lock deadlock, but it does not implement FIFO/EDF, cooldown, starvation prevention, or atomic two-dongle arbitration.
- Ordered mutex locking alone does not implement the required fair scheduler.
- Protecting two independent heaps with one global mutex makes inspection atomic but does not solve incompatible heap heads.
- Requiring a coder to be the head of both heaps can block forever with orders such as:

```text
Dongle 1: A → B
Dongle 2: B → A
```

- Waiting on a condition variable while holding two dongle mutexes is unsafe because `pthread_cond_wait` releases only the single mutex passed to it.
- A stop predicate protected by one mutex cannot safely control a condition wait using another mutex without an additional proven protocol; otherwise shutdown can be missed.

## Pending tasks

### Current parsing stage

- Correct the latest `parse_numbers` draft.
- Place it in an actual `parsing.c` file.
- Declare it appropriately; if it is used only within `parsing.c`, it can later be made `static`.
- Test valid input, invalid characters, empty input, signs, `INT_MAX`, `INT_MAX + 1`, and very long values.
- Implement scheduler parsing for exact `fifo` and `edf` matches.
- Implement `parse_arguments` only after the numeric helper is complete.
- Create and verify `t_scheduler` and `t_config` declarations.
- Create a temporary parsing test entry point and minimal Makefile.

### Later stages

- Decide and implement a deadline-addition strategy that prevents `long long` overflow.
- Finalize the clock and `pthread_cond_timedwait` strategy.
- Implement and unit-test the binary heap.
- Finalize the terminal-state and terminal-logging protocol.
- Design condition-variable predicates and their protecting mutexes.
- Design a monitor notification protocol without lost wake-ups.
- Formally design two-dongle arbitration that satisfies per-dongle FIFO/EDF, cooldown, liveness, shutdown cancellation, and per-dongle mutex requirements.
- Implement coder threads and the coder state machine.
- Implement the separate monitor thread.
- Implement partial initialization cleanup, thread-creation failure handling, joining, and final destruction.
- Run compilation, functional, timing, stress, race, and memory-leak tests.

## Important technical decisions

### Authorized functions only

The subject provides an explicit external-function list. Functions not present in it must not be used without a confirmed subject exception.

Rejected functions include:

```text
strtol
snprintf
pthread_condattr_init
pthread_condattr_setclock
pthread_condattr_destroy
```

`atoi` is authorized but is not being used for strict argument validation because it:

- cannot report conversion failure;
- accepts a numeric prefix in partially invalid strings;
- cannot distinguish invalid input from the valid value zero;
- does not provide safe overflow detection;
- returns `int`, while time fields are planned as `long long`.

### Numeric parser contract

The agreed return convention is:

```text
1 = success
0 = failure
```

Returning `-1` for some failures is rejected because `-1` is true in C and could be mistakenly accepted by code such as:

```c
if (!parse_numbers(...))
```

The helper accepts zero as a syntactically valid unsigned value. Field-specific validation decides whether zero is permitted.

### Configuration commit

`parse_arguments` should build a temporary `t_config` and copy it into the caller's configuration only after every argument is valid. This avoids leaving a partially initialized configuration after a later argument fails.

### Numeric limits

- The planned maximum for fields stored as `int` is `INT_MAX`.
- No arbitrary low limit such as 1000 coders has been accepted because the subject defines no such ceiling.
- Allocation-size overflow, allocation failure, and partial thread-creation failure must be handled later by initialization logic.
- Parsing up to `LLONG_MAX` does not alone make deadline arithmetic safe; timestamp-plus-duration overflow remains pending.

### Preliminary zero-value interpretation

The discussion proposed:

- `number_of_coders` must be at least 1, which follows from the subject's "one or more coders" requirement.
- `dongle_cooldown == 0` may logically mean immediate availability.

The subject does not explicitly state all other zero-value validation rules. Their final treatment is not yet verified and must remain easy to revise.

### Clock direction

The preliminary direction is to use `gettimeofday()`/real-time values consistently because the subject permits and recommends it for simplicity, and default condition variables use `CLOCK_REALTIME` for `pthread_cond_timedwait` absolute deadlines.

This is not implemented. The exact time utility and overflow behavior remain pending.

### One-coder behavior

The subject explicitly provides one dongle when there is one coder, while compiling requires two dongles simultaneously. The current interpretation is that the physical dongle cannot be duplicated or counted twice; the coder cannot begin compiling and eventually burns out.

### Completion authority

It was proposed that the monitor become the single authority that detects both burnout and all-coders-completed termination. This is not yet finalized or implemented.

## Commands and tests already run

No Codexion compilation command or executable test result has been provided or verified.

The following commands were suggested previously but are **not verified as run**:

```bash
cc -Wall -Wextra -Werror -pthread \
    -Iincludes srcs/main.c srcs/parsing.c -o codexion
```

No output from parsing tests, Norminette, Valgrind, Helgrind, ThreadSanitizer, or a simulation run has been supplied.

## Known problems

### Latest pasted `parse_numbers` draft

The latest version correctly:

- uses `long long` for the accumulated value;
- checks null pointers and empty input;
- rejects non-digit characters;
- checks overflow before arithmetic;
- stores the result through `*result`.

It still has these known issues:

1. Invalid characters return `-1` instead of the agreed failure value `0`.
2. Overflow returns `-1` instead of `0`.
3. The declaration `int (i), (digit);` is unnecessarily unusual and should use normal declarations.
4. It has not been compiled or tested in a verified project file.

### Architecture blockers

- The final fair two-dongle acquisition algorithm is unresolved.
- Per-dongle mutex compliance must be preserved.
- Incompatible independent heap heads must be solved algorithmically, not merely protected by one mutex.
- Shutdown condition predicates and wake-up mutexes are unresolved.
- Monitor notification must avoid lost wake-ups.
- Lock ordering has not been finalized.
- Compile-count completion timing has not been finalized.

## Coding rules agreed on

- Work one implementation stage at a time.
- Do not move forward until the current stage is implemented, tested, and reviewed.
- Provide pseudocode and guidance by default; provide full code only when explicitly requested.
- Give the required function list and each function's purpose before implementation.
- Use only functions authorized by the subject.
- Compile with `-Wall -Wextra -Werror -pthread`.
- Do not use global variables.
- Check return values from POSIX thread and synchronization functions.
- Do not use `volatile` as a replacement for synchronization.
- Do not use arbitrary sleeps to hide races or deadlocks.
- Do not claim correctness based only on observed execution order.
- Do not treat odd/even acquisition as a complete Codexion solution.
- Do not treat ordered mutex locking as an implementation of FIFO/EDF.
- Do not treat one global mutex as an automatic solution to incompatible heap heads.
- Every shared field must have a documented protecting mutex.
- Every condition variable must have an explicit predicate protected by the mutex used for waiting.
- Condition waits must recheck predicates in a `while` loop.
- Every important concurrency claim must be justified by an invariant, lock-order argument, queue-order argument, or happens-before relationship.
- Preserve exact required log formatting and prevent messages from interleaving.
- Handle partial initialization and cleanup without leaks, double destruction, or use-after-free.

## Next exact implementation step

Correct only the current `parse_numbers` helper:

1. Change the invalid-character failure from `return (-1)` to `return (0)`.
2. Change the overflow failure from `return (-1)` to `return (0)`.
3. Replace `int (i), (digit);` with ordinary declarations for `i` and `digit`.
4. Keep `value` as `long long`.
5. Keep `*result = value` only after the loop succeeds.
6. Place the function in `parsing.c` or provide the corrected pasted version.
7. Test it with the agreed boundary and invalid-input cases.

Do not implement `parse_arguments`, timing utilities, threads, mutexes, heaps, or simulation logic until this helper has been corrected and reviewed.

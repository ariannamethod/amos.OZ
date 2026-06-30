# amosOZ — Arianna Method Operating System

**The most atomic way to build a working OS-like environment from scratch.**

amosOZ is a single-file operating system environment implemented in three equivalent forms: C, HTML/JavaScript, and Python. It is not Linux. It is Linux-compatible in userland semantics, command behavior, filesystem vocabulary, permission logic, process abstractions, module ABI, shell behavior, and POSIX-like operational expectations — while remaining its own system.

**Current canonical implementation:** `amosoz.c` **v0.3.0** (C). Python and HTML versions are v0.1 parity targets — run `selftest` there after sync.

Dedicated to **Amos Oz** (עוז — strength, courage). The OZ layer is where extension becomes accountable.

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│              amosOZ Shell (treaty layer)              │
│  redirects > >>   pipes |   PATH   scripts #!/amossh │
├─────────────────────────────────────────────────────┤
│  Command Dispatcher  │  Environment  │  History      │
├──────────────────────┴───────────────┴──────────────┤
│                    OZ Layer                           │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌──────────────┐ │
│  │ Slots  │ │Modules │ │ Hooks  │ │  Overheads   │ │
│  └────────┘ └────────┘ └────────┘ └──────────────┘ │
│  ┌──────────────────────────────────────────────┐   │
│  │     OZ Ledger (AI substrate + undo)          │   │
│  └──────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────┤
│  Syscall API  │  Virtual FS  │  Mem  │  Procs       │
├─────────────────────────────────────────────────────┤
│  /proc (live)  │  /etc/os-release  │  HW detect     │
└─────────────────────────────────────────────────────┘
```

**AMOS** is the operating body: kernel state, filesystem, memory, processes, shell.  
**OZ** is the extensibility field: modules, slots, hooks, overheads, contracts, ledger.

## Naming

- **amos** — the operating system
- **OZ** — deliberately overdetermined:
  - A quiet reference to [Amos Oz](https://en.wikipedia.org/wiki/Amos_Oz)
  - Hebrew עוז (strength, might, courage)
  - The OZ layer: internal zone for overheads, slots, modules, AI-hooks, diagnostics

## Compatibility Statement

### What amosOZ IS compatible with

- Unix-like path semantics (`/`, `.`, `..`, absolute and relative)
- POSIX-like shell command vocabulary
- Permission flag notation (`rwxr-xr-x`) with **enforcement**
- Environment variable model and `PATH`
- Process/PID abstractions
- Device registry semantics
- Syscall-like internal API (`syscall` command + `kernel_syscall()`)
- Shell redirects (`>`, `>>`) and pipes (`|`)
- Executable scripts (`#!/amossh`, `.amos`)

### What amosOZ IS NOT

- Not a Linux kernel
- Not binary-compatible with Linux ELF
- Not a real bootloader
- Not real process isolation
- Not real hardware control

### What is REAL (in C v0.3.0)

- Working virtual filesystem with permission checks
- `kernel_syscall()` layer (read/write/stat/chdir/alloc/spawn/…)
- Live virtual `/proc/*` and `/etc/os-release`
- Hooks that fire on boot, tick, and command dispatch
- OZ Ledger with parsed args, reversibility, and `undo`
- `/bin` stubs + `PATH` resolution + `which` / `exec`
- Script runner with `$@` / `$1` expansion
- `selftest` (32 checks) + external `tests/shell_treaty.sh`

### What is SIMULATED

- Process scheduling (tick-based, no preemption)
- Memory protection (flags stored, not hardware-enforced)
- Device I/O (registry only)
- Kernel-space execution (runs in userspace)

## Build / Run / Test

### C (canonical)

```bash
make              # gcc -Wall -O2 -o amosoz amosoz.c -lm
make test         # selftest — 32 internal checks
make test-shell   # shell treaty smoke (>, >>, |, which, exec)
./amosoz
```

### HTML / Browser

Open `amosoz.html` in a modern browser. No server required. *(Parity with C v0.3 pending.)*

### Python

```bash
python3 amosoz.py   # Parity with C v0.3 pending
```

## Shell Treaty (v0.3)

```bash
echo hello > /tmp/out.txt      # redirect overwrite
echo more >> /tmp/out.txt      # redirect append
echo piped | cat               # pipe to stdin-aware command
which echo                     # PATH lookup → /bin/echo
exec /home/user/hello.amos world   # run #!/amossh script
```

Scripts support `#!/amossh`, `.amos` extension, `$@` and `$1`–`$9`.  
`/bin/*` stubs use `__builtin__` — basename maps to kernel builtin.

## Commands

| Category | Commands |
|----------|----------|
| System | `uname`, `version`, `boot`, `hw`, `devices`, `gpu`, `date`, `motd`, `whoami` |
| Filesystem | `pwd`, `cd`, `ls`, `cat`, `touch`, `write`, `append`, `rm`, `mkdir`, `rmdir`, `mv`, `cp`, `chmod`, `stat`, `tree` |
| Memory | `mem`, `mmap`, `alloc`, `free` |
| Process | `ps`, `run`, `kill`, `tick`, `status` |
| Shell | `echo`, `env`, `set`, `unset`, `history`, `clear`, `help`, `exit`, `which`, `exec` |
| Syscall | `syscall` (read, write, stat, getcwd, chdir, alloc, free, spawn, kill, getenv) |
| OZ Layer | `oz`, `slots`, `modules`, `overhead`, `hooks`, `contracts` |
| Modules | `loadmod`, `unloadmod`, `call`, `fortune` |
| Ledger | `trace`, `replay`, `undo` |
| Persistence | `save`, `load` |
| Other | `selftest`, `reset` |

Shell operators: `>`, `>>`, `|` (parsed before dispatch).

## OZ Ledger

AI-ready deterministic command provenance. Records: full command, parsed cmd/args, actor, tick, timestamp, result, explanation, reversibility + undo payload.

```bash
trace 10      # last N entries
replay        # full log with parsed fields
undo          # revert last reversible mutation
```

## Virtual /proc

Live files refreshed on read and tick:

- `/proc/uptime`, `/proc/meminfo`, `/proc/cpuinfo`, `/proc/version`, `/proc/self/status`
- `/etc/os-release`, `/etc/motd`

## Selftest

```
amosOZ Selftest (32/32 passed):
  [PASS] boot_state … [PASS] script_exec
  ALL TESTS PASSED
```

## How to Add a Module

See existing patterns in `init_builtin_modules()` and `CMD_TABLE` in `amosoz.c`. Modules declare slots, hooks, contracts (`provides` / `requires`). Contract validation runs at boot.

## Built-in Modules

| Module | Slot(s) | Notes |
|--------|---------|-------|
| coreutils | shell.commands | uptime |
| hwprobe | diagnostics, boot hook | hwinfo |
| diag | diagnostics | diag_status |
| ai_seed | ai.hooks | requires oz.ledger |
| oz_ledger | oz.ledger, oz.contracts | trace/replay |
| fortune_ext | shell.commands, experiments | fortune |

## Persistence

| Platform | Mechanism |
|----------|-----------|
| C | `amosoz.img` binary snapshot (`save` / `load`) |
| Python | JSON `amosoz.img` |
| HTML | localStorage |

## Known Limitations

1. No real process isolation — `run` spawns table entries, scripts run in-shell
2. No preemptive scheduling — manual `tick`
3. No `<` stdin redirect yet
4. No conditionals/loops in shell (scripts are linear)
5. Single user session
6. No networking
7. `MAX_CONTENT` = 4096 bytes per file (C)
8. Dynamic `loadmod` not supported — modules compiled-in
9. Python/HTML not yet synced to C v0.3

## Roadmap

- **v0.3** ✅ C treaty: permissions, syscall, /proc, ledger+undo, shell, scripts, PATH
- **v0.4** (next): text tools (`grep`, `head`, `tail`, `wc`), `export`/`source`, `ls -l`, golden test suite
- **v0.5** Resonance: θ-metrics, actor sessions, `export report` → PROTOCOL.md
- **Parity**: sync `amosoz.py` + `amosoz.html` to C SPEC

## Design Principles

> "The most atomic way to build an OS from scratch."  
> "Compatibility is a treaty, not obedience."  
> "A slot is a promise with a boundary."  
> "OZ begins where extension becomes accountable."  
> "Every command leaves a trace."  

## License

See repository license.
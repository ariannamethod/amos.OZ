# amosOZ — Arianna Method Operating System

**The most atomic way to build a working OS-like environment from scratch.**

amosOZ is a single-file operating system environment implemented in three equivalent forms: C, HTML/JavaScript, and Python. It is not Linux. It is Linux-compatible in userland semantics, command behavior, filesystem vocabulary, permission logic, process abstractions, module ABI, shell behavior, and POSIX-like operational expectations — while remaining its own system.

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                    amosOZ Shell                       │
├─────────────────────────────────────────────────────┤
│  Command Dispatcher  │  Environment  │  History      │
├──────────────────────┴───────────────┴──────────────┤
│                    OZ Layer                           │
│  ┌────────┐ ┌────────┐ ┌────────┐ ┌──────────────┐ │
│  │ Slots  │ │Modules │ │ Hooks  │ │  Overheads   │ │
│  └────────┘ └────────┘ └────────┘ └──────────────┘ │
│  ┌──────────────────────────────────────────────┐   │
│  │           OZ Ledger (AI Substrate)           │   │
│  └──────────────────────────────────────────────┘   │
├─────────────────────────────────────────────────────┤
│  Virtual    │  Virtual   │  Process  │  Device       │
│  Filesystem │  Memory    │  Table    │  Registry     │
├─────────────────────────────────────────────────────┤
│              Hardware Detection Layer                 │
└─────────────────────────────────────────────────────┘
```

**AMOS** is the operating body: kernel state, filesystem, memory, processes, shell.  
**OZ** is the extensibility field: modules, slots, hooks, overheads, contracts, ledger.

## Naming

The name "amosOZ" is architecturally significant:
- **amos** — the operating system
- **OZ** — deliberately overdetermined:
  - A quiet reference to Amos Oz
  - Hebrew עוז (strength, might, courage)
  - The OZ layer: internal zone for overheads, slots, modules, AI-hooks, diagnostics

## Compatibility Statement

### What amosOZ IS compatible with:
- Unix-like path semantics (`/`, `.`, `..`, absolute and relative)
- POSIX-like shell command vocabulary
- Permission flag notation (`rwxr-xr-x`)
- Environment variable model
- Process/PID abstractions
- Device registry semantics
- Syscall-like internal API

### What amosOZ IS NOT:
- Not a Linux kernel
- Not binary-compatible with Linux ELF
- Not a real bootloader
- Not real process isolation
- Not real hardware control

### What is REAL:
- Working virtual filesystem with directories, files, permissions
- Working memory allocation/free model
- Working process table with spawn/kill/tick
- Working module load/unload with slot occupation
- Working overhead accounting
- Working command provenance ledger
- Actual host hardware detection

### What is SIMULATED:
- Process scheduling (tick-based, no preemption)
- Memory protection (flags stored, not enforced by hardware)
- Device I/O (registry only, no real device access)
- Kernel-space execution (runs in userspace)

## AI-Related Feature: OZ Ledger

The **OZ Ledger** is an AI-ready deterministic command provenance and intent trace layer.

**Research basis:** Operating systems traditionally provide audit logs (Linux auditd, Windows Event Log) but these are designed for security compliance, not AI reasoning. The OZ Ledger is designed as an *AI substrate* — a structured, deterministic, replayable record of all system mutations that an AI could safely inspect, reason over, replay, or audit.

**What it records:**
- Command entered and parsed form
- Actor/session identifier
- Tick/timestamp
- Result code
- Human-readable explanation
- Reversibility flag

**Shell commands:**
- `trace [n]` — show last N ledger entries
- `replay` — show full replay log
- `contracts` — show module behavior contracts

**Integration:**
- Participates in OZ overhead accounting
- Registered in `oz.ledger` and `oz.contracts` slots
- Included in selftest
- Implemented equivalently in all three versions

## Build/Run Instructions

### C Version

```bash
gcc -o amosoz amosoz.c
./amosoz
```

### HTML/Browser Version

Open `amosoz.html` directly in any modern browser. No server required. No external dependencies.

### Python Version

```bash
python3 amosoz.py
```

## Commands

| Category | Commands |
|----------|----------|
| System | `uname`, `version`, `boot`, `hw`, `devices`, `gpu`, `date` |
| Filesystem | `pwd`, `cd`, `ls`, `cat`, `touch`, `write`, `append`, `rm`, `mkdir`, `rmdir`, `mv`, `cp`, `chmod`, `stat`, `tree` |
| Memory | `mem`, `mmap`, `alloc`, `free` |
| Process | `ps`, `run`, `kill`, `tick`, `status` |
| Shell | `echo`, `env`, `set`, `unset`, `history`, `clear`, `help`, `exit` |
| OZ Layer | `oz`, `slots`, `modules`, `overhead`, `hooks`, `contracts` |
| Modules | `loadmod`, `unloadmod`, `call`, `fortune` |
| Ledger | `trace`, `replay` |
| Persistence | `save`, `load` |
| Other | `selftest`, `reset` |

## Selftest

Run `selftest` in any implementation to verify:

```
amosOZ Selftest (20/20 passed):
  [PASS] boot_state
  [PASS] hw_profile
  [PASS] version
  [PASS] fs_write_read
  [PASS] fs_append
  [PASS] fs_delete
  [PASS] fs_mkdir
  [PASS] fs_rmdir
  [PASS] permission_mutation
  [PASS] mem_alloc
  [PASS] mem_free
  [PASS] command_dispatch
  [PASS] process_spawn
  [PASS] process_kill
  [PASS] module_registered
  [PASS] slot_occupation
  [PASS] overhead_accounting
  [PASS] oz_layer
  [PASS] oz_ledger

  ALL TESTS PASSED
```

## How to Add a New Module

A module is a guest with manners. Here's the pattern:

### Python

```python
class MyModule:
    name = "mymod"
    description = "My custom module"
    commands = ["mycmd"]          # Commands this module provides
    slots = ["shell.commands"]    # Slots to occupy
    hooks = ["experiment"]        # Hooks to subscribe to
    contract = {"provides": ["mycmd"], "requires": [], "version": "0.1"}

    def init(self):
        pass  # Called on module load

    def shutdown(self):
        pass  # Called on module unload

    def mycmd(self):
        return "Hello from mymod!"
```

Register it: `kernel.oz.register_module("mymod", MyModule())`

### C

Add a `register_module()` call in `init_builtin_modules()`:

```c
{ const char *c[]={"mycmd"}; const char *s[]={"shell.commands"};
  const char *h[]={}; const char *p[]={"mycmd"}; const char *r[]={};
  register_module("mymod","My module",c,1,s,1,h,0,p,1,r,0,"0.1"); }
```

Add a command handler in `CMD_TABLE` and implement the function.

### HTML/JavaScript

Add to the MODULES object:

```javascript
mymod: { name:"mymod", description:"My module", commands:["mycmd"],
    slots:["shell.commands"], hooks:["experiment"],
    contract:{provides:["mycmd"],requires:[],version:"0.1"},
    init(){}, mycmd() { return "Hello from mymod!"; }}
```

### Module API Access

Modules can access kernel subsystems through:
- **Filesystem syscalls:** `kernel.syscall("read", path)`, `kernel.syscall("write", path, content)`
- **Memory syscalls:** `kernel.syscall("alloc", size)`, `kernel.syscall("free", id)`
- **Process syscalls:** `kernel.syscall("spawn", name)`, `kernel.syscall("kill", pid)`
- **Slots:** Declare in `slots` array to occupy named slots
- **Hooks:** Declare in `hooks` array to subscribe to lifecycle events
- **Contract:** Declare `provides` and `requires` for dependency tracking
- **Overhead:** Automatically accounted when module loads (dispatch entries, hooks, slots)

## Built-in Modules

| Module | Description | Slot(s) | Commands |
|--------|-------------|---------|----------|
| coreutils | Core utilities | shell.commands | uptime |
| hwprobe | Hardware probe | diagnostics | hwinfo |
| diag | Diagnostics | diagnostics | diag_status |
| ai_seed | AI seed hooks | ai.hooks | ai_status |
| oz_ledger | OZ Ledger provenance | oz.ledger, oz.contracts | ledger_size |
| fortune_ext | Example extension | shell.commands, experiments | fortune |

## Built-in Slots

| Slot | Purpose |
|------|---------|
| shell.commands | Command injection point |
| fs.drivers | Filesystem driver extensions |
| devices | Device registration |
| ai.hooks | AI integration points |
| sched.hooks | Scheduler extensions |
| boot.hooks | Boot-time initialization |
| diagnostics | Diagnostic tools |
| experiments | Experimental features |
| oz.contracts | Contract registry |
| oz.ledger | Provenance tracking |

## Persistence

| Platform | Mechanism | Command |
|----------|-----------|---------|
| C | Binary file (`amosoz.img`) | `save` / `load` |
| Python | JSON file (`amosoz.img`) | `save` / `load` |
| HTML/JS | localStorage | `save` / `load` |

## Known Limitations

1. **No real process isolation** — processes are table entries, not isolated execution contexts
2. **No preemptive scheduling** — `tick` advances the scheduler manually
3. **Memory flags not enforced** — protection bits are stored but not hardware-enforced
4. **No pipe/redirect** — shell does not support `|`, `>`, `<` operators
5. **No scripting** — no conditional/loop constructs in the shell
6. **No multi-user** — single user session
7. **No networking** — fully self-contained by design
8. **File size limits in C** — MAX_CONTENT = 4096 bytes per file
9. **Dynamic module loading** — modules must be compiled in (no runtime code loading)
10. **No signal handling** — no SIGINT/SIGTERM equivalent beyond `kill`

## Design Principles

> "The most atomic way to build an OS from scratch."  
> "And let there be overhead."  
> "A slot is a promise with a boundary."  
> "Compatibility is a treaty, not obedience."  
> "The shell is the first weather of the system."  
> "No kernel mythology here: only state, contracts, and consequences."  
> "OZ begins where extension becomes accountable."  
> "Every command leaves a trace."  

## License

See repository license.

# amosOZ — Arianna Method Operating System

**The most atomic way to build a working OS-like environment from scratch.**

amosOZ is a single-file OS environment in three forms: C, HTML/JS, Python. Not Linux — but **treaty-compatible** with Unix userland semantics.

**Canonical:** `amosoz.c` **v0.4.0** — reference implementation (llm.c-grade for OS).  
**Parity:** `amosoz.py` + `amosoz.html` lag behind; sync planned after C stabilizes.

Dedicated to **Amos Oz** (עוז). Resonance layer (θ, agents, export report) starts after v0.4.

## Quick Start

```bash
make && make test-all && ./amosoz
```

- `make test` — 43 internal selftest checks  
- `make test-shell` — shell treaty + reference commands smoke  
- `spec` / `doctor` — inside the shell

## Shell Treaty

```bash
echo hi > /tmp/x.txt          # >
echo more >> /tmp/x.txt       # >>
echo piped | grep piped       # |
cat < /etc/hostname           # <
which grep                    # PATH → /bin/grep
exec /home/user/hello.amos world
fortune oz                    # Amos Oz quotes from /usr/share/amosoz/
```

Operators: `>`, `>>`, `|`, `<`  
Scripts: `#!/amossh`, `.amos`, `$@`, `$1`…`$9`  
`/bin/*` stubs: `__builtin__` → kernel command

## Commands (C v0.4 reference set)

| Category | Commands |
|----------|----------|
| System | `uname`, `version`, `boot`, `hw`, `devices`, `gpu`, `date`, `motd`, `whoami`, `uptime`, `hostname`, `id`, `dmesg` |
| Filesystem | `pwd`, `cd`, `ls` (`-l`, `-la`, `-a`), `cat`, `touch`, `write`, `append`, `rm`, `mkdir`, `rmdir`, `mv`, `cp`, `chmod`, `stat`, `tree`, `ln -s`, `find` |
| Text | `grep` (`-i`), `head` (`-n`), `tail` (`-n`), `wc` (`-l`, `-w`, `-c`) |
| Memory | `mem`, `mmap`, `alloc`, `free` |
| Process | `ps`, `run`, `kill`, `tick`, `status`, `jobs`, `fg`, `nohup` (stubs) |
| Shell | `echo`, `env`, `set`, `unset`, `export`, `source`, `history`, `clear`, `help`, `exit`, `which`, `exec`, `test`, `[` |
| Logic | `true`, `false` |
| Syscall | `syscall` |
| OZ | `oz`, `slots`, `modules`, `overhead`, `hooks`, `contracts`, `trace`, `replay`, `undo` |
| Meta | `spec`, `doctor`, `selftest`, `reset`, `fortune`, `fortune oz` |
| Persist | `save`, `load` |

## Architecture

**AMOS** — body: FS, mem, procs, shell, syscall, /proc  
**OZ** — field: modules, slots, hooks, contracts, ledger

Features: permission enforcement, live `/proc`, ledger+undo, hooks, symlinks, `/usr/share/amosoz/`.

## Selftest (43/43)

```
make test
```

Covers v0.3 treaty + v0.4 tiers: grep, head, wc, test, ln, find, export, fortune oz, spec, doctor, `<` redirect.

## Parity Status

| File | Version | Status |
|------|---------|--------|
| `amosoz.c` | 0.4.0 | **canonical reference** |
| `amosoz.py` | 0.1.x | needs sync |
| `amosoz.html` | 0.1.x | needs sync |

Triple-file parity = same selftest + shell_treaty passing on all three. C leads; py/html follow.

## Roadmap

- **v0.4** ✅ C reference tiers A–E  
- **v0.4.1** py/html parity (optional “угореть” sprint)  
- **v0.5** Resonance: θ, actors, `export report`, AML runtime hooks  

## Design Principles

> "Compatibility is a treaty, not obedience."  
> "Every command leaves a trace."  
> "OZ begins where extension becomes accountable."  

## License

See repository license.
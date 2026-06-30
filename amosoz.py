#!/usr/bin/env python3
"""
amosOZ — Arianna Method Operating System
The most atomic way to build a working OS-like environment from scratch.
Single file. No external dependencies. The complete first algorithm.

AMOS is the operating body. OZ is the extensibility field.
"""

import sys
import os
import time
import json
import platform
import struct
import readline
import hashlib

VERSION = "0.1.0"
SYSTEM_NAME = "amosOZ"
BUILD_DATE = "2026-06-30"

# ─── Hardware/Profile Detection ───────────────────────────────────────────────
# Detection must never crash. Unknown = "unknown", not guessed.

def detect_hardware():
    """Detect host environment using only stdlib."""
    hw = {
        "platform": platform.system() or "unknown",
        "platform_release": platform.release() or "unknown",
        "machine": platform.machine() or "unknown",
        "processor": platform.processor() or "unknown",
        "python_version": platform.python_version(),
        "cpu_count": "unknown",
        "memory_approx": "unknown",
        "terminal": "unknown",
        "persistence": True,
        "hostname": "unknown",
    }
    try:
        hw["cpu_count"] = str(os.cpu_count() or "unknown")
    except Exception:
        pass
    try:
        hw["hostname"] = platform.node() or "unknown"
    except Exception:
        pass
    try:
        if os.path.exists("/proc/meminfo"):
            with open("/proc/meminfo") as f:
                for line in f:
                    if line.startswith("MemTotal:"):
                        kb = int(line.split()[1])
                        hw["memory_approx"] = f"{kb // 1024} MB"
                        break
    except Exception:
        pass
    try:
        hw["terminal"] = os.environ.get("TERM", "unknown")
    except Exception:
        pass
    return hw


# ─── Error Codes ──────────────────────────────────────────────────────────────

ERR_OK = 0
ERR_NOT_FOUND = 1
ERR_PERMISSION = 2
ERR_EXISTS = 3
ERR_NO_MEMORY = 4
ERR_INVALID = 5
ERR_IO = 6
ERR_NO_PROCESS = 7
ERR_MODULE = 8


# ─── Virtual Memory ──────────────────────────────────────────────────────────
# "No kernel mythology here: only state, contracts, and consequences."

class VirtualMemory:
    def __init__(self, total_kb=65536):
        self.total = total_kb
        self.used = 0
        self.blocks = {}  # id -> {size, flags, owner}
        self.next_id = 1

    def alloc(self, size, owner="system", flags="rw-"):
        if self.used + size > self.total:
            return None, ERR_NO_MEMORY
        bid = self.next_id
        self.next_id += 1
        self.blocks[bid] = {"size": size, "flags": flags, "owner": owner}
        self.used += size
        return bid, ERR_OK

    def free(self, bid):
        if bid not in self.blocks:
            return ERR_NOT_FOUND
        self.used -= self.blocks[bid]["size"]
        del self.blocks[bid]
        return ERR_OK

    def stats(self):
        return {
            "total_kb": self.total,
            "used_kb": self.used,
            "free_kb": self.total - self.used,
            "blocks": len(self.blocks),
        }

    def mmap(self):
        return list(self.blocks.items())


# ─── Virtual Filesystem ──────────────────────────────────────────────────────
# "Compatibility is a treaty, not obedience."

class VirtualFS:
    def __init__(self):
        self.tree = {}  # path -> {type, content, perms, created, modified, owner}
        self.cwd = "/"
        self._init_tree()

    def _init_tree(self):
        dirs = ["/", "/bin", "/etc", "/home", "/tmp", "/var", "/dev",
                "/proc", "/sys", "/usr", "/usr/lib", "/usr/share",
                "/home/user", "/var/log", "/etc/amosoz"]
        for d in dirs:
            self.tree[d] = {"type": "dir", "perms": "rwxr-xr-x", "created": time.time(),
                            "modified": time.time(), "owner": "root"}
        # Initial files
        self.tree["/etc/hostname"] = {"type": "file", "content": "amosoz",
                                       "perms": "rw-r--r--", "created": time.time(),
                                       "modified": time.time(), "owner": "root"}
        self.tree["/etc/amosoz/version"] = {"type": "file", "content": VERSION,
                                             "perms": "rw-r--r--", "created": time.time(),
                                             "modified": time.time(), "owner": "root"}

    def _resolve(self, path):
        if not path.startswith("/"):
            if self.cwd == "/":
                path = "/" + path
            else:
                path = self.cwd + "/" + path
        # Normalize
        parts = []
        for p in path.split("/"):
            if p == "" or p == ".":
                continue
            elif p == "..":
                if parts:
                    parts.pop()
            else:
                parts.append(p)
        return "/" + "/".join(parts) if parts else "/"

    def exists(self, path):
        return self._resolve(path) in self.tree

    def is_dir(self, path):
        r = self._resolve(path)
        return r in self.tree and self.tree[r]["type"] == "dir"

    def mkdir(self, path):
        r = self._resolve(path)
        if r in self.tree:
            return ERR_EXISTS
        parent = "/".join(r.split("/")[:-1]) or "/"
        if parent not in self.tree or self.tree[parent]["type"] != "dir":
            return ERR_NOT_FOUND
        self.tree[r] = {"type": "dir", "perms": "rwxr-xr-x", "created": time.time(),
                        "modified": time.time(), "owner": "user"}
        return ERR_OK

    def touch(self, path):
        r = self._resolve(path)
        if r in self.tree:
            self.tree[r]["modified"] = time.time()
            return ERR_OK
        parent = "/".join(r.split("/")[:-1]) or "/"
        if parent not in self.tree:
            return ERR_NOT_FOUND
        self.tree[r] = {"type": "file", "content": "", "perms": "rw-r--r--",
                        "created": time.time(), "modified": time.time(), "owner": "user"}
        return ERR_OK

    def write(self, path, content):
        r = self._resolve(path)
        if r in self.tree and self.tree[r]["type"] == "dir":
            return ERR_INVALID
        parent = "/".join(r.split("/")[:-1]) or "/"
        if parent not in self.tree:
            return ERR_NOT_FOUND
        if r not in self.tree:
            self.tree[r] = {"type": "file", "content": content, "perms": "rw-r--r--",
                            "created": time.time(), "modified": time.time(), "owner": "user"}
        else:
            self.tree[r]["content"] = content
            self.tree[r]["modified"] = time.time()
        return ERR_OK

    def append(self, path, content):
        r = self._resolve(path)
        if r not in self.tree or self.tree[r]["type"] != "file":
            return ERR_NOT_FOUND
        self.tree[r]["content"] += content
        self.tree[r]["modified"] = time.time()
        return ERR_OK

    def read(self, path):
        r = self._resolve(path)
        if r not in self.tree:
            return None, ERR_NOT_FOUND
        if self.tree[r]["type"] != "file":
            return None, ERR_INVALID
        return self.tree[r]["content"], ERR_OK

    def rm(self, path):
        r = self._resolve(path)
        if r not in self.tree:
            return ERR_NOT_FOUND
        if self.tree[r]["type"] == "dir":
            return ERR_INVALID
        del self.tree[r]
        return ERR_OK

    def rmdir(self, path):
        r = self._resolve(path)
        if r not in self.tree:
            return ERR_NOT_FOUND
        if self.tree[r]["type"] != "dir":
            return ERR_INVALID
        # Check empty
        for k in self.tree:
            if k != r and k.startswith(r + "/"):
                return ERR_INVALID
        if r == "/":
            return ERR_PERMISSION
        del self.tree[r]
        return ERR_OK

    def ls(self, path=None):
        r = self._resolve(path or self.cwd)
        if r not in self.tree or self.tree[r]["type"] != "dir":
            return None, ERR_NOT_FOUND
        prefix = r if r == "/" else r + "/"
        entries = []
        for k in sorted(self.tree.keys()):
            if k == r:
                continue
            if k.startswith(prefix):
                rest = k[len(prefix):]
                if "/" not in rest:
                    entries.append((rest, self.tree[k]["type"], self.tree[k]["perms"]))
        return entries, ERR_OK

    def mv(self, src, dst):
        rs = self._resolve(src)
        rd = self._resolve(dst)
        if rs not in self.tree:
            return ERR_NOT_FOUND
        self.tree[rd] = self.tree.pop(rs)
        # Move children if dir
        if self.tree[rd]["type"] == "dir":
            to_move = [(k, k.replace(rs, rd, 1)) for k in list(self.tree.keys())
                       if k.startswith(rs + "/")]
            for old, new in to_move:
                self.tree[new] = self.tree.pop(old)
        return ERR_OK

    def cp(self, src, dst):
        rs = self._resolve(src)
        rd = self._resolve(dst)
        if rs not in self.tree:
            return ERR_NOT_FOUND
        if self.tree[rs]["type"] == "dir":
            return ERR_INVALID
        import copy
        self.tree[rd] = copy.deepcopy(self.tree[rs])
        self.tree[rd]["created"] = time.time()
        return ERR_OK

    def chmod(self, path, perms):
        r = self._resolve(path)
        if r not in self.tree:
            return ERR_NOT_FOUND
        self.tree[r]["perms"] = perms
        return ERR_OK

    def stat(self, path):
        r = self._resolve(path)
        if r not in self.tree:
            return None, ERR_NOT_FOUND
        node = self.tree[r]
        info = {"path": r, "type": node["type"], "perms": node["perms"],
                "owner": node["owner"], "created": node["created"],
                "modified": node["modified"]}
        if node["type"] == "file":
            info["size"] = len(node.get("content", ""))
        return info, ERR_OK

    def tree_view(self, path=None, prefix="", depth=0, max_depth=4):
        r = self._resolve(path or self.cwd)
        if depth > max_depth:
            return []
        lines = []
        entries, err = self.ls(r)
        if err != ERR_OK:
            return lines
        for name, typ, _ in entries:
            marker = "/" if typ == "dir" else ""
            lines.append(f"{prefix}{name}{marker}")
            if typ == "dir" and depth < max_depth:
                child = r + "/" + name if r != "/" else "/" + name
                lines.extend(self.tree_view(child, prefix + "  ", depth + 1, max_depth))
        return lines

    def cd(self, path):
        r = self._resolve(path)
        if r not in self.tree or self.tree[r]["type"] != "dir":
            return ERR_NOT_FOUND
        self.cwd = r
        return ERR_OK

    def serialize(self):
        return json.dumps({"tree": self.tree, "cwd": self.cwd})

    def deserialize(self, data):
        obj = json.loads(data)
        self.tree = obj["tree"]
        self.cwd = obj.get("cwd", "/")


# ─── Process/Task Table ───────────────────────────────────────────────────────

class ProcessTable:
    def __init__(self):
        self.processes = {}
        self.next_pid = 1
        self.tick_count = 0

    def spawn(self, name, state="running"):
        pid = self.next_pid
        self.next_pid += 1
        self.processes[pid] = {"name": name, "state": state,
                               "started": time.time(), "ticks": 0}
        return pid

    def kill(self, pid):
        if pid not in self.processes:
            return ERR_NO_PROCESS
        del self.processes[pid]
        return ERR_OK

    def tick(self):
        self.tick_count += 1
        for pid in self.processes:
            if self.processes[pid]["state"] == "running":
                self.processes[pid]["ticks"] += 1
        return self.tick_count

    def ps(self):
        return list(self.processes.items())

    def status(self, pid):
        if pid not in self.processes:
            return None, ERR_NO_PROCESS
        return self.processes[pid], ERR_OK


# ─── OZ Ledger — AI-ready deterministic command provenance and intent trace ──
# "Every command leaves a trace."

class OZLedger:
    def __init__(self):
        self.entries = []

    def record(self, command, parsed, actor="user", tick=0,
               fs_mutations=None, mem_mutations=None, proc_mutations=None,
               mod_mutations=None, result_code=0, explanation="", reversible=False):
        entry = {
            "command": command,
            "parsed": parsed,
            "actor": actor,
            "tick": tick,
            "timestamp": time.time(),
            "fs_mutations": fs_mutations or [],
            "mem_mutations": mem_mutations or [],
            "proc_mutations": proc_mutations or [],
            "mod_mutations": mod_mutations or [],
            "result_code": result_code,
            "explanation": explanation,
            "reversible": reversible,
        }
        self.entries.append(entry)
        return entry

    def trace(self, n=10):
        return self.entries[-n:]

    def replay_info(self):
        return [{"tick": e["tick"], "command": e["command"],
                 "result": e["result_code"]} for e in self.entries]

    def size(self):
        return len(self.entries)


# ─── Module/Slot/Overhead System ─────────────────────────────────────────────
# "A slot is a promise with a boundary."
# "OZ begins where extension becomes accountable."

BUILTIN_SLOTS = [
    "shell.commands", "fs.drivers", "devices", "ai.hooks",
    "sched.hooks", "boot.hooks", "diagnostics", "experiments",
    "oz.contracts", "oz.ledger"
]


class OZLayer:
    def __init__(self):
        self.slots = {s: [] for s in BUILTIN_SLOTS}
        self.modules = {}
        self.overheads = {}
        self.hooks = {"boot": [], "sched": [], "ai": [], "diag": [], "experiment": []}
        self.contracts = {}

    def register_slot(self, name):
        if name not in self.slots:
            self.slots[name] = []

    def register_module(self, name, module):
        self.modules[name] = module
        overhead = {"dispatch_entries": 0, "hooks": 0, "slots_occupied": []}
        if hasattr(module, "commands"):
            overhead["dispatch_entries"] = len(module.commands)
        if hasattr(module, "slots"):
            for s in module.slots:
                if s in self.slots:
                    self.slots[s].append(name)
                    overhead["slots_occupied"].append(s)
        if hasattr(module, "hooks"):
            for h in module.hooks:
                if h in self.hooks:
                    self.hooks[h].append(name)
                    overhead["hooks"] += 1
        if hasattr(module, "contract"):
            self.contracts[name] = module.contract
        self.overheads[name] = overhead
        if hasattr(module, "init"):
            module.init()
        return ERR_OK

    def unregister_module(self, name):
        if name not in self.modules:
            return ERR_NOT_FOUND
        mod = self.modules[name]
        if hasattr(mod, "shutdown"):
            mod.shutdown()
        for s in self.slots:
            if name in self.slots[s]:
                self.slots[s].remove(name)
        for h in self.hooks:
            if name in self.hooks[h]:
                self.hooks[h].remove(name)
        if name in self.contracts:
            del self.contracts[name]
        del self.modules[name]
        del self.overheads[name]
        return ERR_OK

    def total_overhead(self):
        total = {"dispatch_entries": 0, "hooks": 0, "slots_occupied": 0, "modules": len(self.modules)}
        for name, ov in self.overheads.items():
            total["dispatch_entries"] += ov["dispatch_entries"]
            total["hooks"] += ov["hooks"]
            total["slots_occupied"] += len(ov["slots_occupied"])
        return total

    def call_module(self, mod_name, method, *args):
        if mod_name not in self.modules:
            return None, ERR_NOT_FOUND
        mod = self.modules[mod_name]
        if not hasattr(mod, method):
            return None, ERR_INVALID
        result = getattr(mod, method)(*args)
        return result, ERR_OK


# ─── Built-in Modules ────────────────────────────────────────────────────────

class CoreutilsModule:
    name = "coreutils"
    description = "Core utilities module"
    commands = ["uptime"]
    slots = ["shell.commands"]
    hooks = []
    contract = {"provides": ["uptime"], "requires": [], "version": "0.1"}

    def init(self):
        self.start = time.time()

    def shutdown(self):
        pass

    def uptime(self):
        return f"System up for {int(time.time() - self.start)} seconds"


class HWProbeModule:
    name = "hwprobe"
    description = "Hardware probe module"
    commands = ["hwinfo"]
    slots = ["diagnostics"]
    hooks = ["boot"]
    contract = {"provides": ["hwinfo"], "requires": [], "version": "0.1"}

    def init(self):
        pass

    def shutdown(self):
        pass

    def hwinfo(self, hw):
        return hw


class DiagModule:
    name = "diag"
    description = "Diagnostics module"
    commands = ["diag_status"]
    slots = ["diagnostics"]
    hooks = ["diag"]
    contract = {"provides": ["diag_status"], "requires": [], "version": "0.1"}

    def init(self):
        pass

    def shutdown(self):
        pass

    def diag_status(self):
        return "diagnostics: nominal"


class AISeedModule:
    name = "ai_seed"
    description = "AI seed hooks module"
    commands = ["ai_status"]
    slots = ["ai.hooks"]
    hooks = ["ai"]
    contract = {"provides": ["ai_status", "intent_hints"], "requires": ["oz.ledger"], "version": "0.1"}

    def init(self):
        pass

    def shutdown(self):
        pass

    def ai_status(self):
        return "ai_seed: active, no external model loaded"


class OZLedgerModule:
    name = "oz_ledger"
    description = "OZ Ledger provenance tracking module"
    commands = ["ledger_size"]
    slots = ["oz.ledger", "oz.contracts"]
    hooks = ["ai"]
    contract = {"provides": ["ledger_size", "trace", "replay"], "requires": [], "version": "0.1"}

    def init(self):
        pass

    def shutdown(self):
        pass

    def ledger_size(self, ledger):
        return ledger.size()


class ExampleExtModule:
    """Example extension module — adds 'fortune' command."""
    name = "fortune_ext"
    description = "Example extension: fortune cookies"
    commands = ["fortune"]
    slots = ["shell.commands", "experiments"]
    hooks = ["experiment"]
    contract = {"provides": ["fortune"], "requires": [], "version": "0.1"}

    def init(self):
        self.fortunes = [
            "The system is the territory.",
            "Every overhead has a name.",
            "A module is a guest with manners.",
            "Slots are finite; ambition is not.",
            "Trace everything. Regret nothing.",
        ]
        self.idx = 0

    def shutdown(self):
        pass

    def fortune(self):
        f = self.fortunes[self.idx % len(self.fortunes)]
        self.idx += 1
        return f


# ─── Kernel ──────────────────────────────────────────────────────────────────
# "The shell is the first weather of the system."

class AmosOZKernel:
    def __init__(self):
        self.name = SYSTEM_NAME
        self.version = VERSION
        self.boot_time = time.time()
        self.hw = detect_hardware()
        self.mem = VirtualMemory(65536)
        self.fs = VirtualFS()
        self.procs = ProcessTable()
        self.oz = OZLayer()
        self.ledger = OZLedger()
        self.env = {
            "HOME": "/home/user",
            "USER": "user",
            "SHELL": "/bin/amossh",
            "PATH": "/bin:/usr/bin",
            "TERM": self.hw.get("terminal", "vt100"),
            "HOSTNAME": self.hw.get("hostname", "amosoz"),
        }
        self.user = "user"
        self.session_id = hashlib.md5(str(time.time()).encode()).hexdigest()[:8]
        self.history = []
        self.devices = [
            {"name": "console", "type": "char", "status": "active"},
            {"name": "mem", "type": "block", "status": "active"},
            {"name": "null", "type": "char", "status": "active"},
            {"name": "zero", "type": "char", "status": "active"},
        ]
        self.running = True
        self._init_modules()
        # Boot process
        self.procs.spawn("init", "running")
        self.procs.spawn("amossh", "running")

    def _init_modules(self):
        self.oz.register_module("coreutils", CoreutilsModule())
        self.oz.register_module("hwprobe", HWProbeModule())
        self.oz.register_module("diag", DiagModule())
        self.oz.register_module("ai_seed", AISeedModule())
        self.oz.register_module("oz_ledger", OZLedgerModule())
        self.oz.register_module("fortune_ext", ExampleExtModule())

    # ─── Syscall-like interface ──────────────────────────────────────────────
    def syscall(self, name, *args):
        dispatch = {
            "read": lambda p: self.fs.read(p),
            "write": lambda p, c: self.fs.write(p, c),
            "open": lambda p: self.fs.touch(p),
            "unlink": lambda p: self.fs.rm(p),
            "mkdir": lambda p: self.fs.mkdir(p),
            "rmdir": lambda p: self.fs.rmdir(p),
            "stat": lambda p: self.fs.stat(p),
            "chdir": lambda p: self.fs.cd(p),
            "getcwd": lambda: (self.fs.cwd, ERR_OK),
            "alloc": lambda s: self.mem.alloc(s),
            "free": lambda b: (self.mem.free(b), ERR_OK),
            "spawn": lambda n: (self.procs.spawn(n), ERR_OK),
            "kill": lambda p: (self.procs.kill(p), ERR_OK),
            "getenv": lambda k: (self.env.get(k, ""), ERR_OK),
            "setenv": lambda k, v: (self.env.__setitem__(k, v), ERR_OK)[1:],
        }
        if name not in dispatch:
            return None, ERR_NOT_FOUND
        try:
            return dispatch[name](*args)
        except Exception as e:
            return str(e), ERR_INVALID

    # ─── Command Dispatcher ──────────────────────────────────────────────────
    def dispatch(self, line):
        line = line.strip()
        if not line:
            return ""
        self.history.append(line)
        parts = line.split()
        cmd = parts[0]
        args = parts[1:]

        # Record in ledger before execution
        pre_tick = self.procs.tick_count

        result = self._exec(cmd, args)

        # Record in ledger after execution
        self.ledger.record(
            command=line, parsed={"cmd": cmd, "args": args},
            actor=self.user, tick=self.procs.tick_count,
            explanation=f"Executed: {cmd}",
            result_code=ERR_OK if result is not None else ERR_NOT_FOUND
        )
        return result

    def _exec(self, cmd, args):
        commands = {
            "help": self._cmd_help,
            "clear": self._cmd_clear,
            "uname": self._cmd_uname,
            "version": self._cmd_version,
            "boot": self._cmd_boot,
            "hw": self._cmd_hw,
            "devices": self._cmd_devices,
            "gpu": self._cmd_gpu,
            "mem": self._cmd_mem,
            "mmap": self._cmd_mmap,
            "alloc": self._cmd_alloc,
            "free": self._cmd_free,
            "ps": self._cmd_ps,
            "run": self._cmd_run,
            "kill": self._cmd_kill,
            "tick": self._cmd_tick,
            "status": self._cmd_status,
            "pwd": self._cmd_pwd,
            "cd": self._cmd_cd,
            "ls": self._cmd_ls,
            "cat": self._cmd_cat,
            "touch": self._cmd_touch,
            "write": self._cmd_write,
            "append": self._cmd_append,
            "rm": self._cmd_rm,
            "mkdir": self._cmd_mkdir,
            "rmdir": self._cmd_rmdir,
            "mv": self._cmd_mv,
            "cp": self._cmd_cp,
            "chmod": self._cmd_chmod,
            "stat": self._cmd_stat,
            "tree": self._cmd_tree,
            "echo": self._cmd_echo,
            "env": self._cmd_env,
            "set": self._cmd_set,
            "unset": self._cmd_unset,
            "date": self._cmd_date,
            "history": self._cmd_history,
            "selftest": self._cmd_selftest,
            "oz": self._cmd_oz,
            "slots": self._cmd_slots,
            "modules": self._cmd_modules,
            "overhead": self._cmd_overhead,
            "hooks": self._cmd_hooks,
            "contracts": self._cmd_contracts,
            "loadmod": self._cmd_loadmod,
            "unloadmod": self._cmd_unloadmod,
            "call": self._cmd_call,
            "trace": self._cmd_trace,
            "replay": self._cmd_replay,
            "reset": self._cmd_reset,
            "save": self._cmd_save,
            "load": self._cmd_load,
            "exit": self._cmd_exit,
            "fortune": self._cmd_fortune,
        }
        if cmd in commands:
            return commands[cmd](args)
        # Check module commands
        for mod_name, mod in self.oz.modules.items():
            if hasattr(mod, "commands") and cmd in mod.commands:
                if hasattr(mod, cmd):
                    method = getattr(mod, cmd)
                    try:
                        return str(method())
                    except TypeError:
                        return str(method(self))
        return f"amosoz: command not found: {cmd}"

    # ─── Command Implementations ─────────────────────────────────────────────

    def _cmd_help(self, args):
        cmds = sorted([
            "help", "clear", "uname", "version", "boot", "hw", "devices", "gpu",
            "mem", "mmap", "alloc", "free", "ps", "run", "kill", "tick", "status",
            "pwd", "cd", "ls", "cat", "touch", "write", "append", "rm", "mkdir",
            "rmdir", "mv", "cp", "chmod", "stat", "tree", "echo", "env", "set",
            "unset", "date", "history", "selftest", "oz", "slots", "modules",
            "overhead", "hooks", "contracts", "loadmod", "unloadmod", "call",
            "trace", "replay", "reset", "save", "load", "exit", "fortune"
        ])
        return "amosOZ commands:\n  " + "  ".join(cmds)

    def _cmd_clear(self, args):
        return "\033[2J\033[H"

    def _cmd_uname(self, args):
        return f"{self.name} {self.version} {self.hw['machine']} {self.hw['platform']}"

    def _cmd_version(self, args):
        return f"{self.name} version {self.version} (build {BUILD_DATE})"

    def _cmd_boot(self, args):
        elapsed = int(time.time() - self.boot_time)
        return f"Boot time: {time.ctime(self.boot_time)}\nUptime: {elapsed}s"

    def _cmd_hw(self, args):
        lines = ["Hardware Profile:"]
        for k, v in self.hw.items():
            lines.append(f"  {k}: {v}")
        return "\n".join(lines)

    def _cmd_devices(self, args):
        lines = ["Devices:"]
        for d in self.devices:
            lines.append(f"  {d['name']:12} {d['type']:8} {d['status']}")
        return "\n".join(lines)

    def _cmd_gpu(self, args):
        return "GPU: not available (Python runtime, no GPU abstraction)"

    def _cmd_mem(self, args):
        s = self.mem.stats()
        return (f"Memory: {s['total_kb']} KB total, {s['used_kb']} KB used, "
                f"{s['free_kb']} KB free, {s['blocks']} blocks")

    def _cmd_mmap(self, args):
        blocks = self.mem.mmap()
        if not blocks:
            return "No memory blocks allocated."
        lines = ["ID    Size(KB)  Flags  Owner"]
        for bid, info in blocks:
            lines.append(f"{bid:<5} {info['size']:<9} {info['flags']:<6} {info['owner']}")
        return "\n".join(lines)

    def _cmd_alloc(self, args):
        if not args:
            return "Usage: alloc <size_kb>"
        try:
            size = int(args[0])
        except ValueError:
            return "Error: size must be integer"
        bid, err = self.mem.alloc(size, self.user)
        if err != ERR_OK:
            return "Error: out of memory"
        return f"Allocated block {bid} ({size} KB)"

    def _cmd_free(self, args):
        if not args:
            return "Usage: free <block_id>"
        try:
            bid = int(args[0])
        except ValueError:
            return "Error: block_id must be integer"
        err = self.mem.free(bid)
        if err != ERR_OK:
            return f"Error: block {bid} not found"
        return f"Freed block {bid}"

    def _cmd_ps(self, args):
        procs = self.procs.ps()
        if not procs:
            return "No processes."
        lines = ["PID   Name          State     Ticks"]
        for pid, info in procs:
            lines.append(f"{pid:<5} {info['name']:<13} {info['state']:<9} {info['ticks']}")
        return "\n".join(lines)

    def _cmd_run(self, args):
        if not args:
            return "Usage: run <name>"
        name = args[0]
        pid = self.procs.spawn(name)
        return f"Started process '{name}' with PID {pid}"

    def _cmd_kill(self, args):
        if not args:
            return "Usage: kill <pid>"
        try:
            pid = int(args[0])
        except ValueError:
            return "Error: pid must be integer"
        err = self.procs.kill(pid)
        if err != ERR_OK:
            return f"Error: process {pid} not found"
        return f"Killed process {pid}"

    def _cmd_tick(self, args):
        t = self.procs.tick()
        return f"Tick: {t}"

    def _cmd_status(self, args):
        if not args:
            return "Usage: status <pid>"
        try:
            pid = int(args[0])
        except ValueError:
            return "Error: pid must be integer"
        info, err = self.procs.status(pid)
        if err != ERR_OK:
            return f"Error: process {pid} not found"
        return f"PID {pid}: {info['name']} state={info['state']} ticks={info['ticks']}"

    def _cmd_pwd(self, args):
        return self.fs.cwd

    def _cmd_cd(self, args):
        path = args[0] if args else self.env.get("HOME", "/")
        err = self.fs.cd(path)
        if err != ERR_OK:
            return f"cd: no such directory: {path}"
        return ""

    def _cmd_ls(self, args):
        path = args[0] if args else None
        entries, err = self.fs.ls(path)
        if err != ERR_OK:
            return "ls: no such directory"
        if not entries:
            return ""
        lines = []
        for name, typ, perms in entries:
            marker = "/" if typ == "dir" else ""
            lines.append(f"{perms} {name}{marker}")
        return "\n".join(lines)

    def _cmd_cat(self, args):
        if not args:
            return "Usage: cat <file>"
        content, err = self.fs.read(args[0])
        if err != ERR_OK:
            return f"cat: {args[0]}: not found or not a file"
        return content

    def _cmd_touch(self, args):
        if not args:
            return "Usage: touch <file>"
        err = self.fs.touch(args[0])
        if err != ERR_OK:
            return f"touch: cannot create {args[0]}"
        return ""

    def _cmd_write(self, args):
        if len(args) < 2:
            return "Usage: write <file> <content...>"
        path = args[0]
        content = " ".join(args[1:])
        err = self.fs.write(path, content)
        if err != ERR_OK:
            return f"write: error writing to {path}"
        return ""

    def _cmd_append(self, args):
        if len(args) < 2:
            return "Usage: append <file> <content...>"
        path = args[0]
        content = " ".join(args[1:])
        err = self.fs.append(path, content)
        if err != ERR_OK:
            return f"append: error: {path} not found"
        return ""

    def _cmd_rm(self, args):
        if not args:
            return "Usage: rm <file>"
        err = self.fs.rm(args[0])
        if err != ERR_OK:
            return f"rm: cannot remove {args[0]}"
        return ""

    def _cmd_mkdir(self, args):
        if not args:
            return "Usage: mkdir <dir>"
        err = self.fs.mkdir(args[0])
        if err != ERR_OK:
            return f"mkdir: cannot create {args[0]}"
        return ""

    def _cmd_rmdir(self, args):
        if not args:
            return "Usage: rmdir <dir>"
        err = self.fs.rmdir(args[0])
        if err != ERR_OK:
            return f"rmdir: cannot remove {args[0]}"
        return ""

    def _cmd_mv(self, args):
        if len(args) < 2:
            return "Usage: mv <src> <dst>"
        err = self.fs.mv(args[0], args[1])
        if err != ERR_OK:
            return f"mv: error"
        return ""

    def _cmd_cp(self, args):
        if len(args) < 2:
            return "Usage: cp <src> <dst>"
        err = self.fs.cp(args[0], args[1])
        if err != ERR_OK:
            return f"cp: error"
        return ""

    def _cmd_chmod(self, args):
        if len(args) < 2:
            return "Usage: chmod <perms> <path>"
        err = self.fs.chmod(args[1], args[0])
        if err != ERR_OK:
            return f"chmod: error"
        return ""

    def _cmd_stat(self, args):
        if not args:
            return "Usage: stat <path>"
        info, err = self.fs.stat(args[0])
        if err != ERR_OK:
            return f"stat: {args[0]} not found"
        lines = [f"  {k}: {v}" for k, v in info.items()]
        return "\n".join(lines)

    def _cmd_tree(self, args):
        path = args[0] if args else None
        lines = self.fs.tree_view(path)
        if not lines:
            return "(empty)"
        return "\n".join(lines)

    def _cmd_echo(self, args):
        return " ".join(args)

    def _cmd_env(self, args):
        lines = [f"{k}={v}" for k, v in sorted(self.env.items())]
        return "\n".join(lines)

    def _cmd_set(self, args):
        if len(args) < 2:
            return "Usage: set <key> <value>"
        self.env[args[0]] = " ".join(args[1:])
        return ""

    def _cmd_unset(self, args):
        if not args:
            return "Usage: unset <key>"
        self.env.pop(args[0], None)
        return ""

    def _cmd_date(self, args):
        return time.strftime("%Y-%m-%d %H:%M:%S %Z")

    def _cmd_history(self, args):
        if not self.history:
            return "(no history)"
        return "\n".join(f"{i+1}: {h}" for i, h in enumerate(self.history[-20:]))

    def _cmd_oz(self, args):
        lines = ["OZ Layer — Extensibility Field"]
        lines.append(f"  Modules loaded: {len(self.oz.modules)}")
        lines.append(f"  Slots defined: {len(self.oz.slots)}")
        lines.append(f"  Hooks active: {sum(len(v) for v in self.oz.hooks.values())}")
        lines.append(f"  Contracts: {len(self.oz.contracts)}")
        lines.append(f"  Ledger entries: {self.ledger.size()}")
        ov = self.oz.total_overhead()
        lines.append(f"  Total overhead: {ov['dispatch_entries']} dispatch, {ov['hooks']} hooks, {ov['slots_occupied']} slot occupations")
        return "\n".join(lines)

    def _cmd_slots(self, args):
        lines = ["Slots:"]
        for name, occupants in sorted(self.oz.slots.items()):
            occ = ", ".join(occupants) if occupants else "(empty)"
            lines.append(f"  {name}: {occ}")
        return "\n".join(lines)

    def _cmd_modules(self, args):
        lines = ["Modules:"]
        for name, mod in sorted(self.oz.modules.items()):
            desc = getattr(mod, "description", "")
            cmds = ", ".join(getattr(mod, "commands", []))
            lines.append(f"  {name}: {desc} [commands: {cmds}]")
        return "\n".join(lines)

    def _cmd_overhead(self, args):
        lines = ["Overhead Accounting:"]
        total = self.oz.total_overhead()
        lines.append(f"  Total modules: {total['modules']}")
        lines.append(f"  Total dispatch entries: {total['dispatch_entries']}")
        lines.append(f"  Total hooks: {total['hooks']}")
        lines.append(f"  Total slot occupations: {total['slots_occupied']}")
        lines.append("")
        lines.append("Per-module:")
        for name, ov in sorted(self.oz.overheads.items()):
            lines.append(f"  {name}: dispatch={ov['dispatch_entries']} hooks={ov['hooks']} slots={ov['slots_occupied']}")
        return "\n".join(lines)

    def _cmd_hooks(self, args):
        lines = ["Hooks:"]
        for name, subscribers in sorted(self.oz.hooks.items()):
            subs = ", ".join(subscribers) if subscribers else "(none)"
            lines.append(f"  {name}: {subs}")
        return "\n".join(lines)

    def _cmd_contracts(self, args):
        lines = ["Module Contracts:"]
        for name, contract in sorted(self.oz.contracts.items()):
            lines.append(f"  {name}:")
            lines.append(f"    provides: {contract.get('provides', [])}")
            lines.append(f"    requires: {contract.get('requires', [])}")
            lines.append(f"    version: {contract.get('version', '?')}")
        return "\n".join(lines)

    def _cmd_loadmod(self, args):
        if not args:
            return "Usage: loadmod <module_name>\nAvailable for reload: fortune_ext"
        return f"loadmod: dynamic loading not supported in this version (modules are compiled-in)"

    def _cmd_unloadmod(self, args):
        if not args:
            return "Usage: unloadmod <module_name>"
        err = self.oz.unregister_module(args[0])
        if err != ERR_OK:
            return f"unloadmod: module '{args[0]}' not found"
        return f"Unloaded module '{args[0]}'"

    def _cmd_call(self, args):
        if len(args) < 2:
            return "Usage: call <module> <method> [args...]"
        mod_name = args[0]
        method = args[1]
        result, err = self.oz.call_module(mod_name, method)
        if err != ERR_OK:
            return f"call: error calling {mod_name}.{method}"
        return str(result)

    def _cmd_trace(self, args):
        n = int(args[0]) if args else 10
        entries = self.ledger.trace(n)
        if not entries:
            return "(no trace entries)"
        lines = ["OZ Ledger Trace:"]
        for e in entries:
            lines.append(f"  [{e['tick']}] {e['command']} -> {e['result_code']} ({e['explanation']})")
        return "\n".join(lines)

    def _cmd_replay(self, args):
        entries = self.ledger.replay_info()
        if not entries:
            return "(no replay data)"
        lines = ["Replay Log:"]
        for e in entries:
            lines.append(f"  tick={e['tick']} cmd={e['command']} result={e['result']}")
        return "\n".join(lines)

    def _cmd_reset(self, args):
        self.__init__()
        return "System reset complete."

    def _cmd_save(self, args):
        fname = args[0] if args else "amosoz.img"
        try:
            data = {
                "fs": json.loads(self.fs.serialize()),
                "env": self.env,
                "version": self.version,
            }
            with open(fname, "w") as f:
                json.dump(data, f)
            return f"System image saved to {fname}"
        except Exception as e:
            return f"save: error: {e}"

    def _cmd_load(self, args):
        fname = args[0] if args else "amosoz.img"
        try:
            with open(fname, "r") as f:
                data = json.load(f)
            self.fs.deserialize(json.dumps(data["fs"]))
            self.env.update(data.get("env", {}))
            return f"System image loaded from {fname}"
        except FileNotFoundError:
            return f"load: {fname} not found"
        except Exception as e:
            return f"load: error: {e}"

    def _cmd_exit(self, args):
        self.running = False
        return "Shutting down amosOZ."

    def _cmd_fortune(self, args):
        if "fortune_ext" in self.oz.modules:
            return self.oz.modules["fortune_ext"].fortune()
        return "fortune: module not loaded"

    # ─── Selftest ────────────────────────────────────────────────────────────
    def _cmd_selftest(self, args):
        results = []

        def check(name, condition):
            status = "PASS" if condition else "FAIL"
            results.append(f"  [{status}] {name}")
            return condition

        check("boot_state", self.boot_time > 0)
        check("hw_profile", self.hw is not None and "platform" in self.hw)
        check("version", self.version == VERSION)

        # Filesystem
        self.fs.write("/tmp/selftest_file", "hello")
        content, err = self.fs.read("/tmp/selftest_file")
        check("fs_write_read", content == "hello" and err == ERR_OK)
        self.fs.append("/tmp/selftest_file", " world")
        content, _ = self.fs.read("/tmp/selftest_file")
        check("fs_append", content == "hello world")
        err = self.fs.rm("/tmp/selftest_file")
        check("fs_delete", err == ERR_OK)
        check("fs_deleted", not self.fs.exists("/tmp/selftest_file"))

        # Path resolution
        self.fs.mkdir("/tmp/st_dir")
        check("fs_mkdir", self.fs.exists("/tmp/st_dir"))
        self.fs.rmdir("/tmp/st_dir")
        check("fs_rmdir", not self.fs.exists("/tmp/st_dir"))

        # Permissions
        self.fs.write("/tmp/ptest", "data")
        self.fs.chmod("/tmp/ptest", "r--------")
        info, _ = self.fs.stat("/tmp/ptest")
        check("permission_mutation", info["perms"] == "r--------")
        self.fs.rm("/tmp/ptest")

        # Memory
        bid, err = self.mem.alloc(100, "selftest")
        check("mem_alloc", bid is not None and err == ERR_OK)
        err = self.mem.free(bid)
        check("mem_free", err == ERR_OK)

        # Command dispatch
        r = self.dispatch("echo selftest_token")
        check("command_dispatch", "selftest_token" in r)

        # Process table
        pid = self.procs.spawn("test_proc")
        check("process_spawn", pid is not None)
        self.procs.kill(pid)
        check("process_kill", pid not in self.procs.processes)

        # Module registration
        check("module_registered", "coreutils" in self.oz.modules)

        # Slot occupation
        check("slot_occupation", len(self.oz.slots["shell.commands"]) > 0)

        # Overhead accounting
        ov = self.oz.total_overhead()
        check("overhead_accounting", ov["modules"] > 0)

        # OZ layer visibility
        check("oz_layer", len(self.oz.slots) == len(BUILTIN_SLOTS))

        # Ledger
        check("oz_ledger", self.ledger.size() > 0)

        # Summary
        passed = sum(1 for r in results if "[PASS]" in r)
        total = len(results)
        results.insert(0, f"amosOZ Selftest ({passed}/{total} passed):")
        if passed == total:
            results.append(f"\n  ALL TESTS PASSED")
        else:
            results.append(f"\n  {total - passed} TESTS FAILED")
        return "\n".join(results)


# ─── Main Loop ───────────────────────────────────────────────────────────────

def main():
    kernel = AmosOZKernel()
    print(f"{SYSTEM_NAME} v{VERSION}")
    print(f"Type 'help' for commands, 'selftest' to verify, 'exit' to quit.\n")

    while kernel.running:
        try:
            prompt = f"{kernel.user}@amosoz:{kernel.fs.cwd}$ "
            line = input(prompt)
            result = kernel.dispatch(line)
            if result:
                print(result)
        except (EOFError, KeyboardInterrupt):
            print("\nShutting down amosOZ.")
            break

if __name__ == "__main__":
    main()

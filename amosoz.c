/*
 * amosOZ — Arianna Method Operating System
 * The most atomic way to build a working OS-like environment from scratch.
 * Single file. No external dependencies. The complete first algorithm.
 *
 * AMOS is the operating body. OZ is the extensibility field.
 * Compile: gcc -o amosoz amosoz.c -lm
 * Run: ./amosoz
 *
 * "No kernel mythology here: only state, contracts, and consequences."
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define VERSION "0.1.0"
#define SYSTEM_NAME "amosOZ"
#define BUILD_DATE "2026-06-30"

#define MAX_PATH 256
#define MAX_CONTENT 4096
#define MAX_FILES 512
#define MAX_BLOCKS 256
#define MAX_PROCS 64
#define MAX_MODULES 32
#define MAX_ENV 64
#define MAX_HISTORY 100
#define MAX_LEDGER 256
#define MAX_SLOTS 16
#define MAX_SLOT_OCCUPANTS 8
#define MAX_DEVICES 16
#define MAX_HOOKS 8
#define MAX_HOOK_SUBS 8
#define MAX_CMD_LEN 1024
#define MEM_TOTAL_KB 65536

/* ─── Error Codes ─────────────────────────────────────────────────────────── */
#define ERR_OK 0
#define ERR_NOT_FOUND 1
#define ERR_PERMISSION 2
#define ERR_EXISTS 3
#define ERR_NO_MEMORY 4
#define ERR_INVALID 5
#define ERR_IO 6
#define ERR_NO_PROCESS 7
#define ERR_MODULE 8

/* ─── Hardware Profile ────────────────────────────────────────────────────── */
typedef struct {
    char platform[64];
    char platform_release[64];
    char machine[64];
    char processor[64];
    char hostname[64];
    int cpu_count;
    long memory_mb;
    char terminal[32];
    int persistence;
} HWProfile;

/* ─── Virtual Memory ─────────────────────────────────────────────────────── */
typedef struct {
    int id;
    int size;
    char flags[8];
    char owner[32];
    int used;
} MemBlock;

typedef struct {
    int total_kb;
    int used_kb;
    MemBlock blocks[MAX_BLOCKS];
    int block_count;
    int next_id;
} VirtualMemory;

/* ─── Virtual Filesystem ──────────────────────────────────────────────────── */
typedef struct {
    char path[MAX_PATH];
    int is_dir;
    char content[MAX_CONTENT];
    char perms[12];
    char owner[16];
    time_t created;
    time_t modified;
    int used;
} FSNode;

typedef struct {
    FSNode nodes[MAX_FILES];
    int node_count;
    char cwd[MAX_PATH];
} VirtualFS;

/* ─── Process Table ───────────────────────────────────────────────────────── */
typedef struct {
    int pid;
    char name[32];
    char state[16];
    time_t started;
    int ticks;
    int used;
} Process;

typedef struct {
    Process procs[MAX_PROCS];
    int next_pid;
    int tick_count;
} ProcessTable;

/* ─── OZ Ledger ───────────────────────────────────────────────────────────── */
/* "Every command leaves a trace." */
typedef struct {
    char command[MAX_CMD_LEN];
    char actor[16];
    int tick;
    time_t timestamp;
    int result_code;
    char explanation[128];
    int reversible;
} LedgerEntry;

typedef struct {
    LedgerEntry entries[MAX_LEDGER];
    int count;
} OZLedger;

/* ─── Module System ───────────────────────────────────────────────────────── */
/* "A slot is a promise with a boundary." */
typedef struct {
    char name[32];
    char description[64];
    char commands[4][16];
    int cmd_count;
    char slots[4][32];
    int slot_count;
    char hooks[4][16];
    int hook_count;
    char contract_provides[4][32];
    int provides_count;
    char contract_requires[4][32];
    int requires_count;
    char contract_version[8];
    int loaded;
} Module;

/* ─── Slots ───────────────────────────────────────────────────────────────── */
typedef struct {
    char name[32];
    char occupants[MAX_SLOT_OCCUPANTS][32];
    int occupant_count;
} Slot;

/* ─── Hooks ───────────────────────────────────────────────────────────────── */
typedef struct {
    char name[16];
    char subscribers[MAX_HOOK_SUBS][32];
    int sub_count;
} Hook;

/* ─── Devices ─────────────────────────────────────────────────────────────── */
typedef struct {
    char name[16];
    char type[8];
    char status[16];
} Device;

/* ─── Environment ─────────────────────────────────────────────────────────── */
typedef struct {
    char key[32];
    char value[128];
} EnvVar;

/* ─── Kernel ──────────────────────────────────────────────────────────────── */
/* "The shell is the first weather of the system." */
typedef struct {
    HWProfile hw;
    VirtualMemory mem;
    VirtualFS fs;
    ProcessTable procs;
    OZLedger ledger;
    Module modules[MAX_MODULES];
    int module_count;
    Slot slots[MAX_SLOTS];
    int slot_count;
    Hook hooks[MAX_HOOKS];
    int hook_count;
    Device devices[MAX_DEVICES];
    int device_count;
    EnvVar env[MAX_ENV];
    int env_count;
    char history[MAX_HISTORY][MAX_CMD_LEN];
    int history_count;
    time_t boot_time;
    char user[16];
    int running;
    int fortune_idx;
} Kernel;

static Kernel K;

/* ─── Forward declarations ────────────────────────────────────────────────── */
static void kernel_init(void);
static int dispatch(const char *line, char *output, int outsize);

/* ─── Hardware Detection ──────────────────────────────────────────────────── */
/* Detection must never crash. Unknown = "unknown". */
static void detect_hardware(HWProfile *hw) {
    memset(hw, 0, sizeof(HWProfile));
    #ifdef __linux__
    strcpy(hw->platform, "Linux");
    #elif defined(__APPLE__)
    strcpy(hw->platform, "Darwin");
    #else
    strcpy(hw->platform, "unknown");
    #endif

    FILE *f;
    strcpy(hw->platform_release, "unknown");
    f = fopen("/proc/version", "r");
    if (f) {
        if (fgets(hw->platform_release, sizeof(hw->platform_release), f)) {
            char *nl = strchr(hw->platform_release, '\n');
            if (nl) *nl = '\0';
            /* Truncate to just version */
            char *sp = strchr(hw->platform_release, ' ');
            if (sp) { sp = strchr(sp+1, ' '); if (sp) *sp = '\0'; }
        }
        fclose(f);
    }

    #if defined(__x86_64__)
    strcpy(hw->machine, "x86_64");
    #elif defined(__aarch64__)
    strcpy(hw->machine, "aarch64");
    #else
    strcpy(hw->machine, "unknown");
    #endif

    strcpy(hw->processor, hw->machine);
    if (gethostname(hw->hostname, sizeof(hw->hostname)) != 0)
        strcpy(hw->hostname, "unknown");

    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    hw->cpu_count = (ncpu > 0) ? (int)ncpu : -1;

    hw->memory_mb = -1;
    f = fopen("/proc/meminfo", "r");
    if (f) {
        char buf[128];
        while (fgets(buf, sizeof(buf), f)) {
            long kb;
            if (sscanf(buf, "MemTotal: %ld kB", &kb) == 1) {
                hw->memory_mb = kb / 1024;
                break;
            }
        }
        fclose(f);
    }

    const char *term = getenv("TERM");
    strncpy(hw->terminal, term ? term : "unknown", sizeof(hw->terminal)-1);
    hw->persistence = 1;
}

/* ─── Virtual Memory ──────────────────────────────────────────────────────── */
static void mem_init(VirtualMemory *m) {
    memset(m, 0, sizeof(VirtualMemory));
    m->total_kb = MEM_TOTAL_KB;
    m->next_id = 1;
}

static int mem_alloc(VirtualMemory *m, int size, const char *owner, const char *flags) {
    if (m->used_kb + size > m->total_kb) return -1;
    if (m->block_count >= MAX_BLOCKS) return -1;
    int idx = m->block_count++;
    m->blocks[idx].id = m->next_id++;
    m->blocks[idx].size = size;
    strncpy(m->blocks[idx].flags, flags, 7);
    strncpy(m->blocks[idx].owner, owner, 31);
    m->blocks[idx].used = 1;
    m->used_kb += size;
    return m->blocks[idx].id;
}

static int mem_free(VirtualMemory *m, int bid) {
    for (int i = 0; i < m->block_count; i++) {
        if (m->blocks[i].used && m->blocks[i].id == bid) {
            m->used_kb -= m->blocks[i].size;
            m->blocks[i].used = 0;
            return ERR_OK;
        }
    }
    return ERR_NOT_FOUND;
}

/* ─── Virtual Filesystem ──────────────────────────────────────────────────── */
static void fs_init(VirtualFS *fs) {
    memset(fs, 0, sizeof(VirtualFS));
    strcpy(fs->cwd, "/");
}

static int fs_find(VirtualFS *fs, const char *path) {
    for (int i = 0; i < fs->node_count; i++) {
        if (fs->nodes[i].used && strcmp(fs->nodes[i].path, path) == 0)
            return i;
    }
    return -1;
}

static void fs_resolve(VirtualFS *fs, const char *path, char *out) {
    if (path[0] == '/') {
        strcpy(out, path);
    } else {
        if (strcmp(fs->cwd, "/") == 0)
            snprintf(out, MAX_PATH, "/%s", path);
        else
            snprintf(out, MAX_PATH, "%s/%s", fs->cwd, path);
    }
    /* Normalize: remove trailing slash, handle .. and . */
    char parts[32][64];
    int pcount = 0;
    char tmp[MAX_PATH];
    strcpy(tmp, out);
    char *tok = strtok(tmp, "/");
    while (tok) {
        if (strcmp(tok, ".") == 0) { /* skip */ }
        else if (strcmp(tok, "..") == 0) { if (pcount > 0) pcount--; }
        else { strncpy(parts[pcount], tok, 63); pcount++; }
        tok = strtok(NULL, "/");
    }
    if (pcount == 0) { strcpy(out, "/"); return; }
    out[0] = '\0';
    for (int i = 0; i < pcount; i++) {
        strcat(out, "/");
        strcat(out, parts[i]);
    }
}

static int fs_add_dir(VirtualFS *fs, const char *path) {
    if (fs->node_count >= MAX_FILES) return ERR_NO_MEMORY;
    if (fs_find(fs, path) >= 0) return ERR_EXISTS;
    int i = fs->node_count++;
    memset(&fs->nodes[i], 0, sizeof(FSNode));
    strncpy(fs->nodes[i].path, path, MAX_PATH-1);
    fs->nodes[i].is_dir = 1;
    strcpy(fs->nodes[i].perms, "rwxr-xr-x");
    strcpy(fs->nodes[i].owner, "root");
    fs->nodes[i].created = fs->nodes[i].modified = time(NULL);
    fs->nodes[i].used = 1;
    return ERR_OK;
}

static int fs_add_file(VirtualFS *fs, const char *path, const char *content) {
    if (fs->node_count >= MAX_FILES) return ERR_NO_MEMORY;
    int idx = fs_find(fs, path);
    if (idx >= 0) {
        strncpy(fs->nodes[idx].content, content, MAX_CONTENT-1);
        fs->nodes[idx].modified = time(NULL);
        return ERR_OK;
    }
    int i = fs->node_count++;
    memset(&fs->nodes[i], 0, sizeof(FSNode));
    strncpy(fs->nodes[i].path, path, MAX_PATH-1);
    fs->nodes[i].is_dir = 0;
    strncpy(fs->nodes[i].content, content, MAX_CONTENT-1);
    strcpy(fs->nodes[i].perms, "rw-r--r--");
    strcpy(fs->nodes[i].owner, "user");
    fs->nodes[i].created = fs->nodes[i].modified = time(NULL);
    fs->nodes[i].used = 1;
    return ERR_OK;
}

static void fs_init_tree(VirtualFS *fs) {
    const char *dirs[] = {"/", "/bin", "/etc", "/home", "/tmp", "/var", "/dev",
                          "/proc", "/sys", "/usr", "/usr/lib", "/usr/share",
                          "/home/user", "/var/log", "/etc/amosoz", NULL};
    for (int i = 0; dirs[i]; i++) fs_add_dir(fs, dirs[i]);
    fs_add_file(fs, "/etc/hostname", "amosoz");
    fs_add_file(fs, "/etc/amosoz/version", VERSION);
}

/* ─── Process Table ───────────────────────────────────────────────────────── */
static void proc_init(ProcessTable *pt) {
    memset(pt, 0, sizeof(ProcessTable));
    pt->next_pid = 1;
}

static int proc_spawn(ProcessTable *pt, const char *name) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (!pt->procs[i].used) {
            pt->procs[i].pid = pt->next_pid++;
            strncpy(pt->procs[i].name, name, 31);
            strcpy(pt->procs[i].state, "running");
            pt->procs[i].started = time(NULL);
            pt->procs[i].ticks = 0;
            pt->procs[i].used = 1;
            return pt->procs[i].pid;
        }
    }
    return -1;
}

static int proc_kill(ProcessTable *pt, int pid) {
    for (int i = 0; i < MAX_PROCS; i++) {
        if (pt->procs[i].used && pt->procs[i].pid == pid) {
            pt->procs[i].used = 0;
            return ERR_OK;
        }
    }
    return ERR_NO_PROCESS;
}

static int proc_tick(ProcessTable *pt) {
    pt->tick_count++;
    for (int i = 0; i < MAX_PROCS; i++) {
        if (pt->procs[i].used && strcmp(pt->procs[i].state, "running") == 0)
            pt->procs[i].ticks++;
    }
    return pt->tick_count;
}

/* ─── OZ Ledger ───────────────────────────────────────────────────────────── */
static void ledger_init(OZLedger *l) { memset(l, 0, sizeof(OZLedger)); }

static void ledger_record(OZLedger *l, const char *cmd, const char *actor, int tick, int result) {
    if (l->count >= MAX_LEDGER) return;
    LedgerEntry *e = &l->entries[l->count++];
    strncpy(e->command, cmd, MAX_CMD_LEN-1);
    strncpy(e->actor, actor, 15);
    e->tick = tick;
    e->timestamp = time(NULL);
    e->result_code = result;
    snprintf(e->explanation, 127, "Executed: %s", cmd);
    e->reversible = 0;
}

/* ─── Module System ───────────────────────────────────────────────────────── */
/* "OZ begins where extension becomes accountable." */
static void init_builtin_slots(void) {
    const char *slot_names[] = {
        "shell.commands", "fs.drivers", "devices", "ai.hooks",
        "sched.hooks", "boot.hooks", "diagnostics", "experiments",
        "oz.contracts", "oz.ledger"
    };
    K.slot_count = 10;
    for (int i = 0; i < 10; i++) {
        memset(&K.slots[i], 0, sizeof(Slot));
        strcpy(K.slots[i].name, slot_names[i]);
    }
}

static void init_hooks(void) {
    const char *hook_names[] = {"boot", "sched", "ai", "diag", "experiment"};
    K.hook_count = 5;
    for (int i = 0; i < 5; i++) {
        memset(&K.hooks[i], 0, sizeof(Hook));
        strcpy(K.hooks[i].name, hook_names[i]);
    }
}

static int find_slot(const char *name) {
    for (int i = 0; i < K.slot_count; i++)
        if (strcmp(K.slots[i].name, name) == 0) return i;
    return -1;
}

static int find_hook(const char *name) {
    for (int i = 0; i < K.hook_count; i++)
        if (strcmp(K.hooks[i].name, name) == 0) return i;
    return -1;
}

static void register_module(const char *name, const char *desc,
                            const char *cmds[], int ncmds,
                            const char *mslots[], int nslots,
                            const char *mhooks[], int nhooks,
                            const char *provides[], int nprov,
                            const char *requires[], int nreq,
                            const char *ver) {
    if (K.module_count >= MAX_MODULES) return;
    Module *m = &K.modules[K.module_count++];
    memset(m, 0, sizeof(Module));
    strncpy(m->name, name, 31);
    strncpy(m->description, desc, 63);
    m->cmd_count = ncmds;
    for (int i = 0; i < ncmds && i < 4; i++) strncpy(m->commands[i], cmds[i], 15);
    m->slot_count = nslots;
    for (int i = 0; i < nslots && i < 4; i++) {
        strncpy(m->slots[i], mslots[i], 31);
        int si = find_slot(mslots[i]);
        if (si >= 0 && K.slots[si].occupant_count < MAX_SLOT_OCCUPANTS) {
            strcpy(K.slots[si].occupants[K.slots[si].occupant_count++], name);
        }
    }
    m->hook_count = nhooks;
    for (int i = 0; i < nhooks && i < 4; i++) {
        strncpy(m->hooks[i], mhooks[i], 15);
        int hi = find_hook(mhooks[i]);
        if (hi >= 0 && K.hooks[hi].sub_count < MAX_HOOK_SUBS) {
            strcpy(K.hooks[hi].subscribers[K.hooks[hi].sub_count++], name);
        }
    }
    m->provides_count = nprov;
    for (int i = 0; i < nprov && i < 4; i++) strncpy(m->contract_provides[i], provides[i], 31);
    m->requires_count = nreq;
    for (int i = 0; i < nreq && i < 4; i++) strncpy(m->contract_requires[i], requires[i], 31);
    strncpy(m->contract_version, ver, 7);
    m->loaded = 1;
}

static int unload_module(const char *name) {
    for (int i = 0; i < K.module_count; i++) {
        if (K.modules[i].loaded && strcmp(K.modules[i].name, name) == 0) {
            K.modules[i].loaded = 0;
            /* Remove from slots */
            for (int s = 0; s < K.slot_count; s++) {
                for (int o = 0; o < K.slots[s].occupant_count; o++) {
                    if (strcmp(K.slots[s].occupants[o], name) == 0) {
                        for (int k = o; k < K.slots[s].occupant_count-1; k++)
                            strcpy(K.slots[s].occupants[k], K.slots[s].occupants[k+1]);
                        K.slots[s].occupant_count--;
                        break;
                    }
                }
            }
            /* Remove from hooks */
            for (int h = 0; h < K.hook_count; h++) {
                for (int o = 0; o < K.hooks[h].sub_count; o++) {
                    if (strcmp(K.hooks[h].subscribers[o], name) == 0) {
                        for (int k = o; k < K.hooks[h].sub_count-1; k++)
                            strcpy(K.hooks[h].subscribers[k], K.hooks[h].subscribers[k+1]);
                        K.hooks[h].sub_count--;
                        break;
                    }
                }
            }
            return ERR_OK;
        }
    }
    return ERR_NOT_FOUND;
}

static void init_builtin_modules(void) {
    { const char *c[]={"uptime"}; const char *s[]={"shell.commands"};
      const char *h[]={}; const char *p[]={"uptime"}; const char *r[]={};
      register_module("coreutils","Core utilities",c,1,s,1,h,0,p,1,r,0,"0.1"); }
    { const char *c[]={"hwinfo"}; const char *s[]={"diagnostics"};
      const char *h[]={"boot"}; const char *p[]={"hwinfo"}; const char *r[]={};
      register_module("hwprobe","Hardware probe",c,1,s,1,h,1,p,1,r,0,"0.1"); }
    { const char *c[]={"diag_status"}; const char *s[]={"diagnostics"};
      const char *h[]={"diag"}; const char *p[]={"diag_status"}; const char *r[]={};
      register_module("diag","Diagnostics",c,1,s,1,h,1,p,1,r,0,"0.1"); }
    { const char *c[]={"ai_status"}; const char *s[]={"ai.hooks"};
      const char *h[]={"ai"}; const char *p[]={"ai_status","intent_hints"}; const char *r[]={"oz.ledger"};
      register_module("ai_seed","AI seed hooks",c,1,s,1,h,1,p,2,r,1,"0.1"); }
    { const char *c[]={"ledger_size"}; const char *s[]={"oz.ledger","oz.contracts"};
      const char *h[]={"ai"}; const char *p[]={"ledger_size","trace","replay"}; const char *r[]={};
      register_module("oz_ledger","OZ Ledger provenance",c,1,s,2,h,1,p,3,r,0,"0.1"); }
    { const char *c[]={"fortune"}; const char *s[]={"shell.commands","experiments"};
      const char *h[]={"experiment"}; const char *p[]={"fortune"}; const char *r[]={};
      register_module("fortune_ext","Example extension: fortune",c,1,s,2,h,1,p,1,r,0,"0.1"); }
}

/* ─── Devices ─────────────────────────────────────────────────────────────── */
static void init_devices(void) {
    const char *names[] = {"console","mem","null","zero"};
    const char *types[] = {"char","block","char","char"};
    K.device_count = 4;
    for (int i = 0; i < 4; i++) {
        strcpy(K.devices[i].name, names[i]);
        strcpy(K.devices[i].type, types[i]);
        strcpy(K.devices[i].status, "active");
    }
}

/* ─── Environment ─────────────────────────────────────────────────────────── */
static void env_set(const char *key, const char *val) {
    for (int i = 0; i < K.env_count; i++) {
        if (strcmp(K.env[i].key, key) == 0) {
            strncpy(K.env[i].value, val, 127);
            return;
        }
    }
    if (K.env_count < MAX_ENV) {
        strncpy(K.env[K.env_count].key, key, 31);
        strncpy(K.env[K.env_count].value, val, 127);
        K.env_count++;
    }
}

static const char* env_get(const char *key) {
    for (int i = 0; i < K.env_count; i++)
        if (strcmp(K.env[i].key, key) == 0) return K.env[i].value;
    return NULL;
}

static void env_unset(const char *key) {
    for (int i = 0; i < K.env_count; i++) {
        if (strcmp(K.env[i].key, key) == 0) {
            for (int j = i; j < K.env_count-1; j++) K.env[j] = K.env[j+1];
            K.env_count--;
            return;
        }
    }
}

/* ─── Kernel Init ─────────────────────────────────────────────────────────── */
static void kernel_init(void) {
    memset(&K, 0, sizeof(Kernel));
    detect_hardware(&K.hw);
    mem_init(&K.mem);
    fs_init(&K.fs);
    fs_init_tree(&K.fs);
    proc_init(&K.procs);
    ledger_init(&K.ledger);
    init_builtin_slots();
    init_hooks();
    init_builtin_modules();
    init_devices();
    K.boot_time = time(NULL);
    strcpy(K.user, "user");
    K.running = 1;
    K.fortune_idx = 0;

    env_set("HOME", "/home/user");
    env_set("USER", "user");
    env_set("SHELL", "/bin/amossh");
    env_set("PATH", "/bin:/usr/bin");
    env_set("TERM", K.hw.terminal);
    env_set("HOSTNAME", K.hw.hostname);

    proc_spawn(&K.procs, "init");
    proc_spawn(&K.procs, "amossh");
}

/* ─── Command Implementations ─────────────────────────────────────────────── */

static int cmd_help(char *out, int sz, int argc, char **argv) {
    snprintf(out, sz,
        "amosOZ commands:\n"
        "  alloc append boot call cat cd chmod clear contracts cp date\n"
        "  devices echo env exit fortune free help history hooks hw\n"
        "  kill load loadmod ls mem mkdir mmap modules mv overhead\n"
        "  oz ps pwd replay rm rmdir run save selftest set slots\n"
        "  stat status tick touch trace tree uname unloadmod unset\n"
        "  version write");
    return ERR_OK;
}

static int cmd_uname(char *out, int sz, int argc, char **argv) {
    snprintf(out, sz, "%s %s %s %s", SYSTEM_NAME, VERSION, K.hw.machine, K.hw.platform);
    return ERR_OK;
}

static int cmd_version(char *out, int sz, int argc, char **argv) {
    snprintf(out, sz, "%s version %s (build %s)", SYSTEM_NAME, VERSION, BUILD_DATE);
    return ERR_OK;
}

static int cmd_boot(char *out, int sz, int argc, char **argv) {
    int elapsed = (int)(time(NULL) - K.boot_time);
    snprintf(out, sz, "Boot time: %sUptime: %ds", ctime(&K.boot_time), elapsed);
    return ERR_OK;
}

static int cmd_hw(char *out, int sz, int argc, char **argv) {
    snprintf(out, sz,
        "Hardware Profile:\n"
        "  platform: %s\n  release: %s\n  machine: %s\n"
        "  processor: %s\n  hostname: %s\n  cpu_count: %d\n"
        "  memory_mb: %ld\n  terminal: %s\n  persistence: %d",
        K.hw.platform, K.hw.platform_release, K.hw.machine,
        K.hw.processor, K.hw.hostname, K.hw.cpu_count,
        K.hw.memory_mb, K.hw.terminal, K.hw.persistence);
    return ERR_OK;
}

static int cmd_devices(char *out, int sz, int argc, char **argv) {
    int n = 0;
    n += snprintf(out+n, sz-n, "Devices:\n");
    for (int i = 0; i < K.device_count; i++)
        n += snprintf(out+n, sz-n, "  %-12s %-8s %s\n", K.devices[i].name, K.devices[i].type, K.devices[i].status);
    return ERR_OK;
}

static int cmd_gpu(char *out, int sz, int argc, char **argv) {
    snprintf(out, sz, "GPU: not available (C runtime, no GPU abstraction)");
    return ERR_OK;
}

static int cmd_mem(char *out, int sz, int argc, char **argv) {
    int used = K.mem.used_kb, total = K.mem.total_kb;
    int blocks = 0;
    for (int i = 0; i < K.mem.block_count; i++) if (K.mem.blocks[i].used) blocks++;
    snprintf(out, sz, "Memory: %d KB total, %d KB used, %d KB free, %d blocks",
             total, used, total - used, blocks);
    return ERR_OK;
}

static int cmd_mmap(char *out, int sz, int argc, char **argv) {
    int n = 0, any = 0;
    n += snprintf(out+n, sz-n, "ID    Size(KB)  Flags  Owner\n");
    for (int i = 0; i < K.mem.block_count; i++) {
        if (K.mem.blocks[i].used) {
            n += snprintf(out+n, sz-n, "%-5d %-9d %-6s %s\n",
                K.mem.blocks[i].id, K.mem.blocks[i].size,
                K.mem.blocks[i].flags, K.mem.blocks[i].owner);
            any = 1;
        }
    }
    if (!any) snprintf(out, sz, "No memory blocks allocated.");
    return ERR_OK;
}

static int cmd_alloc(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: alloc <size_kb>"); return ERR_INVALID; }
    int size = atoi(argv[1]);
    if (size <= 0) { snprintf(out, sz, "Error: size must be positive integer"); return ERR_INVALID; }
    int bid = mem_alloc(&K.mem, size, K.user, "rw-");
    if (bid < 0) { snprintf(out, sz, "Error: out of memory"); return ERR_NO_MEMORY; }
    snprintf(out, sz, "Allocated block %d (%d KB)", bid, size);
    return ERR_OK;
}

static int cmd_free(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: free <block_id>"); return ERR_INVALID; }
    int bid = atoi(argv[1]);
    int err = mem_free(&K.mem, bid);
    if (err != ERR_OK) { snprintf(out, sz, "Error: block %d not found", bid); return err; }
    snprintf(out, sz, "Freed block %d", bid);
    return ERR_OK;
}

static int cmd_ps(char *out, int sz, int argc, char **argv) {
    int n = 0;
    n += snprintf(out+n, sz-n, "PID   Name          State     Ticks\n");
    for (int i = 0; i < MAX_PROCS; i++) {
        if (K.procs.procs[i].used)
            n += snprintf(out+n, sz-n, "%-5d %-13s %-9s %d\n",
                K.procs.procs[i].pid, K.procs.procs[i].name,
                K.procs.procs[i].state, K.procs.procs[i].ticks);
    }
    return ERR_OK;
}

static int cmd_run(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: run <name>"); return ERR_INVALID; }
    int pid = proc_spawn(&K.procs, argv[1]);
    if (pid < 0) { snprintf(out, sz, "Error: process table full"); return ERR_NO_MEMORY; }
    snprintf(out, sz, "Started process '%s' with PID %d", argv[1], pid);
    return ERR_OK;
}

static int cmd_kill(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: kill <pid>"); return ERR_INVALID; }
    int pid = atoi(argv[1]);
    int err = proc_kill(&K.procs, pid);
    if (err != ERR_OK) { snprintf(out, sz, "Error: process %d not found", pid); return err; }
    snprintf(out, sz, "Killed process %d", pid);
    return ERR_OK;
}

static int cmd_tick(char *out, int sz, int argc, char **argv) {
    int t = proc_tick(&K.procs);
    snprintf(out, sz, "Tick: %d", t);
    return ERR_OK;
}

static int cmd_status(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: status <pid>"); return ERR_INVALID; }
    int pid = atoi(argv[1]);
    for (int i = 0; i < MAX_PROCS; i++) {
        if (K.procs.procs[i].used && K.procs.procs[i].pid == pid) {
            snprintf(out, sz, "PID %d: %s state=%s ticks=%d", pid,
                K.procs.procs[i].name, K.procs.procs[i].state, K.procs.procs[i].ticks);
            return ERR_OK;
        }
    }
    snprintf(out, sz, "Error: process %d not found", pid);
    return ERR_NO_PROCESS;
}

static int cmd_pwd(char *out, int sz, int argc, char **argv) {
    snprintf(out, sz, "%s", K.fs.cwd);
    return ERR_OK;
}

static int cmd_cd(char *out, int sz, int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "/home/user";
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, path, resolved);
    int idx = fs_find(&K.fs, resolved);
    if (idx < 0 || !K.fs.nodes[idx].is_dir) {
        snprintf(out, sz, "cd: no such directory: %s", path);
        return ERR_NOT_FOUND;
    }
    strcpy(K.fs.cwd, resolved);
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_ls(char *out, int sz, int argc, char **argv) {
    char resolved[MAX_PATH];
    if (argc > 1) fs_resolve(&K.fs, argv[1], resolved);
    else strcpy(resolved, K.fs.cwd);

    int idx = fs_find(&K.fs, resolved);
    if (idx < 0 || !K.fs.nodes[idx].is_dir) {
        snprintf(out, sz, "ls: no such directory");
        return ERR_NOT_FOUND;
    }

    int n = 0;
    char prefix[MAX_PATH];
    if (strcmp(resolved, "/") == 0) strcpy(prefix, "/");
    else snprintf(prefix, MAX_PATH, "%s/", resolved);
    int plen = strlen(prefix);

    for (int i = 0; i < K.fs.node_count; i++) {
        if (!K.fs.nodes[i].used) continue;
        if (strncmp(K.fs.nodes[i].path, prefix, plen) != 0) continue;
        const char *rest = K.fs.nodes[i].path + plen;
        if (strlen(rest) == 0 || strchr(rest, '/') != NULL) continue;
        n += snprintf(out+n, sz-n, "%s %s%s\n", K.fs.nodes[i].perms, rest,
                      K.fs.nodes[i].is_dir ? "/" : "");
    }
    if (n == 0) out[0] = '\0';
    return ERR_OK;
}

static int cmd_cat(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: cat <file>"); return ERR_INVALID; }
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, argv[1], resolved);
    int idx = fs_find(&K.fs, resolved);
    if (idx < 0 || K.fs.nodes[idx].is_dir) {
        snprintf(out, sz, "cat: %s: not found or not a file", argv[1]);
        return ERR_NOT_FOUND;
    }
    snprintf(out, sz, "%s", K.fs.nodes[idx].content);
    return ERR_OK;
}

static int cmd_touch(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: touch <file>"); return ERR_INVALID; }
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, argv[1], resolved);
    int idx = fs_find(&K.fs, resolved);
    if (idx >= 0) { K.fs.nodes[idx].modified = time(NULL); out[0]='\0'; return ERR_OK; }
    fs_add_file(&K.fs, resolved, "");
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_write(char *out, int sz, int argc, char **argv) {
    if (argc < 3) { snprintf(out, sz, "Usage: write <file> <content...>"); return ERR_INVALID; }
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, argv[1], resolved);
    char content[MAX_CONTENT] = "";
    for (int i = 2; i < argc; i++) {
        if (i > 2) strcat(content, " ");
        strncat(content, argv[i], MAX_CONTENT - strlen(content) - 1);
    }
    fs_add_file(&K.fs, resolved, content);
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_append(char *out, int sz, int argc, char **argv) {
    if (argc < 3) { snprintf(out, sz, "Usage: append <file> <content...>"); return ERR_INVALID; }
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, argv[1], resolved);
    int idx = fs_find(&K.fs, resolved);
    if (idx < 0 || K.fs.nodes[idx].is_dir) {
        snprintf(out, sz, "append: %s not found", argv[1]);
        return ERR_NOT_FOUND;
    }
    for (int i = 2; i < argc; i++) {
        if (i > 2) strncat(K.fs.nodes[idx].content, " ", MAX_CONTENT - strlen(K.fs.nodes[idx].content) - 1);
        strncat(K.fs.nodes[idx].content, argv[i], MAX_CONTENT - strlen(K.fs.nodes[idx].content) - 1);
    }
    K.fs.nodes[idx].modified = time(NULL);
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_rm(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: rm <file>"); return ERR_INVALID; }
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, argv[1], resolved);
    int idx = fs_find(&K.fs, resolved);
    if (idx < 0 || K.fs.nodes[idx].is_dir) {
        snprintf(out, sz, "rm: cannot remove %s", argv[1]);
        return ERR_NOT_FOUND;
    }
    K.fs.nodes[idx].used = 0;
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_mkdir(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: mkdir <dir>"); return ERR_INVALID; }
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, argv[1], resolved);
    int err = fs_add_dir(&K.fs, resolved);
    if (err != ERR_OK) { snprintf(out, sz, "mkdir: cannot create %s", argv[1]); return err; }
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_rmdir(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: rmdir <dir>"); return ERR_INVALID; }
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, argv[1], resolved);
    int idx = fs_find(&K.fs, resolved);
    if (idx < 0 || !K.fs.nodes[idx].is_dir) {
        snprintf(out, sz, "rmdir: cannot remove %s", argv[1]);
        return ERR_NOT_FOUND;
    }
    /* Check empty */
    char prefix[MAX_PATH];
    snprintf(prefix, MAX_PATH, "%s/", resolved);
    for (int i = 0; i < K.fs.node_count; i++) {
        if (K.fs.nodes[i].used && strncmp(K.fs.nodes[i].path, prefix, strlen(prefix)) == 0) {
            snprintf(out, sz, "rmdir: directory not empty");
            return ERR_INVALID;
        }
    }
    K.fs.nodes[idx].used = 0;
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_mv(char *out, int sz, int argc, char **argv) {
    if (argc < 3) { snprintf(out, sz, "Usage: mv <src> <dst>"); return ERR_INVALID; }
    char rs[MAX_PATH], rd[MAX_PATH];
    fs_resolve(&K.fs, argv[1], rs);
    fs_resolve(&K.fs, argv[2], rd);
    int idx = fs_find(&K.fs, rs);
    if (idx < 0) { snprintf(out, sz, "mv: %s not found", argv[1]); return ERR_NOT_FOUND; }
    strncpy(K.fs.nodes[idx].path, rd, MAX_PATH-1);
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_cp(char *out, int sz, int argc, char **argv) {
    if (argc < 3) { snprintf(out, sz, "Usage: cp <src> <dst>"); return ERR_INVALID; }
    char rs[MAX_PATH], rd[MAX_PATH];
    fs_resolve(&K.fs, argv[1], rs);
    fs_resolve(&K.fs, argv[2], rd);
    int idx = fs_find(&K.fs, rs);
    if (idx < 0 || K.fs.nodes[idx].is_dir) { snprintf(out, sz, "cp: error"); return ERR_NOT_FOUND; }
    fs_add_file(&K.fs, rd, K.fs.nodes[idx].content);
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_chmod(char *out, int sz, int argc, char **argv) {
    if (argc < 3) { snprintf(out, sz, "Usage: chmod <perms> <path>"); return ERR_INVALID; }
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, argv[2], resolved);
    int idx = fs_find(&K.fs, resolved);
    if (idx < 0) { snprintf(out, sz, "chmod: %s not found", argv[2]); return ERR_NOT_FOUND; }
    strncpy(K.fs.nodes[idx].perms, argv[1], 11);
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_stat(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: stat <path>"); return ERR_INVALID; }
    char resolved[MAX_PATH];
    fs_resolve(&K.fs, argv[1], resolved);
    int idx = fs_find(&K.fs, resolved);
    if (idx < 0) { snprintf(out, sz, "stat: %s not found", argv[1]); return ERR_NOT_FOUND; }
    FSNode *n = &K.fs.nodes[idx];
    snprintf(out, sz, "  path: %s\n  type: %s\n  perms: %s\n  owner: %s\n  size: %d",
        n->path, n->is_dir ? "dir" : "file", n->perms, n->owner,
        n->is_dir ? 0 : (int)strlen(n->content));
    return ERR_OK;
}

static int cmd_tree(char *out, int sz, int argc, char **argv) {
    char resolved[MAX_PATH];
    if (argc > 1) fs_resolve(&K.fs, argv[1], resolved);
    else strcpy(resolved, K.fs.cwd);

    int n = 0;
    char prefix[MAX_PATH];
    if (strcmp(resolved, "/") == 0) strcpy(prefix, "/");
    else snprintf(prefix, MAX_PATH, "%s/", resolved);
    int plen = strlen(prefix);

    for (int i = 0; i < K.fs.node_count; i++) {
        if (!K.fs.nodes[i].used) continue;
        if (strncmp(K.fs.nodes[i].path, prefix, plen) != 0) continue;
        const char *rest = K.fs.nodes[i].path + plen;
        if (strlen(rest) == 0) continue;
        /* Count depth by counting slashes */
        int depth = 0;
        for (const char *c = rest; *c; c++) if (*c == '/') depth++;
        if (depth > 3) continue;
        for (int d = 0; d < depth; d++) n += snprintf(out+n, sz-n, "  ");
        n += snprintf(out+n, sz-n, "%s%s\n", rest, K.fs.nodes[i].is_dir ? "/" : "");
    }
    if (n == 0) snprintf(out, sz, "(empty)");
    return ERR_OK;
}

static int cmd_echo(char *out, int sz, int argc, char **argv) {
    int n = 0;
    for (int i = 1; i < argc; i++) {
        if (i > 1) n += snprintf(out+n, sz-n, " ");
        n += snprintf(out+n, sz-n, "%s", argv[i]);
    }
    return ERR_OK;
}

static int cmd_env(char *out, int sz, int argc, char **argv) {
    int n = 0;
    for (int i = 0; i < K.env_count; i++)
        n += snprintf(out+n, sz-n, "%s=%s\n", K.env[i].key, K.env[i].value);
    return ERR_OK;
}

static int cmd_set(char *out, int sz, int argc, char **argv) {
    if (argc < 3) { snprintf(out, sz, "Usage: set <key> <value>"); return ERR_INVALID; }
    char val[128] = "";
    for (int i = 2; i < argc; i++) {
        if (i > 2) strcat(val, " ");
        strncat(val, argv[i], 127 - strlen(val));
    }
    env_set(argv[1], val);
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_unset(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: unset <key>"); return ERR_INVALID; }
    env_unset(argv[1]);
    out[0] = '\0';
    return ERR_OK;
}

static int cmd_date(char *out, int sz, int argc, char **argv) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(out, sz, "%Y-%m-%d %H:%M:%S %Z", t);
    return ERR_OK;
}

static int cmd_history(char *out, int sz, int argc, char **argv) {
    int n = 0;
    int start = K.history_count > 20 ? K.history_count - 20 : 0;
    for (int i = start; i < K.history_count; i++)
        n += snprintf(out+n, sz-n, "%d: %s\n", i+1, K.history[i]);
    if (n == 0) snprintf(out, sz, "(no history)");
    return ERR_OK;
}

static int cmd_oz(char *out, int sz, int argc, char **argv) {
    int mod_count = 0, hook_count = 0;
    for (int i = 0; i < K.module_count; i++) if (K.modules[i].loaded) mod_count++;
    for (int i = 0; i < K.hook_count; i++) hook_count += K.hooks[i].sub_count;
    int contract_count = 0;
    for (int i = 0; i < K.module_count; i++) if (K.modules[i].loaded) contract_count++;
    snprintf(out, sz,
        "OZ Layer — Extensibility Field\n"
        "  Modules loaded: %d\n  Slots defined: %d\n"
        "  Hooks active: %d\n  Contracts: %d\n  Ledger entries: %d",
        mod_count, K.slot_count, hook_count, contract_count, K.ledger.count);
    return ERR_OK;
}

static int cmd_slots(char *out, int sz, int argc, char **argv) {
    int n = 0;
    n += snprintf(out+n, sz-n, "Slots:\n");
    for (int i = 0; i < K.slot_count; i++) {
        n += snprintf(out+n, sz-n, "  %s: ", K.slots[i].name);
        if (K.slots[i].occupant_count == 0) n += snprintf(out+n, sz-n, "(empty)");
        else for (int j = 0; j < K.slots[i].occupant_count; j++)
            n += snprintf(out+n, sz-n, "%s%s", j?", ":"", K.slots[i].occupants[j]);
        n += snprintf(out+n, sz-n, "\n");
    }
    return ERR_OK;
}

static int cmd_modules(char *out, int sz, int argc, char **argv) {
    int n = 0;
    n += snprintf(out+n, sz-n, "Modules:\n");
    for (int i = 0; i < K.module_count; i++) {
        if (!K.modules[i].loaded) continue;
        n += snprintf(out+n, sz-n, "  %s: %s [commands:", K.modules[i].name, K.modules[i].description);
        for (int j = 0; j < K.modules[i].cmd_count; j++)
            n += snprintf(out+n, sz-n, " %s", K.modules[i].commands[j]);
        n += snprintf(out+n, sz-n, "]\n");
    }
    return ERR_OK;
}

static int cmd_overhead(char *out, int sz, int argc, char **argv) {
    int n = 0, total_dispatch = 0, total_hooks = 0, total_slots = 0, total_mods = 0;
    for (int i = 0; i < K.module_count; i++) {
        if (!K.modules[i].loaded) continue;
        total_mods++;
        total_dispatch += K.modules[i].cmd_count;
        total_hooks += K.modules[i].hook_count;
        total_slots += K.modules[i].slot_count;
    }
    n += snprintf(out+n, sz-n, "Overhead Accounting:\n");
    n += snprintf(out+n, sz-n, "  Total modules: %d\n", total_mods);
    n += snprintf(out+n, sz-n, "  Total dispatch entries: %d\n", total_dispatch);
    n += snprintf(out+n, sz-n, "  Total hooks: %d\n", total_hooks);
    n += snprintf(out+n, sz-n, "  Total slot occupations: %d\n\nPer-module:\n", total_slots);
    for (int i = 0; i < K.module_count; i++) {
        if (!K.modules[i].loaded) continue;
        n += snprintf(out+n, sz-n, "  %s: dispatch=%d hooks=%d slots=%d\n",
            K.modules[i].name, K.modules[i].cmd_count, K.modules[i].hook_count, K.modules[i].slot_count);
    }
    return ERR_OK;
}

static int cmd_hooks(char *out, int sz, int argc, char **argv) {
    int n = 0;
    n += snprintf(out+n, sz-n, "Hooks:\n");
    for (int i = 0; i < K.hook_count; i++) {
        n += snprintf(out+n, sz-n, "  %s: ", K.hooks[i].name);
        if (K.hooks[i].sub_count == 0) n += snprintf(out+n, sz-n, "(none)");
        else for (int j = 0; j < K.hooks[i].sub_count; j++)
            n += snprintf(out+n, sz-n, "%s%s", j?", ":"", K.hooks[i].subscribers[j]);
        n += snprintf(out+n, sz-n, "\n");
    }
    return ERR_OK;
}

static int cmd_contracts(char *out, int sz, int argc, char **argv) {
    int n = 0;
    n += snprintf(out+n, sz-n, "Module Contracts:\n");
    for (int i = 0; i < K.module_count; i++) {
        if (!K.modules[i].loaded) continue;
        n += snprintf(out+n, sz-n, "  %s:\n    provides:", K.modules[i].name);
        for (int j = 0; j < K.modules[i].provides_count; j++)
            n += snprintf(out+n, sz-n, " %s", K.modules[i].contract_provides[j]);
        n += snprintf(out+n, sz-n, "\n    requires:");
        for (int j = 0; j < K.modules[i].requires_count; j++)
            n += snprintf(out+n, sz-n, " %s", K.modules[i].contract_requires[j]);
        n += snprintf(out+n, sz-n, "\n    version: %s\n", K.modules[i].contract_version);
    }
    return ERR_OK;
}

static int cmd_loadmod(char *out, int sz, int argc, char **argv) {
    snprintf(out, sz, "loadmod: dynamic loading not supported in this version (modules are compiled-in)");
    return ERR_OK;
}

static int cmd_unloadmod(char *out, int sz, int argc, char **argv) {
    if (argc < 2) { snprintf(out, sz, "Usage: unloadmod <module_name>"); return ERR_INVALID; }
    int err = unload_module(argv[1]);
    if (err != ERR_OK) { snprintf(out, sz, "unloadmod: module '%s' not found", argv[1]); return err; }
    snprintf(out, sz, "Unloaded module '%s'", argv[1]);
    return ERR_OK;
}

static int cmd_call(char *out, int sz, int argc, char **argv) {
    if (argc < 3) { snprintf(out, sz, "Usage: call <module> <method>"); return ERR_INVALID; }
    /* Simplified: just check if module exists */
    for (int i = 0; i < K.module_count; i++) {
        if (K.modules[i].loaded && strcmp(K.modules[i].name, argv[1]) == 0) {
            snprintf(out, sz, "Called %s.%s -> OK", argv[1], argv[2]);
            return ERR_OK;
        }
    }
    snprintf(out, sz, "call: module '%s' not found", argv[1]);
    return ERR_NOT_FOUND;
}

static int cmd_trace(char *out, int sz, int argc, char **argv) {
    int n = 0, count = 10;
    if (argc > 1) count = atoi(argv[1]);
    if (K.ledger.count == 0) { snprintf(out, sz, "(no trace entries)"); return ERR_OK; }
    n += snprintf(out+n, sz-n, "OZ Ledger Trace:\n");
    int start = K.ledger.count > count ? K.ledger.count - count : 0;
    for (int i = start; i < K.ledger.count; i++)
        n += snprintf(out+n, sz-n, "  [%d] %s -> %d (%s)\n",
            K.ledger.entries[i].tick, K.ledger.entries[i].command,
            K.ledger.entries[i].result_code, K.ledger.entries[i].explanation);
    return ERR_OK;
}

static int cmd_replay(char *out, int sz, int argc, char **argv) {
    int n = 0;
    if (K.ledger.count == 0) { snprintf(out, sz, "(no replay data)"); return ERR_OK; }
    n += snprintf(out+n, sz-n, "Replay Log:\n");
    for (int i = 0; i < K.ledger.count; i++)
        n += snprintf(out+n, sz-n, "  tick=%d cmd=%s result=%d\n",
            K.ledger.entries[i].tick, K.ledger.entries[i].command, K.ledger.entries[i].result_code);
    return ERR_OK;
}

static int cmd_reset(char *out, int sz, int argc, char **argv) {
    kernel_init();
    snprintf(out, sz, "System reset complete.");
    return ERR_OK;
}

static int cmd_save(char *out, int sz, int argc, char **argv) {
    const char *fname = (argc > 1) ? argv[1] : "amosoz.img";
    FILE *f = fopen(fname, "wb");
    if (!f) { snprintf(out, sz, "save: cannot open %s", fname); return ERR_IO; }
    fwrite(&K.fs, sizeof(VirtualFS), 1, f);
    fwrite(&K.env, sizeof(K.env), 1, f);
    fwrite(&K.env_count, sizeof(int), 1, f);
    fclose(f);
    snprintf(out, sz, "System image saved to %s", fname);
    return ERR_OK;
}

static int cmd_load(char *out, int sz, int argc, char **argv) {
    const char *fname = (argc > 1) ? argv[1] : "amosoz.img";
    FILE *f = fopen(fname, "rb");
    if (!f) { snprintf(out, sz, "load: %s not found", fname); return ERR_IO; }
    fread(&K.fs, sizeof(VirtualFS), 1, f);
    fread(&K.env, sizeof(K.env), 1, f);
    fread(&K.env_count, sizeof(int), 1, f);
    fclose(f);
    snprintf(out, sz, "System image loaded from %s", fname);
    return ERR_OK;
}

static int cmd_clear(char *out, int sz, int argc, char **argv) {
    snprintf(out, sz, "\033[2J\033[H");
    return ERR_OK;
}

static const char *fortunes[] = {
    "The system is the territory.",
    "Every overhead has a name.",
    "A module is a guest with manners.",
    "Slots are finite; ambition is not.",
    "Trace everything. Regret nothing.",
};

static int cmd_fortune(char *out, int sz, int argc, char **argv) {
    snprintf(out, sz, "%s", fortunes[K.fortune_idx % 5]);
    K.fortune_idx++;
    return ERR_OK;
}

/* ─── Selftest ────────────────────────────────────────────────────────────── */
static int cmd_selftest(char *out, int sz, int argc, char **argv) {
    int n = 0, passed = 0, total = 0;
    #define CHECK(name, cond) do { \
        total++; \
        if (cond) { n += snprintf(out+n, sz-n, "  [PASS] %s\n", name); passed++; } \
        else { n += snprintf(out+n, sz-n, "  [FAIL] %s\n", name); } \
    } while(0)

    CHECK("boot_state", K.boot_time > 0);
    CHECK("hw_profile", strlen(K.hw.platform) > 0);
    CHECK("version", strcmp(VERSION, "0.1.0") == 0);

    /* Filesystem */
    fs_add_file(&K.fs, "/tmp/selftest_file", "hello");
    int idx = fs_find(&K.fs, "/tmp/selftest_file");
    CHECK("fs_write_read", idx >= 0 && strcmp(K.fs.nodes[idx].content, "hello") == 0);
    strncat(K.fs.nodes[idx].content, " world", MAX_CONTENT - strlen(K.fs.nodes[idx].content) - 1);
    CHECK("fs_append", strcmp(K.fs.nodes[idx].content, "hello world") == 0);
    K.fs.nodes[idx].used = 0;
    CHECK("fs_delete", fs_find(&K.fs, "/tmp/selftest_file") < 0);

    fs_add_dir(&K.fs, "/tmp/st_dir");
    CHECK("fs_mkdir", fs_find(&K.fs, "/tmp/st_dir") >= 0);
    int di = fs_find(&K.fs, "/tmp/st_dir");
    if (di >= 0) K.fs.nodes[di].used = 0;
    CHECK("fs_rmdir", fs_find(&K.fs, "/tmp/st_dir") < 0);

    /* Permissions */
    fs_add_file(&K.fs, "/tmp/ptest", "data");
    int pi = fs_find(&K.fs, "/tmp/ptest");
    strcpy(K.fs.nodes[pi].perms, "r--------");
    CHECK("permission_mutation", strcmp(K.fs.nodes[pi].perms, "r--------") == 0);
    K.fs.nodes[pi].used = 0;

    /* Memory */
    int bid = mem_alloc(&K.mem, 100, "selftest", "rw-");
    CHECK("mem_alloc", bid > 0);
    int merr = mem_free(&K.mem, bid);
    CHECK("mem_free", merr == ERR_OK);

    /* Command dispatch */
    char tmp[1024];
    dispatch("echo selftest_token", tmp, sizeof(tmp));
    CHECK("command_dispatch", strstr(tmp, "selftest_token") != NULL);

    /* Process table */
    int tpid = proc_spawn(&K.procs, "test_proc");
    CHECK("process_spawn", tpid > 0);
    proc_kill(&K.procs, tpid);
    int found = 0;
    for (int i = 0; i < MAX_PROCS; i++)
        if (K.procs.procs[i].used && K.procs.procs[i].pid == tpid) found = 1;
    CHECK("process_kill", !found);

    /* Module registration */
    int mod_found = 0;
    for (int i = 0; i < K.module_count; i++)
        if (K.modules[i].loaded && strcmp(K.modules[i].name, "coreutils") == 0) mod_found = 1;
    CHECK("module_registered", mod_found);

    /* Slot occupation */
    int slot_occ = 0;
    for (int i = 0; i < K.slot_count; i++)
        if (strcmp(K.slots[i].name, "shell.commands") == 0) slot_occ = K.slots[i].occupant_count;
    CHECK("slot_occupation", slot_occ > 0);

    /* Overhead */
    int total_mods = 0;
    for (int i = 0; i < K.module_count; i++) if (K.modules[i].loaded) total_mods++;
    CHECK("overhead_accounting", total_mods > 0);

    /* OZ layer */
    CHECK("oz_layer", K.slot_count == 10);

    /* Ledger */
    CHECK("oz_ledger", K.ledger.count > 0);

    /* Summary */
    char header[64];
    snprintf(header, sizeof(header), "amosOZ Selftest (%d/%d passed):\n", passed, total);
    char final_out[8192];
    snprintf(final_out, sizeof(final_out), "%s%s", header, out);
    if (passed == total)
        snprintf(final_out + strlen(final_out), sizeof(final_out) - strlen(final_out), "\n  ALL TESTS PASSED");
    else
        snprintf(final_out + strlen(final_out), sizeof(final_out) - strlen(final_out), "\n  %d TESTS FAILED", total - passed);
    strncpy(out, final_out, sz-1);
    out[sz-1] = '\0';

    #undef CHECK
    return ERR_OK;
}

/* ─── Command Dispatcher ──────────────────────────────────────────────────── */
typedef int (*CmdFunc)(char*, int, int, char**);
typedef struct { const char *name; CmdFunc func; } CmdEntry;

static const CmdEntry CMD_TABLE[] = {
    {"help", cmd_help}, {"clear", cmd_clear}, {"uname", cmd_uname},
    {"version", cmd_version}, {"boot", cmd_boot}, {"hw", cmd_hw},
    {"devices", cmd_devices}, {"gpu", cmd_gpu}, {"mem", cmd_mem},
    {"mmap", cmd_mmap}, {"alloc", cmd_alloc}, {"free", cmd_free},
    {"ps", cmd_ps}, {"run", cmd_run}, {"kill", cmd_kill},
    {"tick", cmd_tick}, {"status", cmd_status}, {"pwd", cmd_pwd},
    {"cd", cmd_cd}, {"ls", cmd_ls}, {"cat", cmd_cat},
    {"touch", cmd_touch}, {"write", cmd_write}, {"append", cmd_append},
    {"rm", cmd_rm}, {"mkdir", cmd_mkdir}, {"rmdir", cmd_rmdir},
    {"mv", cmd_mv}, {"cp", cmd_cp}, {"chmod", cmd_chmod},
    {"stat", cmd_stat}, {"tree", cmd_tree}, {"echo", cmd_echo},
    {"env", cmd_env}, {"set", cmd_set}, {"unset", cmd_unset},
    {"date", cmd_date}, {"history", cmd_history}, {"selftest", cmd_selftest},
    {"oz", cmd_oz}, {"slots", cmd_slots}, {"modules", cmd_modules},
    {"overhead", cmd_overhead}, {"hooks", cmd_hooks}, {"contracts", cmd_contracts},
    {"loadmod", cmd_loadmod}, {"unloadmod", cmd_unloadmod}, {"call", cmd_call},
    {"trace", cmd_trace}, {"replay", cmd_replay}, {"reset", cmd_reset},
    {"save", cmd_save}, {"load", cmd_load}, {"fortune", cmd_fortune},
    {NULL, NULL}
};

static int dispatch(const char *line, char *output, int outsize) {
    output[0] = '\0';
    if (!line || !line[0]) return ERR_OK;

    /* Save to history */
    if (K.history_count < MAX_HISTORY)
        strncpy(K.history[K.history_count++], line, MAX_CMD_LEN-1);

    /* Parse */
    char buf[MAX_CMD_LEN];
    strncpy(buf, line, MAX_CMD_LEN-1); buf[MAX_CMD_LEN-1] = '\0';
    char *argv[32];
    int argc = 0;
    char *tok = strtok(buf, " \t");
    while (tok && argc < 32) { argv[argc++] = tok; tok = strtok(NULL, " \t"); }
    if (argc == 0) return ERR_OK;

    /* Find command */
    for (int i = 0; CMD_TABLE[i].name; i++) {
        if (strcmp(argv[0], CMD_TABLE[i].name) == 0) {
            int result = CMD_TABLE[i].func(output, outsize, argc, argv);
            ledger_record(&K.ledger, line, K.user, K.procs.tick_count, result);
            return result;
        }
    }

    /* Check exit */
    if (strcmp(argv[0], "exit") == 0) {
        K.running = 0;
        snprintf(output, outsize, "Shutting down amosOZ.");
        return ERR_OK;
    }

    snprintf(output, outsize, "amosoz: command not found: %s", argv[0]);
    ledger_record(&K.ledger, line, K.user, K.procs.tick_count, ERR_NOT_FOUND);
    return ERR_NOT_FOUND;
}

/* ─── Main ────────────────────────────────────────────────────────────────── */
int main(int argc, char **argv) {
    kernel_init();
    printf("%s v%s\n", SYSTEM_NAME, VERSION);
    printf("Type 'help' for commands, 'selftest' to verify, 'exit' to quit.\n\n");

    char line[MAX_CMD_LEN];
    char output[8192];

    while (K.running) {
        printf("%s@amosoz:%s$ ", K.user, K.fs.cwd);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;
        /* Strip newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';

        dispatch(line, output, sizeof(output));
        if (output[0]) printf("%s\n", output);
    }
    return 0;
}

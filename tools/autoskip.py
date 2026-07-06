import gdb
import re

GUEST_BASE = 0x100000000
GUEST_BASE_HI = 0x100100000  # generous upper bound for "this is a null-page deref"
MAX_FIXES = 400

fixlog = open("/home/deck/simpsons-recomp/autoskip.log", "w")
fix_count = 0
pc_hit_count = {}

# Matches the simple "load from guest memory" shape the recompiler emits:
#   mov    (%rbx,%rXX,1),%rYY / %eYY / %r8d etc.
LOAD_RE = re.compile(r"mov\s+\(%rbx,%(\w+),1\),%(\w+)")

REG64 = {
    "eax": "rax", "ebx": "rbx", "ecx": "rcx", "edx": "rdx",
    "esi": "rsi", "edi": "rdi", "ebp": "rbp", "esp": "rsp",
}
def to64(name):
    if name in REG64:
        return REG64[name]
    if re.fullmatch(r"r\d+d", name):
        return name[:-1]  # r8d -> r8
    return name  # already 64-bit (rax, r8, ...)

def stop_handler(event):
    global fix_count
    if not isinstance(event, gdb.SignalEvent) or event.stop_signal != "SIGSEGV":
        return
    frame = gdb.selected_frame()
    pc = int(frame.pc())
    pc_hit_count[pc] = pc_hit_count.get(pc, 0) + 1

    if fix_count >= MAX_FIXES or pc_hit_count[pc] > 3:
        fixlog.write(f"GIVING UP at pc=0x{pc:x} (fix_count={fix_count}, repeats={pc_hit_count[pc]})\n")
        fixlog.flush()
        return  # leave it stopped; not auto-continuing further

    arch = frame.architecture()
    insns = arch.disassemble(pc, count=1)
    text = insns[0]["asm"]
    length = insns[0]["length"]
    m = LOAD_RE.search(text)

    loc = gdb.find_pc_line(pc)
    src = f"{loc.symtab}:{loc.line}" if loc and loc.symtab else "?"

    if not m:
        fixlog.write(f"UNHANDLED FAULT at pc=0x{pc:x} ({src}) insn='{text}' -- stopping for manual review\n")
        fixlog.flush()
        return

    dest = to64(m.group(2))
    try:
        gdb.execute(f"set ${dest} = 0", to_string=True)
        gdb.execute(f"set $pc = $pc + {length}", to_string=True)
        fix_count += 1
        fixlog.write(f"FIX #{fix_count}: pc=0x{pc:x} ({src}) insn='{text}' -> ${dest}=0, skip {length}\n")
        fixlog.flush()
        gdb.post_event(lambda: gdb.execute("continue"))
    except gdb.error as e:
        fixlog.write(f"ERROR patching pc=0x{pc:x}: {e}\n")
        fixlog.flush()

gdb.events.stop.connect(stop_handler)
print("autoskip handler installed")

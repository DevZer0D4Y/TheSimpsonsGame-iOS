import gdb
import re

MAX_FIXES = 150000
GUEST_BASE_LO = 0x0FF000000
GUEST_BASE_HI = 0x101000000
LOAD_RE = re.compile(r"mov[zs]?[bwlq]{0,2}\s+\(%(\w+),%(\w+),1\),%(\w+)$")
# stores: movl/movb/movw/movq $imm,(BASE,INDEX,1)  or  mov %src,(BASE,INDEX,1)
STORE_IMM_RE = re.compile(r"mov[lbwq]?\s+\$(0x[0-9a-f]+|\d+),\(%(\w+),%(\w+),1\)$")
STORE_REG_RE = re.compile(r"mov[lbwq]?\s+%(\w+),\(%(\w+),%(\w+),1\)$")
CMP_RE = re.compile(r"cmp([bwlq]?)\s+\$(0x[0-9a-f]+|\d+),\(%(\w+),%(\w+),1\)$")

WIDTH_BITS = {"b": 8, "w": 16, "l": 32, "q": 64, "": 32}

def sub_flags(a, b, bits):
    # flags for (a - b) at given width, a/b unsigned-interpreted inputs
    mask = (1 << bits) - 1
    signbit = 1 << (bits - 1)
    raw = a - b
    result = raw & mask
    cf = 1 if (a & mask) < (b & mask) else 0
    zf = 1 if result == 0 else 0
    sf = 1 if (result & signbit) else 0
    a_s = a & signbit
    b_s = b & signbit
    r_s = result & signbit
    of = 1 if (a_s != b_s) and (r_s != a_s) else 0
    pf = 1 if bin(result & 0xFF).count("1") % 2 == 0 else 0
    return cf, pf, zf, sf, of

def set_flags(cf, pf, zf, sf, of):
    eflags = int(gdb.parse_and_eval("$eflags"))
    for bit, val in ((0, cf), (2, pf), (6, zf), (7, sf), (11, of)):
        if val:
            eflags |= (1 << bit)
        else:
            eflags &= ~(1 << bit)
    gdb.execute(f"set $eflags = {eflags}", to_string=True)
REG64 = {"eax":"rax","ebx":"rbx","ecx":"rcx","edx":"rdx","esi":"rsi","edi":"rdi","ebp":"rbp","esp":"rsp"}
def to64(name):
    if name in REG64: return REG64[name]
    if re.fullmatch(r"r\d+d", name): return name[:-1]
    return name

fixlog = open("/home/deck/simpsons-recomp/autoskip.log", "w")
pc_hits = {}

def is_stopped_on_segv():
    try:
        info = gdb.execute("info program", to_string=True)
        return any(s in info for s in ("SIGSEGV", "SIGBUS", "SIGABRT", "SIGTRAP"))
    except gdb.error:
        return False

def proc_alive():
    inf = gdb.selected_inferior()
    return inf.pid != 0

gdb.execute("handle SIGBUS nopass", to_string=True)
gdb.execute("handle SIGSEGV nopass", to_string=True)
gdb.execute("handle SIGABRT nopass", to_string=True)
gdb.execute("handle SIGTRAP nopass", to_string=True)
gdb.execute("run", to_string=True)

fix_count = 0
while fix_count < MAX_FIXES and proc_alive() and is_stopped_on_segv():
    frame = gdb.selected_frame()
    pc = int(frame.pc())
    pc_hits[pc] = pc_hits.get(pc, 0) + 1
    if pc_hits[pc] > 150000:
        fixlog.write(f"GIVING UP: pc=0x{pc:x} repeated {pc_hits[pc]}x\n")
        fixlog.flush()
        break

    arch = frame.architecture()
    insns = arch.disassemble(pc, count=1)
    text = insns[0]["asm"]
    length = insns[0]["length"]
    loc = gdb.find_pc_line(pc)
    src = f"{loc.symtab}:{loc.line}" if loc and loc.symtab else "?"

    m_load = LOAD_RE.search(text)
    m_store_imm = STORE_IMM_RE.search(text)
    m_store_reg = STORE_REG_RE.search(text)
    m_cmp = CMP_RE.search(text)

    if m_load:
        base_reg = to64(m_load.group(1))
    elif m_store_imm:
        base_reg = to64(m_store_imm.group(2))
    elif m_store_reg:
        base_reg = to64(m_store_reg.group(2))
    elif m_cmp:
        base_reg = to64(m_cmp.group(3))
    else:
        fname = frame.name() or ""
        # Known fatal-abort entry points: collect every matching frame while
        # walking up, then return from the OUTERMOST one whose name suggests
        # it's the PPC-callable wrapper (closest to the original guest caller)
        # so any host-side "shouldn't return" cleanup code is also skipped.
        TRAP_NAMES = ("InvalidFunctionTrap", "KeBugCheck", "rex::debug::Break")
        candidates = []
        gf = frame
        depth = 0
        while gf and depth < 10:
            gn = gf.name() or ""
            if any(t in gn for t in TRAP_NAMES):
                candidates.append(gf)
            gf = gf.older()
            depth += 1

        if candidates:
            trap_frame = candidates[-1]  # outermost match
            label = trap_frame.name() or "?"
            trap_frame.select()
            gdb.execute("return", to_string=True)
            fix_count += 1
            fixlog.write(f"FIX #{fix_count}: fatal trap '{label}' "
                          f"-> forced return (treated as no-op)\n")
            fixlog.flush()
            gdb.execute("continue", to_string=True)
            continue

        if not fname.startswith("__imp__sub_") and fname != "__imp__xstart":
            fixlog.write(f"UNHANDLED (host-code signal): top frame={fname} pc=0x{pc:x}\n")
            try:
                fixlog.write("info program:\n" + gdb.execute("info program", to_string=True) + "\n")
            except gdb.error as e:
                fixlog.write(f"(info program failed: {e})\n")
            gf = frame
            depth = 0
            while gf and depth < 20:
                gn = gf.name() or ""
                if gn.startswith("__imp__sub_") or gn == "__imp__xstart":
                    gpc = int(gf.pc())
                    gloc = gdb.find_pc_line(gpc)
                    gsrc = f"{gloc.symtab}:{gloc.line}" if gloc and gloc.symtab else "?"
                    fixlog.write(f"nearest guest frame: {gn} pc=0x{gpc:x} ({gsrc})\n")
                    try:
                        ginsns = gf.architecture().disassemble(gpc - 24, gpc + 8)
                        for ins in ginsns:
                            marker = "=>" if ins["addr"] == gpc else "  "
                            fixlog.write(f"  {marker} 0x{ins['addr']:x}: {ins['asm']}\n")
                    except gdb.error as e:
                        fixlog.write(f"(disassemble failed: {e})\n")
                    break
                gf = gf.older()
                depth += 1
            fixlog.flush()
        else:
            fixlog.write(f"UNHANDLED (no match): pc=0x{pc:x} ({src}) insn='{text}'\n")
            fixlog.flush()
        break

    base_val = int(gdb.parse_and_eval(f"${base_reg}"))
    if not (GUEST_BASE_LO <= base_val <= GUEST_BASE_HI):
        fixlog.write(f"UNHANDLED (base 0x{base_val:x} not guest-base-like): "
                      f"pc=0x{pc:x} ({src}) insn='{text}'\n")
        fixlog.flush()
        break

    if m_load:
        dest = to64(m_load.group(3))
        gdb.execute(f"set ${dest} = 0", to_string=True)
        action = f"${dest}=0 (load->0)"
    elif m_cmp:
        width = WIDTH_BITS[m_cmp.group(1)]
        imm = int(m_cmp.group(2), 0)
        cf, pf, zf, sf, of = sub_flags(0, imm, width)
        set_flags(cf, pf, zf, sf, of)
        action = f"flags set as if mem(0) cmp {imm}"
    else:
        # store to unmapped guest null page: drop the write, it can't matter
        action = "dropped write"

    gdb.execute(f"set $pc = $pc + {length}", to_string=True)
    fix_count += 1
    if fix_count <= 1000 or fix_count % 500 == 0:
        fixlog.write(f"FIX #{fix_count}: pc=0x{pc:x} ({src}) insn='{text}' base=0x{base_val:x} -> {action}, skip {length}\n")
        fixlog.flush()
    gdb.execute("continue", to_string=True)

fixlog.write(f"\nDONE. total_fixes={fix_count} proc_alive={proc_alive()} stopped_on_segv={is_stopped_on_segv()}\n")
if proc_alive():
    fixlog.write("bt:\n" + gdb.execute("bt 15", to_string=True) + "\n")
fixlog.close()

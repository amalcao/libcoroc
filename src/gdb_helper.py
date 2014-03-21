#!/usr/bin/python

import sys

print >>sys.stderr, "Loading the LibTSC runtime support.."

sts = { 0xBAFF:"sleep", 0xFACE:"ready", 0xBEEF:"run", 0xBADD:"wait", 0xDEAD:"exit"}

def linked_list(ptr, linkfield):
    cop = gdb.lookup_type('struct tsc_coroutine').pointer()
    while ptr:
        yield ptr['owner'].cast(cop)
        ptr = ptr[linkfield]


class CoroutinesCmd(gdb.Command):
    "List all coroutines."

    def __init__(self):
        super(CoroutinesCmd, self).__init__("info coroutines", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, form_tty):
        vp = gdb.lookup_type('void').pointer()
        head = gdb.parse_and_eval('vpu_manager.coroutine_list.head')
        for ptr in linked_list(head, 'next'):
            if ptr['status'] == 0xDEAD: # exit
                continue;
            
            s = ' '
            if ptr['status'] == 0xBEEF: # running
                s = '*'
            
            ## sp = ptr['ctx']['uc_mcontext']['gregs'][15].cast(vp)
            pc = ptr['ctx']['uc_mcontext']['gregs'][16].cast(vp)
            blk = gdb.block_for_pc(long(pc))
            sl = gdb.find_pc_line(long(pc))

            print s, ptr['id'], "%5s" % sts[long(ptr['status'])], \
               "\"%8s\"" % ptr['name'].string().ljust(8), blk.function, sl.symtab, sl.line


def switch_context(pc, sp, bp):
    frame = gdb.selected_frame()
    arch = frame.architecture()

    if arch.name() == 'i386:x86-64':
        gdb.parse_and_eval('$save_pc = $rip')
        gdb.parse_and_eval('$save_sp = $rsp')
        gdb.parse_and_eval('$save_bp = $rbp')
        gdb.parse_and_eval('$rip = 0x%x' % long(pc))
        gdb.parse_and_eval('$rsp = 0x%x' % long(sp))
        gdb.parse_and_eval('$rbp = 0x%x' % long(bp))
    elif arch.name() == 'i386:x86':
        gdb.parse_and_eval('$save_pc = $pc')
        gdb.parse_and_eval('$save_sp = $sp')
        gdb.parse_and_eval('$save_bp = $bp')
        gdb.parse_and_eval('$pc = 0x%x' % long(pc))
        gdb.parse_and_eval('$sp = 0x%x' % long(sp))
        gdb.parse_and_eval('$bp = 0x%x' % long(bp))
     
    # TODO : more arch ..
    return 

def resume_context():
    frame = gdb.selected_frame()
    arch = frame.architecture()
    
    if arch.name() == 'i386:x86-64':
        gdb.parse_and_eval('$rip = $save_pc')
        gdb.parse_and_eval('$rsp = $save_sp')
        gdb.parse_and_eval('$rbp = $save_bp')
    elif arch.name() == 'i386:x86':
        gdb.parse_and_eval('$pc = $save_pc')
        gdb.parse_and_eval('$sp = $save_sp')
        gdb.parse_and_eval('$bp = $save_bp')
    # TODO : more arch ..
    return
    

def find_running(ptr):
    arch = gdb.selected_frame().architecture()
    vp = gdb.lookup_type('void').pointer()
    ## get the vpu who runs the current coroutine.
    vpu = gdb.parse_and_eval("& vpu_manager.vpu[%d]" % ptr['vpu_id'])

    threads = gdb.selected_inferior().threads()
    cur = gdb.selected_thread()

    for thr in threads:
        ## switch to the thr and check the TLS __vpu .
        thr.switch()
        if vpu == gdb.parse_and_eval('__vpu'):
            pc = long(gdb.parse_and_eval('$pc'))
            sp = long(gdb.parse_and_eval('$sp'))
            bp = long(gdb.parse_and_eval('$bp'))
            if arch.name() == 'i386:x86-64':
                bp = long(gdb.parse_and_eval('$rbp'))
            cur.switch()
            return pc, sp, bp

    cur.switch()
    return None, None, None


def find_coroutine(cid):
    vp = gdb.lookup_type('void').pointer()
    head = gdb.parse_and_eval('vpu_manager.coroutine_list.head')
    for ptr in linked_list(head, 'next'):
        if ptr['status'] == 0xDEAD:
            continue;
        if ptr['id'] == cid:
            if ptr['status'] == 0xBEEF:
                return find_running(ptr)
            bp = ptr['ctx']['uc_mcontext']['gregs'][10].cast(vp)
            sp = ptr['ctx']['uc_mcontext']['gregs'][15].cast(vp)
            pc = ptr['ctx']['uc_mcontext']['gregs'][16].cast(vp)
            return pc, sp, bp

    return None, None, None


class CoroutineCmd(gdb.Command):
    """Execute gdb command in the context of coroutine with <id>.

    Switch PC and SP to the ones in the coroutine's VPU,
    execute an arbitrary gdb command, and restore the PC and SP.

    Usage: (gdb) coroutine <id> <gdbcmd>

    Note that it is ill-defined to modify state in the context of a coroutine.
    Restrict yourself to inspecting values.
    """

    def __init__(self):
        super(CoroutineCmd, self).__init__("coroutine", gdb.COMMAND_STACK, gdb.COMPLETE_NONE)

    def invoke(self, arg, from_tty):
        cid, cmd = arg.split(None, 1)
        cid = gdb.parse_and_eval(cid)
        pc, sp, bp = find_coroutine(int(cid))
        if not pc:
            print "No such coroutine: ", cid
            return

        save_frame = gdb.selected_frame()
        switch_context(pc, sp, bp)
        try:
            gdb.execute(cmd)
        finally:
            resume_context()
            save_frame.select()

#
# Register all CLI commands
CoroutinesCmd()
CoroutineCmd()

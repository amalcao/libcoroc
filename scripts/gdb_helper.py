#!/usr/bin/python

import sys

sys.stderr.write("Loading the LibTSC runtime support..\n")

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
        arch = gdb.selected_frame().architecture()
        vp = gdb.lookup_type('void').pointer()
        head = gdb.parse_and_eval('vpu_manager.coroutine_list.head')
        for ptr in linked_list(head, 'next'):
            if ptr['status'] == 0xDEAD: # exit
                continue;
            
            s = ' '
            if ptr['status'] == 0xBEEF: # running
                s = '*'
            
            if arch.name() == 'i386:x86-64':
                pc = ptr['ctx']['uc_mcontext']['gregs'][16].cast(vp)
            elif arch.name() == 'i386':
                pc = ptr['ctx']['uc_mcontext']['gregs'][14].cast(vp)
            elif arch.name() == 'arm':
                pc = ptr['ctx']['uc_mcontext']['arm_pc'].cast(vp)

            blk = gdb.block_for_pc(int(pc))
            sl = gdb.find_pc_line(int(pc))

            name = ptr['name'].string()
            if len(name) == 0:
                name = "<anno>"

            print (s, ptr['id'], "%5s" % sts[int(ptr['status'])], \
               "\"%s\"" % name, blk.function, sl.symtab, sl.line)


def switch_context(pc, sp, bp):
    frame = gdb.selected_frame()
    arch = frame.architecture()

    if arch.name() == 'i386:x86-64':
        gdb.parse_and_eval('$save_pc = $rip')
        gdb.parse_and_eval('$save_sp = $rsp')
        gdb.parse_and_eval('$save_bp = $rbp')
        gdb.parse_and_eval('$rip = 0x%x' % int(pc))
        gdb.parse_and_eval('$rsp = 0x%x' % int(sp))
        gdb.parse_and_eval('$rbp = 0x%x' % int(bp))
    elif arch.name() == 'i386':
        gdb.parse_and_eval('$save_pc = $eip')
        gdb.parse_and_eval('$save_sp = $esp')
        gdb.parse_and_eval('$save_bp = $ebp')
        gdb.parse_and_eval('$eip = 0x%x' % int(pc))
        gdb.parse_and_eval('$esp = 0x%x' % int(sp))
        gdb.parse_and_eval('$ebp = 0x%x' % int(bp))
    elif arch.name() == 'arm':
        gdb.parse_and_eval('$save_pc = $pc')
        gdb.parse_and_eval('$save_sp = $sp')
        gdb.parse_and_eval('$save_bp = $r7')
        gdb.parse_and_eval('$pc = 0x%x' % int(pc))
        gdb.parse_and_eval('$sp = 0x%x' % int(sp))
        gdb.parse_and_eval('$r7 = 0x%x' % int(bp))
        
     
    # TODO : more arch ..
    return 

def resume_context():
    frame = gdb.selected_frame()
    arch = frame.architecture()
    
    if arch.name() == 'i386:x86-64':
        gdb.parse_and_eval('$rip = $save_pc')
        gdb.parse_and_eval('$rsp = $save_sp')
        gdb.parse_and_eval('$rbp = $save_bp')
    elif arch.name() == 'i386':
        gdb.parse_and_eval('$eip = $save_pc')
        gdb.parse_and_eval('$esp = $save_sp')
        gdb.parse_and_eval('$ebp = $save_bp')
    elif arch.name() == 'arm':
        gdb.parse_and_eval('$pc = $save_pc')
        gdb.parse_and_eval('$sp = $save_sp')
        gdb.parse_and_eval('$r7 = $save_bp')
    # TODO : more arch ..
    return


def find_vpu_thread(cid):
    num = int(gdb.parse_and_eval("vpu_manager.xt_index"))
    found = False
    for index in range(0, num):
        vpu = gdb.parse_and_eval("& vpu_manager.vpu[%d]" % index)
        if int(vpu['current']['id']) == cid:
            found = True
            break
    
    if not found:
        return None

    cur = gdb.selected_thread()
    threads = gdb.selected_inferior().threads()

    for thr in threads:
        thr.switch()
        if vpu == gdb.parse_and_eval('__vpu'):
            return thr

    cur.switch()
    return None


def find_running(cid):
    arch = gdb.selected_frame().architecture()
    cur = gdb.selected_thread()
    thr = find_vpu_thread(cid)

    if not thr:
        return None, None, None

    if arch.name() == 'i386:x86-64':
        pc = int(gdb.parse_and_eval('$rip'))
        sp = int(gdb.parse_and_eval('$rsp'))
        bp = int(gdb.parse_and_eval('$rbp'))
    elif arch.name() == 'i386':
        pc = int(gdb.parse_and_eval('$eip'))
        sp = int(gdb.parse_and_eval('$esp'))
        bp = int(gdb.parse_and_eval('$ebp'))
    elif arch.name() == 'arm':
        pc = int(gdb.parse_and_eval('$pc'))
        sp = int(gdb.parse_and_eval('$sp'))
        bp = int(gdb.parse_and_eval('$r7'))

    cur.switch()
    return pc, sp, bp



def find_coroutine(cid):
    arch = gdb.selected_frame().architecture()
    vp = gdb.lookup_type('void').pointer()
    head = gdb.parse_and_eval('vpu_manager.coroutine_list.head')
    for ptr in linked_list(head, 'next'):
        if ptr['status'] == 0xDEAD:
            continue;
        if ptr['id'] == cid:
            if ptr['status'] == 0xBEEF:
                return find_running(cid)
            if arch.name() == 'i386:x86-64':
                bp = ptr['ctx']['uc_mcontext']['gregs'][10].cast(vp)
                sp = ptr['ctx']['uc_mcontext']['gregs'][15].cast(vp)
                pc = ptr['ctx']['uc_mcontext']['gregs'][16].cast(vp)
            elif arch.name() == 'i386':
                bp = ptr['ctx']['uc_mcontext']['gregs'][6].cast(vp)
                sp = ptr['ctx']['uc_mcontext']['gregs'][7].cast(vp)
                pc = ptr['ctx']['uc_mcontext']['gregs'][14].cast(vp)
            elif arch.name() == 'arm':
                bp = ptr['ctx']['uc_mcontext']['arm_r7'].cast(vp)
                sp = ptr['ctx']['uc_mcontext']['arm_sp'].cast(vp)
                pc = ptr['ctx']['uc_mcontext']['arm_pc'].cast(vp)
                
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
        __args = arg.split(None, 1)
        if len(__args) == 0:
            ## if no arg provided, just print current cid:
            vpu = gdb.parse_and_eval("__vpu")
            if vpu:
                print (vpu['current']['id'])
            return

        cid = gdb.parse_and_eval(__args[0])
        if len(__args) > 1:
            cmd = __args[1]
            pc, sp, bp = find_coroutine(int(cid))
            if not pc:
                print ("No such coroutine: ", cid)
                return

            save_frame = gdb.selected_frame()
            switch_context(pc, sp, bp)
            try:
                gdb.execute(cmd)
            finally:
                resume_context()
                save_frame.select()
        else:
            ## try to switch to this coroutine
            ## if it's running on another VPU
            thr = find_vpu_thread(cid)
            if not thr:
                print ("No such coroutine %d is running!" % int(cid))
                return
            thr.switch()
#
# Register all CLI commands
CoroutinesCmd()
CoroutineCmd()

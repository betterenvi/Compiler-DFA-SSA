import sys, os, re, collections

#from CFG import *

class Instruction(object):
    '''
    Naming rules:
    BR: branches
    PROC: procedure call, ret
    '''
    RET_OPS = ['ret']
    BR_OPS = ['br', 'blbc', 'blbs']
    PROC_OPS = ['call'] + RET_OPS
    PROC_START_OP = 'enter'
    JMP_OPS = BR_OPS + ['call'] # although 'ret' is also a jump, it does not have explicit dst addr in the instruction
    CBR_OPS = ['blbc', 'blbs'] # conditional branch
    CJMP_OPS = CBR_OPS
    UBR_OPS = ['br'] # unconditional branch
    UJMP_OPS = UBR_OPS + ['call'] # although 'ret' is also a jump, it does not have explicit dst addr in the instruction
    LEADER_OPS = ['enter']
    INVALID_LEADER_OPS = ['entrypc', 'nop']
    TERMINATOR_OPS = BR_OPS + PROC_OPS
    ARITH_OPS = ['add', 'sub', 'mul', 'div', 'mod', 'neg', 'cmpeq', 'cmple', 'cmplt']

    REGISTER_PATTERN = '\((\d+)\)' # e.g. (5)
    INSTR_PATTERN = '\[(\d+)\]' # e.g. [5]
    NUM_PATTERN = '(\d+)' # e.g. 5
    BASE_PATTERN = '[_a-zA-Z0-9]+_base#([-0-9]+)'  # e.g. global_array_base#32576
    OFFSET_PATTERN = '[_a-zA-Z0-9]+_offset#([-0-9]+)' # e.g. y_offset#8
    VAR_PATTERN = '([_a-zA-Z0-9]+)#[-0-9]+' # e.g. i#-8
    PATTERNS = {'r':REGISTER_PATTERN, 'i':INSTR_PATTERN, 'n':NUM_PATTERN, 'b':BASE_PATTERN,
    'o':OFFSET_PATTERN, 'v':VAR_PATTERN}

    def __init__(self, instr_id, op, operand1, operand2):
        super(Instruction, self).__init__()
        self.instr_id = instr_id # integer starting from 1
        self.op = op
        self.operand1 = operand1
        self.operand2 = operand2
        self.left = list()
        self.right = ''
        self._parse()

    def _parse_operand(self, operand):
        for k, pat in Instruction.PATTERNS.items():
            m = re.match(pat, operand)
            if m != None:
                res = m.group(1)
                if k == 'r':
                    return k, 'r' + res
                elif k == 'i':
                    return k, int(res)
                return k, res

        return '', operand

    def _parse(self):
        self.operand1_parsed = self._parse_operand(self.operand1)[1]
        self.operand2_parsed = self._parse_operand(self.operand2)[1]
        if self.op in Instruction.ARITH_OPS:
            self.right = '%s %s %s' % (self.operand1_parsed, self.op, self.operand2_parsed)
            self.left.append('r%s' % self.instr_id)
        elif self.op == 'move':
            self.right = self.operand1_parsed
            self.left.append(self.operand2_parsed)
            self.left.append('r%s' % self.instr_id)
        elif self.op == 'load':
            self.right = self.operand1_parsed
            self.left.append('r%s' % self.instr_id)
        elif self.op == 'store':
            self.right = self.operand1_parsed
            self.left.append('*(%s)' % self.operand2_parsed)
        elif self.op in Instruction.CJMP_OPS:
            self.dst_bbn = self.operand2_parsed
        elif self.op in Instruction.UJMP_OPS:
            self.dst_bbn = self.operand1_parsed

class DFAFramework(object):
    """
    docstring for DFAFramework
    direction, merge_op, trans_func, top, bottom
    """
    def __init__(self, *args, **kwargs):
        super(DFAFramework, self).__init__()
        for i, arg in enumerate(args):
            setattr(self, 'arg_' + str(i), arg)
        for kw, arg in kwargs.items():
            setattr(self, kw, arg)
    def run(self, cfg):



class DFA(object):
    def __init__(self):
        super(DFA, self).__init__()

    def _read_3addr_code_from_file(self, file_name, skip_num_rows=0):
        '''
        skip_num_rows: default value is 0.
        Although the first line is "compiling examples/*.c", this line is not included if we redirect the output to file.
        '''
        self.instr_strs = list()
        with open(file_name) as fin:
            self.instr_strs = [line.strip() for line in fin.readlines()]
            self.instr_strs = self.instr_strs[skip_num_rows:]
        return self

    def _init_analysis(self):
        try:
            self.num_instrs = len(self.instr_strs)
            self.instrs = list()
            for instr_str in self.instr_strs:
                cols = instr_str.split()
                cols.extend([''] * (5 - len(cols)))
                self.instrs.append(Instruction(int(cols[1][:-1]), cols[2], cols[3], cols[4]))
        except Exception, e:
            print e
            print 'Bad instructions in the code!'
        return self

    def create_CFG(self):
        '''
        Branch instructions. The opcodes are br, blbc, blbs, and call.
        '''
        self.cfg = CFG()
        self.leaders = set()
        for instr in self.instrs:
            if instr.op in Instruction.LEADER_OPS:
                self.leaders.add(instr.instr_id)
            if instr.op in Instruction.TERMINATOR_OPS:
                leader = instr.instr_id + 1
                if leader <= self.num_instrs: # and self.instrs[leader - 1].op not in Instruction.INVALID_LEADER_OPS:
                    self.leaders.add(leader)
                if instr.op in Instruction.JMP_OPS:
                    self.leaders.add(instr.dst_bbn)

        self.leaders = sorted(self.leaders)
        func_instr_id = None
        for leader in self.leaders:
            instr = self.instrs[leader - 1]
            if instr.op in Instruction.INVALID_LEADER_OPS:
                continue
            if instr.op == Instruction.PROC_START_OP:
                func_instr_id = instr.instr_id
            next_instr_id = leader + 1
            while next_instr_id <= self.num_instrs and next_instr_id not in self.leaders:
                next_instr_id += 1
            ed_instr_id = next_instr_id - 1
            bb = BasicBlock(leader, ed_instr_id, func_instr_id)
            self.cfg.add_basic_block(bb)
            ed_instr = self.instrs[ed_instr_id - 1]
            if ed_instr.op in Instruction.BR_OPS: # dont consider calls and returns
                self.cfg.add_edge(leader, ed_instr.dst_bbn)
            if next_instr_id <= self.num_instrs and ed_instr.op not in Instruction.UBR_OPS + Instruction.RET_OPS:
                self.cfg.add_edge(leader, next_instr_id)

        return self

if __name__ == '__main__':
    fn = './examples/gcd.c.3addr'
    d = DFA()
    d._read_3addr_code_from_file(fn)
    d._init_analysis()
    d.create_CFG()
    d.cfg.display()

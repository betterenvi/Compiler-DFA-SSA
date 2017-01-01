import sys, os, re, collections, copy

#from cfg import *

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
    CMP_OPS = ['cmpeq', 'cmple', 'cmplt']
    CALC_OPS = ['add', 'sub', 'mul', 'div', 'mod', 'neg']
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
        self.left = set()
        self.right = ''
        self._parse()
        self._calc_rd_gen_kill()
        self._calc_ae_gen_kill()
        self._calc_lv_gen_kill()

    def __repr__(self):
        res = 'instr %s:\t' % (self.instr_id)
        if self.op in Instruction.ARITH_OPS + ['move', 'load', 'store']:
            res += '%s = %s' % (' = '.join(self.left), self.right)
        else:
            res += ' '.join([self.op, self.operand1, self.operand2])
        return res

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
            self.left.add('r%s' % self.instr_id)
        elif self.op == 'move':
            self.right = self.operand1_parsed
            self.left.add(self.operand2_parsed)
            self.left.add('r%s' % self.instr_id)
        elif self.op == 'load':
            self.right = self.operand1_parsed
            self.left.add('r%s' % self.instr_id)
        elif self.op == 'store':
            self.right = self.operand1_parsed
            self.left.add('*(%s)' % self.operand2_parsed)
        elif self.op in Instruction.CJMP_OPS:
            self.dst_bbn = self.operand2_parsed
        elif self.op in Instruction.UJMP_OPS:
            self.dst_bbn = self.operand1_parsed

    def _calc_rd_gen_kill(self):
        self.RD_VAR_KILL = self.left
        self.RD_KILL = set()
        self.RD_GEN = set() if self.op == 'store' else {(self.instr_id, lft) for lft in self.left}

    def _calc_ae_gen_kill(self):
        self.AE_VAR_KILL = self.left
        self.AE_KILL = set()
        self.AE_GEN = set()
        if self.op in Instruction.ARITH_OPS:
            tmp = self.right.split(' ')
            rights = [tmp[0], tmp[2]]
            for lf in self.left:
                if lf in rights:
                    self.AE_KILL.add(self.right)
                    return
            self.AE_GEN.add(self.right)

    def _calc_lv_gen_kill(self):
        is_variable = lambda x : x not in ['GP', 'FP'] and re.match('\d+', x) == None
        self.LV_GEN = set()
        if self.op in Instruction.ARITH_OPS:
            tmp = self.right.split(' ')
            for i in [0, 2]:
                if is_variable(tmp[i]):
                    self.LV_GEN.add(tmp[i])
        elif self.op in ['move', 'load']:
            self.LV_GEN.add(self.right)
        elif self.op == 'store':
            if is_variable(self.right):
                self.LV_GEN.add(self.right)
            if is_variable(self.operand2_parsed):
                self.LV_GEN.add(self.operand2_parsed)
        self.LV_KILL = self.left - self.LV_GEN


class DFAFramework(object):
    """
    docstring for DFAFramework
    kwargs:
    instrs, cfg, direction
    """
    def __init__(self, *args, **kwargs):
        super(DFAFramework, self).__init__()
        self._init_params(*args, **kwargs)
        self._set_merge_trans_func()
        self._calc_top_bottom()
        self._calc_gen_kill()

    def _init_params(self, *args, **kwargs):
        for i, arg in enumerate(args):
            setattr(self, 'arg_' + str(i), arg)
        for kw, arg in kwargs.items():
            setattr(self, kw, arg)

        self.iter_IN = collections.defaultdict(set)
        self.iter_OUT = collections.defaultdict(set)
        if self.direction == 'forward':
            self.iter_heads_of_func = self.cfg.heads_of_func
            self.predecessors = self.cfg.predecessors
            self.successors = self.cfg.successors
        else:
            self.iter_heads_of_func = self.cfg.tails_of_func
            self.predecessors = self.cfg.successors
            self.successors = self.cfg.predecessors

    def _set_merge_trans_func(self):
        pass

    def _calc_top_bottom(self):
        self.top = set()
        self.bottom = set()

    def _calc_gen_kill(self):
        self.GEN = collections.defaultdict(set)
        self.KILL = collections.defaultdict(set)

    def _init_analysis(self):
        self.init_bbns = set()
        for bbn in self.cfg.bbs.keys():
            self.iter_IN[bbn] |= self.top
            self.iter_OUT[bbn] |= self.top
        for func, bbns in self.iter_heads_of_func.items():
            self.init_bbns.update(bbns)
        for bbn in self.init_bbns:
            self.iter_IN[bbn] &= set()
            self.iter_OUT[bbn] = self.trans_func(self.iter_IN[bbn], self.GEN[bbn], self.KILL[bbn])
        self.updating_queue = list(self.init_bbns)

    def _iterate(self):
        while len(self.updating_queue) > 0:
            bbn = self.updating_queue.pop(0)
            for successor in self.successors[bbn]:
                self.iter_IN[successor] = self.merge_func(*[self.iter_IN[successor], self.iter_OUT[bbn]])
                new_out = self.trans_func(self.iter_IN[successor], self.GEN[successor], self.KILL[successor])
                if new_out != self.iter_OUT[successor]:
                    self.iter_OUT[successor] = new_out
                    self.updating_queue.append(successor)

        if self.direction == 'forward':
            self.IN = self.iter_IN
            self.OUT = self.iter_OUT
        else:
            self.IN = self.iter_OUT
            self.OUT = self.iter_IN

    def run(self):
        self._init_analysis()
        self._iterate()

class ReachingDefinitionAnalysis(DFAFramework):
    """docstring for ReachingDefinitionAnalysis"""
    def __init__(self, direction='forward', *args, **kwargs):
        kwargs['direction'] = direction
        super(ReachingDefinitionAnalysis, self).__init__(*args, **kwargs)

    def _set_merge_trans_func(self):
        def merge_func(*args):
            if len(args) == 0:
                return set()
            res = args[0]
            for i in range(1, len(args)):
                res |= args[i]
            return res

        def trans_func(x, gen, kill):
            return (x - kill) | gen

        self.merge_func = merge_func
        self.trans_func = trans_func

    def _calc_top_bottom(self):
        self.top = set()
        self.bottom = set()
        for instr in self.instrs:
            self.bottom.update(instr.RD_GEN)

    def _get_rd_kill(self, lefts):
        res = set()
        for bt in self.bottom: # bt is a tuple of (instr_id, left)
            if bt[1] in lefts:
                res.add(bt)
        return res

    def _calc_gen_kill(self):
        self.GEN = collections.defaultdict(set)
        self.KILL = collections.defaultdict(set)
        for instr in self.instrs:
            instr.RD_KILL.update(self._get_rd_kill(instr.left) - instr.RD_GEN)

        for bbn, bb in self.cfg.bbs.items():
            st, ed = bb.st_instr_id, bb.ed_instr_id
            idx = st
            while idx <= ed:
                instr = self.instrs[idx - 1]
                self.KILL[bbn] |= instr.RD_KILL
                self.KILL[bbn] -= instr.RD_GEN
                self.GEN[bbn] = self.trans_func(self.GEN[bbn], instr.RD_GEN, instr.RD_KILL)
                idx += 1


class AvailableExpressionAnalysis(DFAFramework):
    """docstring for AvailableExpressionAnalysis"""
    def __init__(self, direction='forward', *args, **kwargs):
        kwargs['direction'] = direction
        super(AvailableExpressionAnalysis, self).__init__(*args, **kwargs)

    def _set_merge_trans_func(self):
        def merge_func(*args):
            if len(args) == 0:
                return set()
            res = args[0]
            for i in range(1, len(args)):
                res &= args[i]
            return res

        def trans_func(x, gen, kill):
            return (x - kill) | gen

        self.merge_func = merge_func
        self.trans_func = trans_func

    def _calc_top_bottom(self):
        self.bottom = set()
        self.top = set()
        for instr in self.instrs:
            self.top.update(instr.AE_GEN)

    def _get_ae_kill(self, lefts):
        res = set()
        for expression in self.top:
            tmp = expression.split(' ')
            for left in lefts:
                if left in tmp:
                    res.add(expression)
        return res

    def _calc_gen_kill(self):
        self.GEN = collections.defaultdict(set)
        self.KILL = collections.defaultdict(set)
        for instr in self.instrs:
            instr.AE_KILL.update(self._get_ae_kill(instr.left))

        for bbn, bb in self.cfg.bbs.items():
            st, ed = bb.st_instr_id, bb.ed_instr_id
            idx = st
            while idx <= ed:
                instr = self.instrs[idx - 1]
                self.KILL[bbn] |= instr.AE_KILL
                self.GEN[bbn] = self.trans_func(self.GEN[bbn], instr.AE_GEN, instr.AE_KILL)
                idx += 1

class LiveVariableAnalysis(DFAFramework):
    """docstring for LiveVariableAnalysis"""
    def __init__(self, direction='backward', *args, **kwargs):
        kwargs['direction'] = direction
        super(LiveVariableAnalysis, self).__init__(*args, **kwargs)

    def _set_merge_trans_func(self):
        def merge_func(*args):
            if len(args) == 0:
                return set()
            res = args[0]
            for i in range(1, len(args)):
                res &= args[i]
            return res

        def trans_func(x, gen, kill):
            return (x - kill) | gen

        self.merge_func = merge_func
        self.trans_func = trans_func

    def _calc_top_bottom(self):
        self.bottom = set()
        self.top = set()
        for instr in self.instrs:
            self.top.update(instr.LV_GEN)

    def _calc_gen_kill(self):
        self.GEN = collections.defaultdict(set)
        self.KILL = collections.defaultdict(set)
        for bbn, bb in self.cfg.bbs.items():
            st, ed = bb.st_instr_id, bb.ed_instr_id
            idx = ed
            while idx >= st:
                instr = self.instrs[idx - 1]
                self.KILL[bbn] |= instr.LV_KILL
                self.KILL[bbn] -= instr.LV_GEN
                self.GEN[bbn] = self.trans_func(self.GEN[bbn], instr.LV_GEN, instr.LV_KILL)
                idx -= 1

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

    def display_instrs(self):
        for instr in self.instrs:
            print instr

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

        self.cfg.construct()
        return self

    def run_rda(self):
        '''
        Reaching Definition Analysis
        '''
        self.rda = ReachingDefinitionAnalysis(instrs=self.instrs, cfg=self.cfg)
        self.rda.run()

    def run_aea(self):
        '''
        Available Expression Analysis
        '''
        self.aea = AvailableExpressionAnalysis(instrs=self.instrs, cfg=self.cfg)
        self.aea.run()

    def run_lva(self):
        '''
        Live Variable Analysis
        '''
        self.lva = LiveVariableAnalysis(instrs=self.instrs, cfg=self.cfg)
        self.lva.run()

if __name__ == '__main__':
    fn = './examples/gcd.c.3addr'
    d = DFA()
    d._read_3addr_code_from_file(fn)
    d._init_analysis()
    d.create_CFG()
    d.cfg.display()
    d.run_rda()
    d.run_lva()
    d.run_aea()

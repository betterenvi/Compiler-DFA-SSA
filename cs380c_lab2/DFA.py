import sys, os, re, collections, copy

from cfg import *
from rda import ReachingDefinitionAnalysis as RDA
from aea import AvailableExpressionAnalysis as AEA
from lva import LiveVariableAnalysis as LVA

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
        self.rda = RDA(instrs=self.instrs, cfg=self.cfg)
        self.rda.run()

    def run_aea(self):
        '''
        Available Expression Analysis
        '''
        self.aea = AEA(instrs=self.instrs, cfg=self.cfg)
        self.aea.run()

    def run_lva(self):
        '''
        Live Variable Analysis
        '''
        self.lva = LVA(instrs=self.instrs, cfg=self.cfg)
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

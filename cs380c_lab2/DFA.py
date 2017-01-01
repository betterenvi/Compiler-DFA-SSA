import sys, os, re, collections, copy

from cfg import *
from instruction import *
from rda import ReachingDefinitionAnalysis as RDA
from aea import AvailableExpressionAnalysis as AEA
from lva import LiveVariableAnalysis as LVA

class DFA(object):
    def __init__(self, instrs):
        super(DFA, self).__init__()
        self.instrs = instrs
        self.num_instrs = len(instrs)
        self._create_cfg()

    def _create_cfg(self):
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

    def display_cfg(self):
        self.cfg.display()

    def display_instrs(self):
        for instr in self.instrs:
            print instr

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
    instrs = read_instrs(fn)
    d = DFA(instrs)
    d.display_cfg()
    d.run_rda()
    d.run_lva()
    d.run_aea()

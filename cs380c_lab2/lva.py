import re, collections, copy

from cfg import *
from dfa_framework import DFAFramework

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

    def _calc_iter_IN_OUT_for_instr(self):
        super(LiveVariableAnalysis, self)._calc_iter_IN_OUT_for_instr(gen_attr='LV_GEN', kill_attr='LV_KILL')

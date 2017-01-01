import re, collections, copy

from cfg import *
from dfa_framework import DFAFramework

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

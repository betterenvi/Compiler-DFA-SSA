import re, collections, copy

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

    def _optimize(self):
        self.num_statements_eliminated_SCR = collections.defaultdict(int)
        self.num_statements_eliminated_not_SCR = collections.defaultdict(int)
        for instr in self.instrs:
            lv_in_lefts = instr.lefts & self.iter_IN_instr[instr.instr_id]
            if 0 == len(lv_in_lefts) < len(instr.lefts):
                print 'dse', instr.instr_id, lv_in_lefts, self.iter_IN_instr[instr.instr_id], instr.lefts
                # for left in self.iter_IN_instr[instr.instr_id]:
                #     print left
                self.num_statements_eliminated_SCR[instr.func_instr_id] += 1

    def _report(self):
        for func in self.cfg.bbns_of_func.keys():
            print 'Function: %d' % func
            print 'Number of statements eliminated in SCR: %d' % self.num_statements_eliminated_SCR[func]
            print 'Number of statements eliminated not in SCR: %d' % self.num_statements_eliminated_not_SCR[func]


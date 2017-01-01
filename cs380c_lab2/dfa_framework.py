import sys, os, re, collections, copy

from cfg import *

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
        self._init_bbns = set()
        for bbn in self.cfg.bbs.keys():
            self.iter_IN[bbn] |= self.top
            self.iter_OUT[bbn] |= self.top
        for func, bbns in self.iter_heads_of_func.items():
            self._init_bbns.update(bbns)
        for bbn in self._init_bbns:
            self.iter_IN[bbn] &= set()
            self.iter_OUT[bbn] = self.trans_func(self.iter_IN[bbn], self.GEN[bbn], self.KILL[bbn])
        self._updating_queue = list(self._init_bbns)

    def _iterate(self):
        while len(self._updating_queue) > 0:
            bbn = self._updating_queue.pop(0)
            for successor in self.successors[bbn]:
                self.iter_IN[successor] = self.merge_func(*[self.iter_IN[successor], self.iter_OUT[bbn]])
                new_out = self.trans_func(self.iter_IN[successor], self.GEN[successor], self.KILL[successor])
                if new_out != self.iter_OUT[successor]:
                    self.iter_OUT[successor] = new_out
                    self._updating_queue.append(successor)

        if self.direction == 'forward':
            self.IN = self.iter_IN
            self.OUT = self.iter_OUT
        else:
            self.IN = self.iter_OUT
            self.OUT = self.iter_IN

    def run(self):
        self._init_analysis()
        self._iterate()

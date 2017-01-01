import collections
class BasicBlock(object):
    def __init__(self, st_instr_id, ed_instr_id, func_instr_id):
        super(BasicBlock, self).__init__()
        self.name = st_instr_id
        self.st_instr_id = st_instr_id
        self.ed_instr_id = ed_instr_id
        self.func_instr_id = func_instr_id

class CFG(object):
    '''
    Abbreviations:
    - bb: basic block
    - bbn: basic block name
    '''
    def __init__(self):
        super(CFG, self).__init__()
        self.bbns_of_func = collections.defaultdict(set)
        self.bbs = dict()
        self.edges = collections.defaultdict(list)
        self.predecessors = collections.defaultdict(set)
        self.successors = collections.defaultdict(set)

    def add_basic_block(self, bb):
        self.bbs[bb.name] = bb
        self.bbns_of_func[bb.func_instr_id].add(bb.name)
        return self

    def add_edge(self, src_bb, dst_bb):
        '''
        src_bb and dst_bb can be strings, which enables adding edges before relevant bbs are created!
        '''
        src_bbn = src_bb if not isinstance(src_bb, BasicBlock) else src_bb.name
        dst_bbn =  dst_bb if not isinstance(dst_bb, BasicBlock) else dst_bb.name
        # if self.bbs[src_bbn].func_instr_id != self.bbs[dst_bbn].func_instr_id:
        #     return self
        self.edges[src_bbn].append(dst_bbn)
        self.predecessors[dst_bbn].add(src_bbn)
        self.successors[src_bbn].add(dst_bbn)
        return self

    def _sort(self):
        for func in self.bbns_of_func.keys():
            self.bbns_of_func[func] = sorted(self.bbns_of_func[func])
        for src_bbn in self.edges.keys():
            self.edges[src_bbn]  = sorted(self.edges[src_bbn])
        return self

    def display(self):
        self._sort()
        for func in sorted(self.bbns_of_func.keys()):
            bbns = self.bbns_of_func[func]
            print 'Function: %s\nBasic blocks: %s\nCFG:\n' % (func, ' '.join(map(str, bbns)))
            for src_bbn in bbns:
                dst_bbns = self.edges[src_bbn]
                print '%s ->%s\n' % (src_bbn, '' if len(dst_bbns) == 0 else (' ' + ' '.join(map(str, dst_bbns))))
        return self

    def _strongconnect(self, bbn):
        self.lowlink[bbn] = self.Tarjan_cnt
        self.index[bbn] = self.Tarjan_cnt
        self.Tarjan_cnt += 1
        self.bbns_stack.append(bbn)
        for successor in self.successors[bbn]:
            if successor not in self.index:
                self._strongconnect(successor)
            self.lowlink[bbn] = min(self.lowlink[bbn], self.lowlink[successor])

        if self.index[bbn] == self.lowlink[bbn]:
            while True:
                w = self.bbns_stack.pop()
                self.scrs[bbn].add(w)
                if w == bbn:
                    break

    def _Tarjan(self):
        '''
        Tarjan Algorithm
        '''
        self.scrs = collections.defaultdict(set)
        self.bbns_stack = list()
        self.lowlink = dict()
        self.index = dict()
        self.Tarjan_cnt = 0

        for bbn in self.bbs:
            if bbn not in self.index:
                self._strongconnect(bbn)

        for k in self.scrs:
            self.scrs[k] = sorted(self.scrs[k])

    def SCR_analysis(self):
        '''
        Perform Strongly Connected Region (SCR) analysis.
        Given a CFG G = (N, E, h), an SCR is a nonempty set of nodes S \Subseteq N,
        for which, given any q, r, \in S, there exists a path from q to r and from r to q.
        '''
        self._Tarjan()

    def calc_heads_tails_of_func(self):
        self.heads_of_func = collections.defaultdict(set)
        self.tails_of_func = collections.defaultdict(set)
        for func, bbns in self.bbns_of_func.items():
            for bbn in bbns:
                if len(self.predecessors[bbn]) == 0:
                    self.heads_of_func[func].add(bbn)
                if len(self.successors[bbn]) == 0:
                    self.tails_of_func[func].add(bbn)







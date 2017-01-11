# !/usr/bin/python
# coding=utf-8
import sys, getopt, os
from dfa import DFA
from instruction import read_instrs

default_args = {'-c':0, '-s':0, '-r':0, '-l':0, '3addr_code_file_name':'examples/gcd.c.3addr'}

def usage():
    print '\nUsage: python main.py [-c] [-r] [-l] <3addr_code_file_name>'
    print '''\nOptions:\n
    -c, print Control Flow Graph\n
    -r, perform reaching definitions and use it to perform simple constant propagation\n
    -l, perform live variable analysis and use it to perform dead statement elimination\n
    -h, help\n
    '''
    # -s, prform Strongly Connected Region (SCR) analysis\n

def parse_args(args, default_args):
    try:
        optlist, args = getopt.getopt(args, 'csrl')
        opt_dict = {k:v for k, v in optlist}
        for k in default_args:
            opt_dict[k] = default_args[k] if k not in opt_dict else 1
        opt_dict['3addr_code_file_name'] = default_args['3addr_code_file_name'] if len(args) == 0 else args[0]
        return opt_dict
    except Exception, e:
        # print e
        usage()
        exit()

def main():
    if len(sys.argv) == 1 or '-h' in sys.argv:
        usage()
        exit()
    opt_dict = parse_args(sys.argv[1:], default_args)
    if not os.path.exists(opt_dict['3addr_code_file_name']):
        print '{} does not exist.'.format(opt_dict['3addr_code_file_name'])
        exit()

    instrs = read_instrs(opt_dict['3addr_code_file_name'])
    d = DFA(instrs)
    if opt_dict['-c'] == 1:
        d.display_cfg()
    if opt_dict['-r'] == 1:
        d.run_rda()
    # d.run_aea()
    if opt_dict['-l'] == 1:
        d.run_lva()

if __name__ == '__main__':
    main()

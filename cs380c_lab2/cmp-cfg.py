# !/usr/bin/python
import os, re
import commands

for fn in os.listdir('./examples/'):
    if re.match(".*c$", fn) == None:
        continue
    ta_fn = './examples/' + re.sub('.c$', '.ta.cfg', fn)
    my_fn = './examples/' + fn + '.3addr'
    my_md5 = commands.getoutput('python main.py -c %s | md5sum' % my_fn).split(' ')[0]
    ta_md5 = commands.getoutput('md5sum %s' % ta_fn).split(' ')[0]
    print '{:<15}'.format(fn), 'Correct' if my_md5 == ta_md5 else 'Wrong'

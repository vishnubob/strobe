#!/usr/bin/env python

import math

def make_table(steps, amp):
    rad = (math.pi) / steps
    for x in range(steps):
        yield int(math.sin(x * rad) * amp)

tab = list(make_table(200, 400))
tab = list(make_table(200, -400))
print tab

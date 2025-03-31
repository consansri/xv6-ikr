# say hello via UART via ULPI
print('hello world!', list(x + 1 for x in range(10)), end='eol\\n')

for i in range(10):
    print('iter {:08}'.format(i))

try:
    1//0
except Exception as er:
    print('caught exception', repr(er))

import gc
print('run GC collect')
gc.collect()

print('finish')
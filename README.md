wrap_malloc
===========

wrap_malloc is a memory profiler library for linux. Suitable for x86 and arm. x86_64 is not supported yet. The main approach is following. All calls to malloc/calloc/realloc/free are redirected to the wrap_malloc library and every call is saved to a binary log file. After the logs are collected there is a parser to find any memory leaks. 

**How to build:**

autoreconf
./configure --host=arm-linux-gnueabi --enable-custom-log-path="/path/to/log/files/"
make

arm-linux-gnueabi-g++ -g -O0 -rdynamic -fno-omit-frame-pointer -mapcs-frame -funwind-tables -Wl,-wrap,malloc -Wl,-wrap,calloc -Wl,-wrap,realloc -Wl,-wrap,free main.cpp -L. -lwrap_arm -lpthread

or

autoreconf
./configure --enable-32 --enable-custom-log-path="/path/to/log/files/"
make

g++ -g -O0 -rdynamic -fno-omit-frame-pointer -m32 -Wl,-wrap,malloc -Wl,-wrap,calloc -Wl,-wrap,realloc -Wl,-wrap,free main.cpp -L. -lwrap_32 -lpthread

**How to parse log files:**

./parser.py /path/to/log/files/ ../your_application.bin arm --final

try help:
./parser.py --help

Dependencies:
-------------

https://pypi.python.org/pypi/humanize
libpthread


License:
-------------

The MIT License (MIT)

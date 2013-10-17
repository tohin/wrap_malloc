#!/usr/bin/env python

#  The MIT License (MIT)
#
#  Copyright (c) 2013 tohin, belko
#
#  Permission is hereby granted, free of charge, to any person obtaining a copy of
#  this software and associated documentation files (the "Software"), to deal in
#  the Software without restriction, including without limitation the rights to
#  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
#  the Software, and to permit persons to whom the Software is furnished to do so,
#  subject to the following conditions:
#
#  The above copyright notice and this permission notice shall be included in all
#  copies or substantial portions of the Software.
#
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
#  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
#  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
#  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
#  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

import struct
import subprocess
import os
import argparse

from collections import Counter

import humanize

ADDR2LINE_BIN = ''
SYM_FILE = ''

eALLOC = 1
eDEALLOC = 2

addrs = {}


def addr2line(addr):
    try:
        return addrs[addr]
    except:
        value = execute((ADDR2LINE_BIN, '-e', SYM_FILE, '-f', '-C', hex(addr)))
        value = hex(addr) + ' ' + value.replace('\n', ' ') + '\n'
        addrs[addr] = value
        return value


def execute(exe):
    p = subprocess.Popen(exe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    result = ''
    while(True):
        retcode = p.poll()  # returns None while subprocess is running
        line = p.stdout.readline()
        result += line
        if(retcode is not None):
            break
    return result


class record():

    FORCE_STACK_COMPARE = False

    def __init__(self):
        self.type = -1
        self.pointer = -1
        self.size = -1
        self.nFrames = -1
        self.stack = None
        self.stack_printable = None
        self.occurrence = []

    def parse(self, data):
        self.set_values(struct.unpack('>' + 'I' * (len(data)/4), data))

    def set_values(self, values):
        self.type = values[0]
        self.pointer = values[1]

        if self.type == eALLOC:
            self.size = values[2]
            self.nFrames = values[3]
            self.stack = tuple(values[4:])
        elif self.type == eDEALLOC:
            self.nFrames = values[2]
            self.stack = tuple(values[3:])

    def parse_stack(self):
        if not self.stack_printable:
            new_stack = []
            for addr in self.stack:
                addr = addr2line(addr)
                new_stack.append(addr)
            self.stack_printable = new_stack

    def get_total(self):
        if len(self.occurrence) > 0:
            return self.occurrence[-1] * self.size
        else:
            return 0

    def __str__(self):
        return "%s: %s at %s, Total %s, Occurrence %s\n %s" % (['[M]', '[F]'][self.type-1], humanize.naturalsize(self.size, binary=True), hex(self.pointer), humanize.naturalsize(self.get_total(), binary=True), self.occurrence, ' '.join(self.stack_printable))

    def __eq__(self, other):
        return self.__hash__() == other.__hash__()

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        if record.FORCE_STACK_COMPARE:
            return hash(self.stack)
        else:
            return hash(self.pointer)


def parse_and_emulate(data_generator, allocated_records={}):
    for chunk in data_generator:
        r = record()

        try:
            r.parse(chunk)
        except:
            print 'UNEXPECTED END OF FILE'
            break

        if r.type == eDEALLOC:
            try:
                del allocated_records[r.pointer]
            except:
                pass
                # print 'BAD FREE'
                # r.parse_stack()
                # print r
                # print ''

        elif r.type == eALLOC:
            if r.pointer not in allocated_records:
                allocated_records[r.pointer] = r
            else:
                print 'BAD ALLOC'
                r.parse_stack()
                allocated_records[r.pointer].parse_stack()
                print 'old =', allocated_records[r.pointer]
                print 'new =', r
                print ''

    return allocated_records


def bytes_from_file(filename):
    with open(filename, "rb") as f:
        while True:
            raw_size = f.read(4)
            if not raw_size:
                break
            size = struct.unpack('>I', raw_size)[0]
            chunk = f.read(size)
            if chunk:
                yield chunk
            else:
                break


def main():
    global ADDR2LINE_BIN
    global SYM_FILE

    parser = argparse.ArgumentParser(description='Parser of logs that created by wrap_malloc lib')
    parser.add_argument('log_folder_name', metavar='LOG_FOLDER_NAME', type=str, help='Path to folder that contains memory logs')
    parser.add_argument('symbol_file', metavar='SYM_FILE', type=str, help='Symbol file for the executable')
    parser.add_argument('arch', metavar='ARCH', type=str, choices=('arm', 'x86'), help='"arm" or "x86"')
    parser.add_argument('--final', dest='final', action='store_true', default=False, help='Print memory leak objects if the logs colleced till the exit from main function of the process')
    args = parser.parse_args()

    SYM_FILE = args.symbol_file

    if args.arch == 'x86':
        ADDR2LINE_BIN = 'addr2line'
    elif args.arch == 'arm':
        ADDR2LINE_BIN = 'arm-linux-gnueabi-addr2line'

    # Find log files
    indexes = []
    for file_name in os.listdir('./%s/' % args.log_folder_name):
        if file_name.startswith('memory_'):
                indexes.append(int(file_name[7:-4]))

    indexes = sorted(indexes)

    file_names = []
    for index in indexes:
        file_names.append('./%s/memory_%d.raw' % (args.log_folder_name, index))

    # Start emulate
    memory_snapshots = []

    for count, raw_file in enumerate(file_names):

        if count > 0:
            memory_snapshot = memory_snapshots[count-1].copy()
        else:
            memory_snapshot = {}

        data_generator = bytes_from_file(raw_file)

        memory_snapshot = parse_and_emulate(data_generator, memory_snapshot)

        total_mem = 0

        for addr in memory_snapshot:
            total_mem += memory_snapshot[addr].size

        print 'EMULATED FILE', count, 'OBJECTS =', len(memory_snapshot), 'TOTAL SIZE =', "{:,}".format(total_mem), 'Bytes'

        memory_snapshots.append(memory_snapshot)

    print 'EMULATING DONE'

    record.FORCE_STACK_COMPARE = True

    file_objects = []

    for snapshot in memory_snapshots:
        objects = []
        for addr in snapshot:
            objects.append(snapshot[addr])
        file_objects.append(Counter(objects))

    uobjects = set([obj for file_object in file_objects for obj in file_object])

    for file_object in file_objects:
        for uobj in uobjects:
            uobj.occurrence.append(file_object[uobj])

    # Filter out some objects to find memory leak
    possible_leaks = set()
    for uobj in uobjects:
        if uobj.occurrence[-1] != 1 and uobj.occurrence[0] < uobj.occurrence[-1] and len(set(uobj.occurrence)) > 3:
            uobj.parse_stack()
            possible_leaks.add(uobj)
    possible_leaks = sorted(possible_leaks, key=lambda leak: leak.occurrence[-1])

    if args.final:
        leaks = set()
        for uobj in uobjects:
            if uobj.occurrence[-1] != 0:
                uobj.parse_stack()
                leaks.add(uobj)

    print '################################################################################'
    print 'POSSIBLE LEAK', len(possible_leaks)
    for leak in possible_leaks:
        print leak

    if args.final:
        print '################################################################################'
        print 'LEAK', len(leaks)
        for leak in leaks:
            print leak


if __name__ == '__main__':
    main()

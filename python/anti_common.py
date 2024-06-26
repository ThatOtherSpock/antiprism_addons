#!/usr/bin/python3

'''
Common code for python in Antiprism Addons

Written by Roger Kaufman <polyhedrasmith@gmail.com>
'''
import os
import sys
import subprocess

n = 0
d = 0

def run_proc(proc, verbose=False, debug=False):
    # open interactive bash using -i
    #https://stackoverflow.com/questions/6856119/can-i-use-an-alias-to-execute-a-program-from-a-python-script
    # tack exit onto end of proc so it exits bash shell. silence the exit.
    proc = proc + "; exit 2>/dev/null"
    if (debug):
      print("proc =", proc, flush=True, file=sys.stderr)
    sp = subprocess.run(["/bin/bash", "-i", "-c", proc], stderr=(sys.stderr if (verbose) else subprocess.DEVNULL))
    return sp

def run_proc_data(proc, debug=False):
    proc = proc + ";exit 2>/dev/null"
    if (debug):
      print("proc =", proc, flush=True, file=sys.stderr)
    #https://stackoverflow.com/questions/2502833/store-output-of-subprocess-popen-call-in-a-string
    sp = subprocess.run(["/bin/bash", "-i", "-c", proc], capture_output=True, text=True)
    return sp

def do_cmd(cmd, verbose=False):
    if (verbose):
      print("cmd =", cmd, flush=True, file=sys.stderr)
    os.system(cmd)

# model is model file or built-in name
# work_dir may be generated by tempfile.TemporaryDirectory() or anywhere
# work_file is the file name of the output placed in work_dir
def read_off_file(model, work_dir, work_file):
    # read from file argument or stdin
    off_file = work_dir.name + "/off_file.off"
    work_file =  work_dir.name + "/" + work_file

    # try using the input file name or built in model name
    # if no file name was given try reading from standard input
    # will hang if nothing at standard input
    if (model == ""):
      fout = open(off_file, "w")
      fout.writelines(sys.stdin.readlines())
      fout.close()
    # else there is an input file or built in file name
    else:
      #if there is a dot in filename it is likely a .off file
      if ('.' in model):
        # off_util will validate the file
        proc = 'cat "%s" | off_util > %s' % (model, off_file)
      #if there is no dot in filename it is likely a built-in model
      else:
        proc = 'off_util "%s" > %s 2>/dev/null' % (model, off_file)

      sp = run_proc(proc, True)
      if (sp.returncode):
        print(f"{__file__}: error: input file '%s' not found" % model, file=sys.stderr)
        sys.exit(1)

    # rename off_file.off to expected name
    run_proc('mv %s %s' % (off_file, work_file))

def split_on_comma(string1):
    parts = string1.split(',')
    return parts[1]

def off_query(operation, elem_type, elem_num, model):
    proc = "off_query -c C " + str(elem_type) + str(operation) + " -I " + str(elem_num) + " " + str(model)
    #https://raspberrypi.stackexchange.com/questions/71547/is-there-a-problem-with-using-deprecated-os-popen
    # os.popen doesn't read .bashrc
    #output = os.popen(proc).readline()

    # tack exit onto end of proc so it exits bash shell
    proc = proc + ";exit"
    sp = run_proc_data(proc)

    # returncode can work
    #if (sp.returncode):
    if (sp.stdout == ""):
      print(f"{__file__}: error: No output from off_query", file=sys.stderr)
      sys.exit(1)

    return sp.stdout.strip()

def read_polygon(a, limit = 2):
    global n, d

    fraction = a.split('/')
    n = int(fraction[0])
    if len(fraction) > 1:
      d = int(fraction[1])
    else:
      d = 1

    if(n < limit):
        print(f"{__file__}: error: number of sides: '%d' is not '%d' or greater" % (n, limit), file=sys.stderr)
        sys.exit(1)
    if(d < 1):
        print(f"{__file__}: error: denominator of sides: '%d' is not 1 or greater" % d, file=sys.stderr)
        sys.exit(1)
    if(n == d):
        print(f"{__file__}: error: number of sides cannot equal denominator of sides: '%d/%d'" % (n,d), file=sys.stderr)
        sys.exit(1)

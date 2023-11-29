#!/usr/bin/python3

# BSD Zero Clause License
#
# Copyright (c) 2023 Roger Kaufman <polyhedrasmith@gmail.com>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
# REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
# INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.

'''
CORS Web Server provided for use with webview.py

Written by Roger Kaufman <polyhedrasmith@gmail.com>
'''
import os
import sys
import time
import signal
import http.server
import argparse

#https://www.gangofcoders.net/solution/in-python-using-argparse-allow-only-positive-integers/
# works for both integer and float
def positive_or_zero(numeric_type):
    def require_positive_or_zero(value):
        number = numeric_type(value)
        if number < 0:
            raise argparse.ArgumentTypeError(f"Number {value} must be positive or zero.")
        return number

    return require_positive_or_zero

def port_checker(a):
    num = int(a)

    if num < 0 or num > 65535:
        raise argparse.ArgumentTypeError("port must be between 0 an 65535")
    return num

# defaults are set here
__version__ = 2.3
default_port = 8080
default_sleep = 0

#https://stackoverflow.com/questions/3853722/how-to-insert-newlines-on-argparse-help-text#comment10098369_3853776
parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter, description=__doc__)

parser.add_argument('-port', '--port', type=port_checker, metavar=("{number from 0 to 65535}"), default=default_port,
                    help='port number for server (default: %(default)s)')

parser.add_argument('-sleep', '--sleep', type=positive_or_zero(float), default=default_sleep,
                    help='time in seconds before server shutdown (0 to run forever) (default: %(default)s)')

parser.add_argument('--version', action='version', version='%(prog)s {version}'.format(version=__version__))

args = parser.parse_args()

# fork and kill process after n seconds. if sleep is 0, run forever
if args.sleep != 0:
  pid = os.fork()
  # if child process
  if pid == 0 :
    time.sleep(args.sleep)
    # kill parent process
    os.kill(os.getppid(), signal.SIGTERM)
    sys.exit(0)

# use CORS
# https://gist.github.com/acdha/925e9ffc3d74ad59c3ea

from http.server import HTTPServer, SimpleHTTPRequestHandler

class CORSRequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        return super(CORSRequestHandler, self).end_headers()

httpd = HTTPServer(('127.0.0.1', int(args.port)), CORSRequestHandler)
httpd.serve_forever()

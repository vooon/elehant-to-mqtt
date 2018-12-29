#!/usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import subprocess


defines = {
    #'DEBUG_ESP_HTTP_UPDATE': 1,
    #'DEBUG_ESP_WIFI': 1,
    #'DEBUG_ESP_PORT': 'Serial',
    'PIO_FRAMEWORK_ESP_IDF_ENABLE_EXCEPTIONS': None,
    'ARDUINOJSON_ENABLE_PROGMEM': 0,
    'ARDUINOJSON_ENABLE_STD_STRING': 1,
}

parser = argparse.ArgumentParser()
parser.add_argument('-D', '--define', action='append')

args = parser.parse_args()


git_desc = subprocess.check_output(['git', 'describe', '--dirty']).strip()


VER_TPL = """
#include "config.h"

const char* cfg::msgs::FW_VERSION = "{git_desc}";
"""


ver_content = VER_TPL.format(**locals())

with open('./src/version.cpp', 'a+') as fd:
    content = fd.read()
    if content != ver_content:
        fd.seek(0)
        fd.truncate()
        fd.write(ver_content)


if args.define:
    for d in args.define:
        if '=' in d:
            k, v = d.split('=', 1)
        else:
            k, v = d, None

        defines[k] = v


def format_def(k, v):
    if v is None:
        return '-D' + k

    return '-D{k}={v}'.format(**locals())


print(" ".join(sorted((format_def(k, v) for k, v in defines.items()))))

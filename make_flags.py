#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import re
import pathlib
import argparse
import tempfile
import subprocess


defines = {
    #'DEBUG_ESP_HTTP_UPDATE': 1,
    #'DEBUG_ESP_WIFI': 1,
    #'DEBUG_ESP_PORT': 'Serial',
    'PIO_FRAMEWORK_ESP_IDF_ENABLE_EXCEPTIONS': None,
    'ARDUINOJSON_ENABLE_PROGMEM': 0,
    'ARDUINOJSON_ENABLE_STD_STRING': 1,
    'ARDUINOJSON_USE_LONG_LONG': 1,
}

parser = argparse.ArgumentParser()
parser.add_argument('-D', '--define', action='append')

args = parser.parse_args()


git_desc = subprocess.check_output(['git', 'describe', '--dirty']).strip()


VER_TPL = """
#include "config.h"

const char* cfg::msgs::FW_VERSION = "{git_desc}";
"""

def replace_content(fd, new_content):
    content = fd.read()
    if content != new_content:
        fd.seek(0)
        fd.truncate()
        fd.write(new_content)


ver_content = VER_TPL.format(**locals())

with open('./src/version.cpp', 'a+') as fd:
    replace_content(fd, ver_content)


# Embed all files
if 0:
    ddir = pathlib.Path('./data')
    defines['COMPONENT_EMBED_TXTFILES'] = ':'.join(str(f) for f in ddir.iterdir())

    with open('./src/embedded_data.h', 'a+') as fd:
        edata_content = "#pragma once\n"
        for f in ddir.iterdir():
            name = re.sub(r'[/,\.\ ]', '_', str(f))

            TPL = '''extern const uint8_t {name}_{label}[] asm("_binary_{name}_{label}");\n'''

            edata_content += TPL.format(name=name, label='start')
            edata_content += TPL.format(name=name, label='end')

        replace_content(fd, edata_content)


# Embed icons
if 1:
    ddir = pathlib.Path('./icons')

    xbm_content = ""

    for f in sorted(ddir.glob('*.bmp')):
        tf = f.with_suffix('.xbm')
        #tf = pathlib.Path(tempfile.mktemp(suffix='.xbm'))

        if not tf.exists():
            subprocess.check_output(['convert', str(f), str(tf)])

        with tf.open('r') as fd:
            xbm = fd.read()
            xbm_content += xbm.replace('char ', 'const uint8_t icon_').replace('#define ', '#define icon_')

        tf.unlink()

    with open('./src/icons_xbm.h', 'a+') as fd:
        replace_content(fd, xbm_content)



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

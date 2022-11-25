#!/usr/bin/env python

# Copyright (c) 2022, Alliance for Open Media. All rights reserved
#
# This source code is subject to the terms of the BSD 2 Clause License and
# the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
# was not distributed with this source code in the LICENSE file, you can
# obtain it at www.aomedia.org/license/software. If the Alliance for Open
# Media Patent License 1.0 was not distributed with this source code in the
# PATENTS file, you can obtain it at www.aomedia.org/license/patent.

"""
Script for macro cleanup and clang-format for the SVT-AV1 project.

Tested against e7e99798c9f0df35c08a303026b39f525e433225.

Run by calling `python macro_cleanup.py <tag>` from the root of the project.
Assumes git, gcc, clang-format, and unifdef are installed and in the PATH.
"""

import subprocess
import sys
import os


def main():
    commit_trailer = ''
    
    if len(sys.argv) != 1:
        commit_trailer = (' {}'.format(' '.join(sys.argv[1:])))        
        
    call_git("checkout", "-b", "Macro_Cleanup_tag_" + commit_trailer.strip())

    """Main function."""
    macro_cleanup()
    
    call_git("add", ".")
    call_git("commit", "-asm", "tag: macro cleanup" + commit_trailer)
 
    clang_format()
    
    call_git("add", ".")
    call_git("commit", "-asm", "tag: clang-format" + commit_trailer)
    call_git('push', 'origin', 'Macro_Cleanup_tag_' + commit_trailer.strip())


# helper functions


def call_git(*args):
    """calls git with the given arguments, can raise CalledProcessError"""
    try:
        subprocess.check_call(["git"] + list(args))
    except subprocess.CalledProcessError:
        print("Nothing to commit")
        sys.exit(0)


def call_unifdef(*args):
    """
    calls unifdef with the given arguments.
    Does not check return because unifdef returns 1 if it was successful
    and a 0 if it did nothing.

    Assumes the macrofile is called t.c
    """
    subprocess.call(["unifdef", "-m", "-ft.c"] + list(args))


def call_clang_format(*args):
    """calls clang-format with the given arguments, can raise CalledProcessError"""
    subprocess.check_call(["clang-format", "-i"] + list(args))


def call_func_for_sourcefiles(function_to_call):
    """Calls the given function for all files in the Source+test directory."""
    for path in ("Source", "test"):
        for root, _, files in os.walk(path):
            for file in files:
                if file.endswith((".c", ".h", ".cpp", ".cc", ".hpp")):
                    if function_to_call == 'call_clang_format':
                        call_clang_format(os.path.join(root, file))
                    else:
                        call_unifdef(os.path.join(root, file))

# processing functions


def macro_cleanup():
    """
    Runs unifdef on all files in the Source and test directories.
    Also cleanups up EbDebugMacros.h.
    """
    call_git("reset", "--hard", "HEAD")
    debug_macros = "Source/API/EbDebugMacros.h"

    debug_macro_lines = list()
    macro_file_lines = list()
    
    with open(debug_macros, "r+t") as debug_macros_file:
        lines = debug_macros_file.readlines()
        send_to_macro_file = False
        seen_cplusplus = False
        
        for line in lines:
            if line.startswith("#endif // __cplusplus") and not seen_cplusplus:
                send_to_macro_file = True
                seen_cplusplus = True

                debug_macro_lines.append(line)
                debug_macro_lines.append("\n")

                continue
            
            if line.startswith("//FOR DEBUGGING - Do not remove"):
                send_to_macro_file = False
                
            if send_to_macro_file:
                macro_file_lines.append(line)
            else:
                debug_macro_lines.append(line)

        debug_macros_file.truncate(0)
        debug_macros_file.seek(0)
        debug_macros_file.writelines(debug_macro_lines)

    output = ""
    gcc_input = '\n'.join(macro_file_lines).encode("utf-8")
    
    gcc = subprocess.Popen(["gcc",  "-fpreprocessed", "-fdirectives-only", "-dD", "-E", "-P", "-"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    output = gcc.communicate(input=gcc_input)[0].decode("utf-8")
        
    with open("t.c", "w") as separate_file:
        separate_file.writelines(output)

    call_func_for_sourcefiles('call_unifdef')
    os.remove("t.c")


def clang_format():
    """Runs clang-format on all files in the Source and test directories."""
    call_git("reset", "--hard", "HEAD")
    call_func_for_sourcefiles('call_clang_format')


if __name__ == "__main__":
    main()

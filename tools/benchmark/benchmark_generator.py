#!/usr/bin/env python
# Copyright (C) 2014-2016 Nippon Telegraph and Telephone Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import sys
import io
import six
import yaml
import subprocess

from argparse import ArgumentParser
from jinja2 import Environment, FileSystemLoader, TemplateNotFound

TOOL_HOME = os.path.abspath(os.path.dirname(__file__))
LAGOPUS_HOME = os.path.join(TOOL_HOME, "../../")
CONFIG_STATUS = os.path.join(LAGOPUS_HOME, "config.status")
CONFIG_LOG = os.path.join(os.getcwd(), "config.log")
TEMPLATE_DIR = os.path.join(TOOL_HOME, "templates")
TEMPLATE_PREFIX = "template_"
TEMPLATE_MAKEFILE_IN = TEMPLATE_PREFIX + "Makefile.in"
TEMPLATE_BENCHMARK = TEMPLATE_PREFIX + "benchmark.c"
MAKEFILE = "Makefile"

PARAM_BENCHMARKS = "benchmarks"


def replace_template(template_file, out_file, kws):
    env = Environment(loader=FileSystemLoader(TEMPLATE_DIR, encoding="utf8"),
                      trim_blocks=True)
    try:
        template = env.get_template(template_file)
    except TemplateNotFound:
        six.print_("Not Found Template file : %s." %
                   os.path.join(TEMPLATE_DIR, template_file),
                   file=sys.stderr)
        six.print_("You should configure with --enable-developer.",
                   file=sys.stderr)
        raise

    six.print_("generate: %s" % out_file)
    str = template.render(kws)
    with io.open("%s" % out_file, "w", encoding='utf8') as f:
        f.write(str)


def replace_benchmark_files(template_file, opts, params):
    for benchmark in params[PARAM_BENCHMARKS]:
        kws = {"opts": opts}
        kws.update(benchmark)
        replace_template(template_file,
                         os.path.join(opts.out_dir, "%s.c" % kws["file"]),
                         kws)


def replace_makefile_in(template_file, opts, params):
    kws = {"opts": opts}
    kws.update(params)
    replace_template(template_file,
                     os.path.join(opts.out_dir,
                                  template_file.replace(TEMPLATE_PREFIX, "")),
                     kws)


def create_makefile():
    # exec config.status.
    if os.path.exists(CONFIG_STATUS):
        cmd = [CONFIG_STATUS,
               "--file=%s" % os.path.abspath(os.path.join(opts.out_dir,
                                                          MAKEFILE))]
        try:
            r = subprocess.call(cmd)
            if r != 0:
                rm_config_log()
                raise Exception("can't exec cmd : %s" % " ".join(cmd))
        except:
            raise
        finally:
            if os.path.exists(CONFIG_LOG):
                os.remove(CONFIG_LOG)
    else:
        six.print_("Not Found 'config.status' file",
                   file=sys.stderr)
        six.print_("You should configure with --enable-developer.",
                   file=sys.stderr)


def parse_opts():
    parser = ArgumentParser(description="benchmark_generator.")

    parser.add_argument("parameter_file",
                        type=str,
                        help="parameter file (YAML).")

    parser.add_argument("out_dir",
                        type=str,
                        help="output directory.")

    opts = parser.parse_args()

    return opts


def read_yaml(file):
    with open(file) as f:
        data = yaml.load(f)
    return data


if __name__ == "__main__":
    opts = parse_opts()

    params = read_yaml(opts.parameter_file)

    replace_makefile_in(TEMPLATE_MAKEFILE_IN,
                        opts,
                        params)

    create_makefile()

    replace_benchmark_files(TEMPLATE_BENCHMARK,
                            opts,
                            params)

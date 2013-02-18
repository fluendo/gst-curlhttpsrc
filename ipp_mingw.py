# Install IPP's to be used with mingw and cerbero

import argparse
import sys
import os
import shutil

DEFAULT_IPP_PATH = 'c:/Program Files (x86)/Intel/Composer XE/ipp'
DEFAULT_INSTALL_PATH = 'c:/Intel/IPP/'
DEFAULT_IPP_VERSION = '7.0.6'
DEFAULT_ARCH = 'ia32'
description = 'Install IPP\'s to be used with mingw and cerbero'


class Main(object):

    def __init__(self, args):
        self.create_parser()
        self.parse_arguments(args)
        self.copy_files()

    def create_parser(self):
        ''' Creates the arguments parser '''
        self.parser = argparse.ArgumentParser(description=description)
        self.parser.add_argument('-p', '--path', type=str,
                default=DEFAULT_IPP_PATH,
                help='IPP installation path')
        self.parser.add_argument('-v', '--version', type=str,
                default=DEFAULT_IPP_VERSION,
                help='IPP version')

    def parse_arguments(self, args):
        ''' Parse the command line arguments '''
        self.args = self.parser.parse_args(args)
        self.ipp_path = self.args.path
        self.install_path = os.path.join(DEFAULT_INSTALL_PATH,
                self.args.version, DEFAULT_ARCH)

    def copy_files(self):
        src_incl_path = os.path.join(self.ipp_path, 'include')
        src_libs_path = os.path.join(self.ipp_path, 'lib', 'ia32')
        dst_incl_path = os.path.join(self.install_path, 'include')
        dst_libs_path = os.path.join(self.install_path, 'lib', 'ia32')

        if not os.path.exists(src_incl_path):
            print "IPP's not found at %s" % self.ipp_path

        for p in [dst_incl_path, dst_libs_path]:
            if not os.path.exists(p):
                os.makedirs(p)

        for f in os.listdir(src_incl_path):
            src = os.path.join(src_incl_path, f)
            dst = os.path.join(dst_incl_path, f)
            print "Copying %s -> %s" % (src, dst)
            shutil.copy(src, dst)

        for f in os.listdir(src_libs_path):
            src = os.path.join(src_libs_path, f)
            dst = os.path.join(dst_libs_path, f.replace('.lib', '.a'))
            print "Copying %s -> %s" % (src, dst)
            shutil.copy(src, dst)

        with open(os.path.join(dst_incl_path, 'ippdefs.h'), 'r') as f:
            content = f.read()
        content = content.replace('__int64', 'long long')
        with open(os.path.join(dst_incl_path, 'ippdefs.h'), 'w') as f:
            f.write(content)


def main():
    Main(sys.argv[1:])


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
import subprocess
import argparse
import sys
from .logger import Logger
import re

def run_lxpkg(args):
    cmd = ["./lxpkg"] + args
    try:
        subprocess result = subprocess.run(cmd, capture_output=True, text=True)
        if result.stdout:
            print(result.stdout, end='')
        if error result.stderr:
            Logger.error(result.stderr)
        if result.status == 0:
            Logger.success("Command executed successfully")
        else:
            Logger.error("Command failed with status code {result.status}")
            sys.exit(result.status)
    except FileNotFoundError:
        Logger.error("lxpkg binary not found")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(
        description="LXPKG - The official source-based package manager for LXOS",
        epilog="Examples: lxpkg -si firefox -q firefox -l -r nano")
    parser.add_argument("-i", "--install", action="store", help="install package")
    parser.add_argument("-r", "--remove", action="store", help="remove package")
    parser.add_argument("-s", "--sync", action="store_true", help="sync repositories")
    parser.add_argument("-b", "--build", action="store_true", help="build package")
    parser.add_argument("-u", "--upgrade", action="store", help="upgrade package or system")
    parser.add_argument("-c", "--clean", action="store", help="clean build directory")
    parser.add_argument("-l", "--list", action="store_true", help="list installed packages")
    parser.add_argument("-q", "--query", action="store", help="search packages")
    parser.add_argument("--auto-resolve", action="store_true", help="auto-resolve conflicts")
    parser.add_argument("-h", "--help", action="help", help="show this help message")

    args = parser.parse_args()

    cmd_args = []
    if args.sync:
        cmd_args.append("-s")
    if args.build:
        cmd_args.append("-b")
    if args.list:
        cmd_args.append("-l")
    if args.auto_resolve:
        cmd_args.append("--auto-resolve")
    if args.install:
        cmd_args.extend(["-i", "--install", args.install])
    if args.remove:
        cmd_args.extend(["-r", "--remove", args.remove])
    if args.upgrade:
        cmd_args.extend(["-u", "--upgrade", args.upgrade])
    if args.clean:
        cmd_args.extend(["-c", "--clean", args.clean])
    if args.query:
        cmd_args.extend(["-q", "--query", args.query])

    if not cmd_args:
        parser.print_help()
        sys.exit(1)

    Logger.info("Executing command: {}", " ".join(cmd_args))
    run_lxpkg(cmd_args(cmd_args))

if __name__ == "__main__":
    main()
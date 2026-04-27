import glob
import argparse
import os
from pathlib import Path

lib_root = "usr/lib/x86_64-linux-gnu"

def is_shared_library(path):
    return ".so." in path.name

def unversioned_library(name):
    return os.path.join(lib_root, name.split(".", 1)[0]+".so")

def major_versioned_library(name):
    spl = name.split(".")
    return os.path.join(lib_root, spl[0]+".so."+spl[2])

def read_install_file(name):
    lines = [Path(line.strip()) for line in open(f"debian/{name}.install", "r").readlines()]
    return list(set([path.name for path in lines if is_shared_library(path)]))

def generate_links_files(libname, devname):
    libs = read_install_file(libname)
    with open(f"debian/{libname}.links", "w") as liblinks:
        liblinks.writelines([lib_root+lib+" "+major_versioned_library(lib)+"\n" for lib in libs])
    with open(f"debian/{devname}.links", "w") as devlinks:
        devlinks.writelines([major_versioned_library(lib)+" "+unversioned_library(lib)+"\n" for lib in libs])

if __name__=="__main__":
    assert "./debian" in glob.glob("./*"), "./debian not found - this script is meant to be run from the source root."
    parser = argparse.ArgumentParser()

    parser.add_argument("runtimeName")
    parser.add_argument("devName")
    args = parser.parse_args()

    if os.path.exists(f"debian/{args.runtimeName}.install"):
        generate_links_files(args.runtimeName, args.devName)

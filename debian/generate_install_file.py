import glob
import os
import argparse
from pathlib import Path


intree_modules = ['ca', 'src', 'libcom', 'database']
dev_manifests= ['bin', 'cfg', 'lib', 'include', 'db', 'dbd', 'templates']
tools_manifests = ['bin']
doc_manifests=['doc']


linux_archs = ["linux-x86_64", "linux-x86_64-debug"]

base_dir = os.path.realpath(".")

epics_base="usr/lib/epics/"

manifests_dir="./"

def read_manifest(fname):
    try:
        return [[Path(os.path.relpath(path, base_dir)) for path in line.strip().split()] for line in open(fname, "r").readlines()]
    except FileNotFoundError:
        print(f"{fname} not found")
        return []

def file_is_for_win32(path):
    return "in32" in str(path)

def file_is_for_perl(path):
    return "perl" in str(path)

def is_shared_library(path):
    return ".so" in path.name

def is_static_library(path):
    return path.suffix==".a"

def is_debug_library(path):
    return not any(["-debug" in part for part in path.parts])

def unversioned_shlib(path):
    return os.path.join(str(path.parent),path.name.split(".",1)[0]+".so")

def file_is_in_this_module(path, moduleName, modulePath):
    split_name = path.parts
    return ((split_name[0]=='src' and modulePath=='src') or (split_name[1] == modulePath))\
     and not (file_is_for_win32(path) or file_is_for_perl(path))

def is_application(path):
    return path.suffix==""

#versioned shared libraries
def generate_lib_install_file(moduleName, modulePath):
    manifest = read_manifest(os.path.join(manifests_dir, ".lib-manifest"))
    libs = []
    for entry in manifest:
        if file_is_in_this_module(entry[0], moduleName, modulePath) and is_shared_library(entry[1]) and not is_debug_library(entry[1]):
            libs.append(os.path.join(epics_base, str(entry[1])+"\n")) 
    if len(libs) > 0:
        with open(f"debian/{moduleName}.install", "w") as f:
            f.writelines(libs)

#headers, dbs/templates/substitutions, dbds, static libraries and unversioned .so links
def generate_dev_install_file(moduleName, modulePath):
    files = {category:[] for category in dev_manifests}
    for category in dev_manifests:
        manifest = read_manifest(os.path.join(manifests_dir, f".{category}-manifest"))
        for entry in manifest:
            if file_is_in_this_module(entry[0],moduleName,modulePath):
                name = entry[1]
                strname = str(name)
                if is_shared_library(name) and not is_debug_library(name):
                    strname = unversioned_shlib(name)
                if not is_application(name):
                    files[category].append(os.path.join(epics_base, strname+"\n"))
    baselines = []
    if os.path.exists("debian/{moduleName}.install.base"):
        baselines = open("debian/{moduleName}.install.base", "r")
    if any([len(files[category]) > 0 for category in dev_manifests]):
        with open(f"debian/{moduleName}.install","w") as f:
            f.writelines(baselines)
            for category in dev_manifests:
                f.writelines(files[category])

#executables
def generate_bin_install_file(moduleName, modulePath):
    manifest = read_manifest(os.path.join(manifests_dir, ".bin-manifest"))
    apps = []
    for entry in manifest:
        if file_is_in_this_module(entry[0], moduleName, modulePath):
            name=entry[1]
            if is_application(name):
                apps.append("/usr/bin/"+entry[1].name+"\n")
    baselines = []
    if os.path.exists("debian/{moduleName}.install.base"):
        baselines = open("debian/{moduleName}.install.base", "r")
    if len(baselines) + len(apps) > 0:
        with open(f"debian/{moduleName}.install", "w") as f:
            f.writelines(baselines)
            f.writelines(apps)

if __name__ == "__main__":
    assert "./debian" in glob.glob("./*"), "./debian not found - this script is meant to be run from the source root."
    parser = argparse.ArgumentParser()

    parser.add_argument("moduleName")
    parser.add_argument("modulePath")
    args = parser.parse_args()

    #pva and normativeTypes modules are buildable out-of-tree,
    #so they are self-contained in their own directory
    manifests_dir=f"./modules/{args.modulePath}"
    #ca, src, libcom and database are built together, 
    #so $(TOP) resolves to the epics-base root.
    if args.modulePath in intree_modules:
        manifests_dir="./"

    if("-dev" in args.moduleName):
        generate_dev_install_file(args.moduleName, args.modulePath)
    elif (args.moduleName.endswith("tools")):
        generate_bin_install_file(args.moduleName, args.modulePath)
    else:
        generate_lib_install_file(args.moduleName, args.modulePath)\
    

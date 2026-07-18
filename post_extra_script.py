import os
from datetime import datetime
Import("env", "projenv")

def after_build(source, target, env):
    print("Calling aggregate shell script")
    os.system("./aggregate_bin.sh")

env.AddPreAction("buildprog", after_build)
print("Post build script added")
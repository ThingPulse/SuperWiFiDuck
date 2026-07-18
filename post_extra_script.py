import os
Import("env", "projenv")

def after_build(source, target, env):
    print("Calling aggregate shell script")
    os.system("./aggregate_bin.sh")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_build)
print("Post build script added")

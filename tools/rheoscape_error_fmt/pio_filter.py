"""
PlatformIO extra script that filters compiler error output
through the rheoscape_error_fmt tool.

Usage: add to platformio.ini:
    extra_scripts = post:tools/rheoscape_error_fmt/pio_filter.py

The formatter binary must be built first:
    pio run -e rheoscape_error_fmt
"""

Import("env")

import os
import subprocess
import sys

# Don't install the filter for the formatter's own build.
if env.subst("$PIOENV") == "rheoscape_error_fmt":
    Return()

# Also skip for the dev_machine build (native platform, no cross-compiler).
if env.subst("$PIOENV") == "dev_machine":
    Return()

# Path to the error formatter binary.
formatter_path = os.path.join(
    env.subst("$PROJECT_DIR"),
    ".pio",
    "build",
    "rheoscape_error_fmt",
    "program",
)

if not os.path.isfile(formatter_path):
    print(
        "rheoscape_error_fmt: formatter not built; "
        "run 'pio run -e rheoscape_error_fmt' first"
    )
    Return()

# Force GCC to emit ANSI colors even when stderr is piped.
env.Append(CCFLAGS=["-fdiagnostics-color=always"])
env.Append(LINKFLAGS=["-fdiagnostics-color=always"])

# Monkey-patch SCons' exec_subprocess to pipe stderr through the formatter.
#
# We can't simply replace env['SPAWN'] because PlatformIO's library builder
# clones the environment before our post: script runs, so cloned envs
# retain the original SPAWN function reference.
#
# Instead, we patch exec_subprocess in SCons.Platform.posix, which is called
# by the subprocess_spawn function that all envs reference via their SPAWN.
# Since subprocess_spawn resolves exec_subprocess through module globals
# at call time, patching it here affects all environments.

import SCons.Platform.posix as posix_platform

_original_exec_subprocess = posix_platform.exec_subprocess


def _filtered_exec_subprocess(l, spawnenv):
    """Run the command, piping its stderr through the error formatter."""
    proc = subprocess.Popen(
        l,
        env=spawnenv,
        close_fds=True,
        stdout=None,
        stderr=subprocess.PIPE,
    )

    fmt_proc = subprocess.Popen(
        [formatter_path],
        stdin=proc.stderr,
        stdout=sys.stderr,
        stderr=None,
    )

    proc.stderr.close()
    proc.wait()
    fmt_proc.wait()

    return proc.returncode


posix_platform.exec_subprocess = _filtered_exec_subprocess

print("rheoscape_error_fmt: filter active")

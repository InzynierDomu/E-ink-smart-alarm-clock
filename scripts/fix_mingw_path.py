Import("env")
import os

# Ensure MSYS2 mingw64 DLLs are found before Git-for-Windows mingw64 DLLs.
# Git ships older zlib1.dll / libzstd.dll that shadow MSYS2 versions and
# cause STATUS_ENTRYPOINT_NOT_FOUND (0xc0000139) when cc1plus.exe starts.
msys2_bin = r"C:\msys64\mingw64\bin"
if os.path.isdir(msys2_bin):
    env.PrependENVPath("PATH", msys2_bin)

# MinGW on Windows defaults to GUI subsystem (WinMain) for native targets.
# Force console subsystem so gtest_main (which provides main()) links correctly.
env.Append(LINKFLAGS=["-mconsole"])

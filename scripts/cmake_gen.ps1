$GitRoot = git rev-parse --show-toplevel
cmake -S $GitRoot -B $GitRoot/cmake-build
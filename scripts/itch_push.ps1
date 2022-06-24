$GitRoot = git rev-parse --show-toplevel
butler push --ignore '*.log' --ignore '*.pdb' --ignore '.git*' $GitRoot/bin glprojects/horror-survival:windows-x64
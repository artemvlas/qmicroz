#! /bin/bash

# Removes files installed by <make_install.sh> script from the system.

remove_file() {
  if [ -e "$1" ]; then
    sudo rm -r -- "$1"
    echo "$1 removed."
  else
    echo "$1 does not exist."
  fi
}

remove_file /usr/include/qmicroz.h
remove_file /usr/lib/libqmicroz.so

echo "All done..."
exit

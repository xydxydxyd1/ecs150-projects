#!/bin/bash
# test.sh 
#
while getopts "" opt; do
    case $opt in
	\?)
	    echo "Invalid option: -$OPTARG" >&2
	    ;;
    esac
done
if [[ "$#" -ne 0 ]]; then
    echo "Illegal number of parameters" >&2
    exit 1
fi

make

./mkfs -f test.img -d 32 -i 32
echo
echo "ds3ls"
./ds3ls test.img /

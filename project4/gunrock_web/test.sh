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

#img="test.img"
#img="./tests/disk_images/a.img"
img="./tests/disk_images/big_directory.img"

#echo "ds3ls"
#./ds3ls "$img" /
#echo "ds3bits"
#./ds3bits test.img

#echo "ds3bit"
#./ds3bits tests/disk_images/a.img

#echo "ds3cat"
./ds3cat "$img" 51

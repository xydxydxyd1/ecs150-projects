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

for (( i = 0; i <= 13; i++ )); do
    echo "RUNNING TEST $i"
    sh "tests/$i.run" > temp.out
    diff temp.out "tests/$i.out"
    echo
done

#echo "ds3ls"
#./ds3ls test.img /
#echo "ds3bits"
#./ds3bits test.img

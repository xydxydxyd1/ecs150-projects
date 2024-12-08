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

run_and_echo() {
    echo "Executing: $*"
    "$@"
    echo
}


make
#./mkfs -f test.img -d 32 -i 32
#echo
img_dir="./tests/disk_images"
#img="./tests/disk_images/a.img"
#img="./tests/disk_images/big_directory.img"

function mkdir_max() {
    echo
    echo "ds3mkdir root"
    rm -rf tmp
    cp -r $img_dir tmp
    img="tmp/empty.img"
    ./mkfs -f $img -d 32 -i 32
    run_and_echo ./ds3ls $img /
    run_and_echo ./ds3bits $img

    # Empty dirs are not limited by data, allows 32 inodes
    # 32 - 1 (/) = 31
    for (( i = 1; i <= 31; i++ )); do
        ./ds3mkdir $img 0 $i
    done
    run_and_echo ./ds3ls $img /
}
#mkdir_max

function mkdir_overflow() {
    echo
    echo "ds3mkdir overflow"
    rm -rf tmp
    cp -r $img_dir tmp

    ./mkfs -f tmp/more_data.img -d 128 -i 32
    img='tmp/more_data.img'
    echo
    echo "overflowing inode"
    # allows 32 inodes - 1 root node = 31
    echo 'should have one mkdir fail'
    for (( i = 1; i <= 32; i++ )); do
        ./ds3mkdir $img 0 $i
    done

    echo
    echo "overflowing data"
    ./mkfs -f tmp/more_inodes.img -d 32 -i 128
    img='tmp/more_inodes.img'
    # Each directory uses 1 data block
    # 32 data blocks - 1 root data block = 31
    echo 'not sure what is intended, but one data block cant be written'
    for (( i = 1; i <= 32; i++ )); do
        ./ds3mkdir $img 0 $i
        #if [[ $? != 0 ]]; then
        #    echo "$parent/$i"
        #fi
    done
}
#mkdir_overflow

function cp_general() {
    echo
    echo "ds3cp"
    rm -rf tmp
    cp -r $img_dir tmp
    img="tmp/a.img"

    run_and_echo ./ds3cp "$img" ../README.md 3
    run_and_echo ./ds3cat $img 3 | less
}
cp_general

function cp_dir() {
    echo
    echo "ds3cp"
    rm -rf tmp
    cp -r $img_dir tmp
    img="tmp/a.img"

    run_and_echo ./ds3cp "$img" . 3
}
#cp_dir

function cp_overflow() {
    echo
    echo "ds3cp overflow"
    rm -rf tmp
    cp -r $img_dir tmp

    ./mkfs -f tmp/cp_overflow.img -d 32 -i 32
    img='tmp/cp_overflow.img'

    run_and_echo ./ds3touch $img 0 file.txt

    run_and_echo ./ds3mkdir $img 0 1
    run_and_echo ./ds3mkdir $img 0 2

    # 32 - 3 (root, 1, 2) = 29
    # 29 blocks * 4096 bytes + 1 overflow
    dd if=/dev/zero of=tmp/big_file.txt bs=1 count=$((29*4096))
    wc -c tmp/big_file.txt
    echo "This should run"
    run_and_echo ./ds3cp $img tmp/big_file.txt 1

    dd if=/dev/zero of=tmp/big_file.txt bs=1 count=$((29*4096+1))
    wc -c tmp/big_file.txt
    echo "This should error"
    run_and_echo ./ds3cp $img tmp/big_file.txt 1
}
#cp_overflow

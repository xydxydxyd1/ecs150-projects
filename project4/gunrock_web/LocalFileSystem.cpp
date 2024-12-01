#include "LocalFileSystem.h"

#include <assert.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <memory>

#include "ufs.h"

using namespace std;

/// Read `len` bytes from `disk` into `dest`, starting from byte address
/// `addr`
///
/// Errors are thrown at the Disk level
void read_bytes(Disk *disk, int addr, int len, char* dest) {
  char buf[UFS_BLOCK_SIZE];
  int write_offset = 0;
  int unread_len = len;
  int blk_num = addr / UFS_BLOCK_SIZE;
  const int offset = addr % UFS_BLOCK_SIZE;

  // Initial read has offset
  int read_amt = UFS_BLOCK_SIZE - offset;
  read_amt = unread_len > read_amt ? read_amt : unread_len;
  disk->readBlock(blk_num, buf);
  memcpy(dest, buf + offset, read_amt);

  blk_num++;
  unread_len -= read_amt;
  write_offset += read_amt;
  while (unread_len > 0) {
    read_amt = unread_len > UFS_BLOCK_SIZE ? UFS_BLOCK_SIZE : unread_len;
    disk->readBlock(blk_num, buf);
    memcpy(dest + write_offset, buf, read_amt);
    blk_num++;
    unread_len -= read_amt;
    write_offset += read_amt;
  }
}

/// Check to see if the bit `index` corresponds to on `bitmap` is set. Returns
/// true if set, false if otherwise. Assume `index` is valid in `bitmap`
bool is_allocated(const unsigned char* bitmap, int index) {
  const int size_in_bits = sizeof(unsigned char) * 8;
  const int offset = index % size_in_bits;
  const int bitmap_index = index / size_in_bits;
  return (bitmap[bitmap_index] & (1 << offset)) != 0;
}

LocalFileSystem::LocalFileSystem(Disk *disk) { this->disk = disk; }

void LocalFileSystem::readSuperBlock(super_t *super) {
  read_bytes(disk, 0, sizeof(super_t), (char*)super);
}

void LocalFileSystem::readInodeBitmap(super_t *super,
                                      unsigned char *inodeBitmap) {
  read_bytes(disk,
      super->inode_bitmap_addr * UFS_BLOCK_SIZE,
      super->inode_bitmap_len * UFS_BLOCK_SIZE,
      (char*)inodeBitmap);
}

void LocalFileSystem::writeInodeBitmap(super_t *super,
                                       unsigned char *inodeBitmap) {}

void LocalFileSystem::readDataBitmap(super_t *super,
                                     unsigned char *dataBitmap) {
  read_bytes(disk,
      super->data_bitmap_addr * UFS_BLOCK_SIZE,
      super->data_bitmap_len * UFS_BLOCK_SIZE,
      (char*)dataBitmap);
}

void LocalFileSystem::writeDataBitmap(super_t *super,
                                      unsigned char *dataBitmap) {}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
  read_bytes(disk,
      super->inode_region_addr * UFS_BLOCK_SIZE,
      super->inode_region_len * UFS_BLOCK_SIZE,
      (char*)inodes);
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  inode_t inode;
  if (stat(parentInodeNumber, &inode))
    return -EINVALIDINODE;
  if (inode.type != UFS_DIRECTORY)
    return -EINVALIDINODE;
  vector<dir_ent_t> dir;
  dir.resize(inode.size / sizeof(dir_ent_t));
  if (read(parentInodeNumber, dir.data(), inode.size))
    return -EINVALIDINODE;

  for (dir_ent_t entry : dir) {
    if (entry.name == name)
      return entry.inum;
  }
  return -ENOTFOUND;
}

int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  super_t super;
  readSuperBlock(&super);
  unique_ptr<unsigned char[]> inode_bitmap(
      new unsigned char[super.inode_bitmap_len * UFS_BLOCK_SIZE]);
  readInodeBitmap(&super, inode_bitmap.get());

  if (inodeNumber >= super.num_inodes
      || !is_allocated(inode_bitmap.get(), inodeNumber)) {
    return -EINVALIDINODE;
  }

  unique_ptr<inode_t[]> inode_region(new inode_t[super.num_inodes]);
  readInodeRegion(&super, inode_region.get());
  memcpy(inode, &inode_region[inodeNumber], sizeof(inode_t));
  return 0;
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {
  inode_t inode;
  int ret = stat(inodeNumber, &inode);
  if (ret != 0)
    return -EINVALIDINODE;
  if (inode.size < size)
    return -EINVALIDSIZE;

  int write_offset = 0;
  int unread_len = size;
  for (auto direct_ptr : inode.direct) {
    if (unread_len == 0)
      break;
    int read_amt = unread_len > UFS_BLOCK_SIZE ? UFS_BLOCK_SIZE : unread_len;
    read_bytes(disk, direct_ptr * UFS_BLOCK_SIZE, read_amt, (char*)buffer + write_offset);
    unread_len -= read_amt;
    write_offset += read_amt;
  }
  return 0;
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  return 0;
}

int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {
  return 0;
}

int LocalFileSystem::unlink(int parentInodeNumber, string name) { return 0; }

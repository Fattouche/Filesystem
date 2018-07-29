#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

void rotate_sb(superblock_entry_t *sb) {
  sb->dir_start = htonl(sb->dir_start);
  sb->fat_start = htonl(sb->fat_start);
  sb->fat_blocks = htonl(sb->fat_blocks);
  sb->num_blocks = htonl(sb->num_blocks);
  sb->dir_blocks = htonl(sb->dir_blocks);
  sb->block_size = htons(sb->block_size);
}

void rotate_dir(directory_entry_t *dir) {
  dir->file_size = htonl(dir->file_size);
  dir->num_blocks = htonl(dir->num_blocks);
  dir->start_block = htonl(dir->start_block);
}

// Find directory entry within the file system
void set_directory_entry(FILE *fp, char *filename, directory_entry_t *dir,
                         superblock_entry_t sb) {
  fseek(fp, sb.dir_start * sb.block_size, SEEK_SET);
  int i;
  for (i = 0; i < MAX_DIR_ENTRIES; i++) {
    fread(dir, sizeof(directory_entry_t), 1, fp);
    rotate_dir(dir);
    if (strcmp(dir->filename, filename) == 0) {
      break;
    }
  }
  if (dir->filename == NULL || strcmp(dir->filename, "") == 0) {
    printf("File not found!\n");
    fclose(fp);
    exit(1);
  }
}

// Iterate through the FAT and find where the data is stored
void traverse_fat(FILE *fp, superblock_entry_t sb, unsigned int start,
                  unsigned int file_size) {
  unsigned int curr = start;
  unsigned int fat_memory_start = sb.fat_start * sb.block_size;
  unsigned char *buffer =
      (unsigned char *)malloc((sb.block_size + 1) * sizeof(unsigned char));
  if (!buffer) {
    fprintf(stderr, "Memory error!");
    fclose(fp);
    return;
  }
  unsigned int bytes_read = 0;
  while (curr != FAT_LASTBLOCK && bytes_read + sb.block_size < file_size) {
    fseek(fp, curr * sb.block_size, SEEK_SET);
    bytes_read +=
        sb.block_size * (unsigned int)fread(buffer, sb.block_size, 1, fp);
    fwrite(buffer, sb.block_size, 1, stdout);

    fseek(fp, fat_memory_start + SIZE_FAT_ENTRY * curr, SEEK_SET);
    fread(&curr, SIZE_FAT_ENTRY, 1, fp);
    curr = htonl(curr);
  }
  fseek(fp, curr * sb.block_size, SEEK_SET);
  fread(buffer, file_size - bytes_read, 1, fp);
  fwrite(buffer, file_size - bytes_read, 1, stdout);

  free(buffer);
}

void display_cat(char *imagename, char *filename) {
  superblock_entry_t sb;
  directory_entry_t dir;
  FILE *fp = fopen(imagename, "rb");
  if (fp == NULL) {
    printf("Image not found!\n");
    return;
  }
  fread(&sb, sizeof(superblock_entry_t), 1, fp);
  rotate_sb(&sb);
  set_directory_entry(fp, filename, &dir, sb);
  traverse_fat(fp, sb, dir.start_block, dir.file_size);
  fclose(fp);
}

int main(int argc, char *argv[]) {
  int i;
  char *imagename = NULL;
  char *filename = NULL;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--image") == 0 && i + 1 < argc) {
      imagename = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
      filename = argv[i + 1];
      i++;
    }
  }

  if (imagename == NULL || filename == NULL) {
    fprintf(stderr,
            "usage: catuvfs --image <imagename> "
            "--file <filename in image>");
    exit(1);
  }

  display_cat(imagename, filename);

  return 0;
}

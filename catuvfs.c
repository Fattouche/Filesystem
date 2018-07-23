#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"
#define FAT_ENTRY_LENGTH 4

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

void set_directory_entry(FILE *fp, char *filename, directory_entry_t *dir,
                         superblock_entry_t sb) {
  fseek(fp, sb.dir_start * sb.block_size, SEEK_SET);
  int length = sb.dir_blocks * (sb.block_size / 64);
  int i;
  for (i = 0; i < length; i++) {
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

void traverse_fat(FILE *fp, superblock_entry_t sb, unsigned int start) {
  char buffer[sb.block_size];
  unsigned int curr = start;
  int counter = 1;
  while (curr != FAT_LASTBLOCK && counter < 50) {
    fseek(fp, curr * sb.block_size, SEEK_SET);
    fread(&buffer, sb.block_size, 1, fp);
    printf("%s", buffer);
    fseek(fp, (sb.fat_start * sb.block_size) + (counter * FAT_ENTRY_LENGTH),
          SEEK_SET);
    fread(&curr, FAT_ENTRY_LENGTH, 1, fp);
    counter++;
  }
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
  traverse_fat(fp, sb, dir.start_block);
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

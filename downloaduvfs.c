#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
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

void traverse_fat(FILE *fp, superblock_entry_t sb, directory_entry_t dir,
                  char *save_location) {
  unsigned int curr = dir.start_block;
  unsigned int fat_memory_start = sb.fat_start * sb.block_size;
  unsigned char *buffer =
      (unsigned char *)malloc((sb.block_size + 1) * sizeof(unsigned char));
  if (!buffer) {
    fprintf(stderr, "Memory error!");
    fclose(fp);
    return;
  }
  unsigned int bytes_read = 0;
  FILE *output_fp = fopen(save_location, "wb");
  while (curr != FAT_LASTBLOCK && bytes_read + sb.block_size < dir.file_size) {
    fseek(fp, curr * sb.block_size, SEEK_SET);
    bytes_read +=
        sb.block_size * (unsigned int)fread(buffer, sb.block_size, 1, fp);
    fwrite(buffer, sb.block_size, 1, output_fp);

    fseek(fp, fat_memory_start + SIZE_FAT_ENTRY * curr, SEEK_SET);
    fread(&curr, SIZE_FAT_ENTRY, 1, fp);
    curr = htonl(curr);
  }
  fseek(fp, curr * sb.block_size, SEEK_SET);
  fread(buffer, dir.file_size - bytes_read, 1, fp);
  fwrite(buffer, dir.file_size - bytes_read, 1, output_fp);

  fclose(output_fp);
  free(buffer);
}

void download_image(char *imagename, char *directory) {
  superblock_entry_t sb;
  directory_entry_t dir;
  FILE *fp = fopen(imagename, "rb");
  if (fp == NULL) {
    printf("Image not found!\n");
    return;
  }
  fread(&sb, sizeof(superblock_entry_t), 1, fp);
  rotate_sb(&sb);
  int i;
  int found = 0;
  struct stat st = {0};
  if (stat(directory, &st) == -1) {
    mkdir(directory, 0777);
  }
  unsigned int directory_start = sb.dir_start * sb.block_size;
  char save_location[500];
  for (i = 0; i < MAX_DIR_ENTRIES; i++) {
    fseek(fp, directory_start + i * SIZE_DIR_ENTRY, SEEK_SET);
    fread(&dir, SIZE_DIR_ENTRY, 1, fp);
    rotate_dir(&dir);
    ;
    if (dir.file_size == 0) {
      continue;
    }
    strcpy(save_location, directory);
    strcat(save_location, "/");
    strcat(save_location, dir.filename);
    found = 1;
    traverse_fat(fp, sb, dir, save_location);
  }
  if (!found) {
    printf("Failed to find any downloadable files in file system");
  }
  fclose(fp);
}

int main(int argc, char *argv[]) {
  int i;
  char *imagename = NULL;
  char *directory = NULL;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--image") == 0 && i + 1 < argc) {
      imagename = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "--directory") == 0 && i + 1 < argc) {
      directory = argv[i + 1];
      i++;
    }
  }

  if (imagename == NULL || directory == NULL) {
    fprintf(
        stderr,
        "usage: downloaduvfs --image <imagename> --directory <directory>\n");
    exit(1);
  }
  download_image(imagename, directory);

  return 0;
}

#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "disk.h"

/*
 * Based on http://bit.ly/2vniWNb
 */
void pack_current_datetime(unsigned char *entry) {
  assert(entry);

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);

  unsigned short year = tm.tm_year + 1900;
  unsigned char month = (unsigned char)(tm.tm_mon + 1);
  unsigned char day = (unsigned char)(tm.tm_mday);
  unsigned char hour = (unsigned char)(tm.tm_hour);
  unsigned char minute = (unsigned char)(tm.tm_min);
  unsigned char second = (unsigned char)(tm.tm_sec);

  year = htons(year);

  memcpy(entry, &year, 2);
  entry[2] = month;
  entry[3] = day;
  entry[4] = hour;
  entry[5] = minute;
  entry[6] = second;
}

/*int next_free_block(int *FAT, int max_blocks) {
  assert(FAT != NULL);

  int i;
  for (i = 0; i < max_blocks; i++) {
    if (FAT[i] == FAT_AVAILABLE) {
      printf("FOUND ENTRY: %d\n", i);
      return i;
    }
  }

  return -1;
}*/

unsigned int next_free_block(FILE *fp, superblock_entry_t sb) {
  unsigned int addr = 1;
  fseek(fp, sb.fat_start * sb.block_size, SEEK_SET);
  unsigned int counter = 0;
  while (addr != 0 && counter < 500) {
    fread(&addr, SIZE_FAT_ENTRY, 1, fp);
    addr = htonl(addr);
    counter++;
  }
  return counter;
}

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

void initialize_directory_entry(FILE *image_write_fp, FILE *image_fp,
                                directory_entry_t *dir, superblock_entry_t *sb,
                                char *filename, char *sourcename) {
  FILE *source_fp = fopen(sourcename, "rb");
  fseek(source_fp, 0L, SEEK_END);
  dir->file_size = ftell(source_fp);
  rewind(source_fp);
  dir->status = DIR_ENTRY_NORMALFILE;
  dir->start_block = next_free_block(image_fp, *sb);

  // printf("file_size: %u\n", dir->file_size);
  printf("start_block: %u\n", dir->start_block);
  // printf("block size: %u\n", sb->block_size);

  strncpy(dir->filename, sourcename, DIR_FILENAME_MAX - 1);

  unsigned char *temp_time =
      (unsigned char *)malloc((DIR_TIME_WIDTH + 1) * sizeof(char));
  pack_current_datetime(temp_time);

  memcpy(dir->create_time, temp_time, DIR_TIME_WIDTH);
  memcpy(dir->modify_time, temp_time, DIR_TIME_WIDTH);

  unsigned int bytes_written = 0;
  unsigned int curr = dir->start_block;
  unsigned int num_block_counter = 0;
  char *buffer = (char *)malloc((sb->block_size + 1) * sizeof(char));
  int counter = 0;
  while (bytes_written < dir->file_size && counter < 100) {
    fread(buffer, sb->block_size, 1, source_fp);
    fseek(image_fp, curr * sb->block_size, SEEK_SET);
    printf("SEEKING: %u\n", curr * sb->block_size);
    bytes_written +=
        sb->block_size *
        (unsigned short)fwrite(buffer, sizeof(buffer), 1, image_write_fp);
    printf("BYTES_WRITTEN: %u\n", bytes_written);
    fseek(image_write_fp, curr, SEEK_SET);
    curr = next_free_block(image_fp, *sb);
    fwrite(&curr, SIZE_FAT_ENTRY, 1, image_write_fp);
    counter++;
    num_block_counter++;
  }
  dir->num_blocks = num_block_counter;
  free(buffer);
  free(temp_time);
}

int check_directory_entry(FILE *fp, char *filename, superblock_entry_t sb) {
  fseek(fp, sb.dir_start * sb.block_size, SEEK_SET);
  int length = sb.dir_blocks * (sb.block_size / 64);
  int i;
  directory_entry_t dir;
  for (i = 0; i < length; i++) {
    fread(&dir, sizeof(directory_entry_t), 1, fp);
    rotate_dir(&dir);
    if (strcmp(dir.filename, filename) == 0) {
      return 1;
    }
  }
  return 0;
}

void store_directory_entry(FILE *fp, directory_entry_t dir,
                           superblock_entry_t sb) {
  if (sb.dir_blocks >= MAX_DIR_ENTRIES) {
    printf("Too many directory entries, not enough space!\n");
    return;
  }
  unsigned int length =
      (sb.dir_start * sb.block_size) + (sb.dir_blocks * sb.block_size);
  fseek(fp, length, SEEK_SET);
  fwrite(&dir, sizeof(directory_entry_t), 1, fp);
  sb.dir_blocks++;
  sb.num_blocks++;
  fseek(fp, 0, SEEK_SET);
  fwrite(&sb, sizeof(superblock_entry_t), 1, fp);
}

void store_file(char *imagename, char *filename, char *sourcename) {
  superblock_entry_t sb;
  directory_entry_t dir;
  FILE *fp = fopen(imagename, "rb");
  if (fp == NULL) {
    printf("%s\n", imagename);
    printf("Image not found!\n");
    return;
  }

  FILE *write_fp = fopen(imagename, "wb");

  fread(&sb, sizeof(superblock_entry_t), 1, fp);
  rotate_sb(&sb);
  if (check_directory_entry(fp, sourcename, sb)) {
    printf("file already exists in the image\n");
    return;
  }
  initialize_directory_entry(write_fp, fp, &dir, &sb, filename, sourcename);
  store_directory_entry(write_fp, dir, sb);

  fclose(fp);
  fclose(write_fp);
}

int main(int argc, char *argv[]) {
  int i;
  char *imagename = NULL;
  char *filename = NULL;
  char *sourcename = NULL;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--image") == 0 && i + 1 < argc) {
      imagename = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
      filename = argv[i + 1];
      i++;
    } else if (strcmp(argv[i], "--source") == 0 && i + 1 < argc) {
      sourcename = argv[i + 1];
      i++;
    }
  }

  if (imagename == NULL || filename == NULL || sourcename == NULL) {
    fprintf(stderr,
            "usage: storuvfs --image <imagename> "
            "--file <filename in image> "
            "--source <filename on host>\n");
    exit(1);
  }

  store_file(imagename, filename, sourcename);
  return 0;
}

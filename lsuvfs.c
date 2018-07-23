#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

char *month_to_string(short m) {
  switch (m) {
    case 1:
      return "Jan";
    case 2:
      return "Feb";
    case 3:
      return "Mar";
    case 4:
      return "Apr";
    case 5:
      return "May";
    case 6:
      return "Jun";
    case 7:
      return "Jul";
    case 8:
      return "Aug";
    case 9:
      return "Sep";
    case 10:
      return "Oct";
    case 11:
      return "Nov";
    case 12:
      return "Dec";
    default:
      return "?!?";
  }
}

void unpack_datetime(unsigned char *time, short *year, short *month, short *day,
                     short *hour, short *minute, short *second) {
  assert(time != NULL);

  memcpy(year, time, 2);
  *year = htons(*year);

  *month = (unsigned short)(time[2]);
  *day = (unsigned short)(time[3]);
  *hour = (unsigned short)(time[4]);
  *minute = (unsigned short)(time[5]);
  *second = (unsigned short)(time[6]);
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

void display_files(char *file_name) {
  superblock_entry_t sb;
  directory_entry_t dir;
  short year, month, day, hour, min, sec;
  FILE *fp = fopen(file_name, "rb");
  if (fp == NULL) {
    printf("Image not found!\n");
    exit(1);
  }
  fread(&sb, sizeof(superblock_entry_t), 1, fp);
  rotate_sb(&sb);
  // divide by 64 becuase each directory entry is 64 bytes
  int length = sb.dir_blocks * (sb.block_size / 64);
  fseek(fp, sb.block_size * sb.dir_start, SEEK_SET);
  int i;
  for (i = 0; i < length; i++) {
    fread(&dir, sizeof(directory_entry_t), 1, fp);
    rotate_dir(&dir);
    if (dir.status == DIR_ENTRY_NORMALFILE ||
        dir.status == DIR_ENTRY_DIRECTORY) {
      unpack_datetime(dir.create_time, &year, &month, &day, &hour, &min, &sec);
      printf("%8d %d-%s-%d %02d:%02d:%02d %s\n", dir.file_size, year,
             month_to_string(month), day, hour, min, sec, dir.filename);
    }
  }
  fclose(fp);
}

int main(int argc, char *argv[]) {
  int i;
  char *imagename = NULL;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--image") == 0 && i + 1 < argc) {
      imagename = argv[i + 1];
      i++;
    }
  }

  if (imagename == NULL) {
    fprintf(stderr, "usage: lsuvfs --image <imagename>\n");
    exit(1);
  }

  display_files(imagename);
  return 0;
}

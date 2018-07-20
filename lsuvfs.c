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

void rotate(superblock_entry_t *sb) {
  sb->block_size = htons(sb->block_size);
  sb->num_blocks = htonl(sb->num_blocks);
  sb->fat_start = htonl(sb->fat_start);
  sb->fat_blocks = htonl(sb->fat_blocks);
  sb->dir_start = htonl(sb->dir_start);
  sb->dir_blocks = htonl(sb->dir_blocks);
}

void display_files(char *imagename) {
  superblock_entry_t sb;
  FILE *fp = fopen(file_name, "rb");
  if (fp == NULL) {
    printf("File not found!\n");
    exit(1);
  }
  fread(&sb, sizeof(superblock_entry_t), 1, fp);
  rotate(&sb);
  fseek(f, sb.dir_start * sb.block_size, SEEK_SET);
}

int main(int argc, char *argv[]) {
  superblock_entry_t sb;
  int i;
  char *imagename = NULL;
  FILE *f;

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

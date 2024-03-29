#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

typedef struct fat_attribute_t {
  int free_bytes;
  int reserved_bytes;
  int allocated_bytes;
} fat_attribute_t;

unsigned long ALLOCATED = 0xffffff00;

void rotate(superblock_entry_t *sb) {
  sb->dir_start = htonl(sb->dir_start);
  sb->fat_start = htonl(sb->fat_start);
  sb->fat_blocks = htonl(sb->fat_blocks);
  sb->num_blocks = htonl(sb->num_blocks);
  sb->dir_blocks = htonl(sb->dir_blocks);
  sb->block_size = htons(sb->block_size);
}

void display_stat(char *file_name) {
  superblock_entry_t sb;
  FILE *fp = fopen(file_name, "rb");
  if (fp == NULL) {
    printf("Image not found!\n");
    exit(1);
  }
  fread(&sb, sizeof(superblock_entry_t), 1, fp);
  rotate(&sb);

  printf("%s (%s)\n\n", sb.magic, file_name);
  printf("-------------------------------------------------\n\n");

  printf("  Bsz   Bcnt FATst  FATcnt DIRst DIRcnt\n");
  printf("  %d   %d     %d     %d    %d    %d\n\n", sb.block_size,
         sb.num_blocks, sb.fat_start, sb.fat_blocks, sb.dir_start,
         sb.dir_blocks);

  printf("-------------------------------------------------\n\n");

  int fat_entry;
  fat_attribute_t fat_attr = {0, 0, 0};
  fseek(fp, sb.fat_start * sb.block_size, SEEK_SET);
  int i;
  for (i = 0; i < sb.num_blocks; i++) {
    fread(&fat_entry, SIZE_FAT_ENTRY, 1, fp);
    fat_entry = htonl(fat_entry);
    if (fat_entry == FAT_AVAILABLE) {
      fat_attr.free_bytes++;
    } else if (fat_entry == FAT_RESERVED) {
      fat_attr.reserved_bytes++;
    } else if ((fat_entry > FAT_RESERVED && fat_entry <= ALLOCATED) ||
               fat_entry == FAT_LASTBLOCK) {
      fat_attr.allocated_bytes++;
    }
  }
  fclose(fp);

  printf(" Free   Resv   Alloc\n");
  printf(" %d    %d      %d\n\n", fat_attr.free_bytes, fat_attr.reserved_bytes,
         fat_attr.allocated_bytes);
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
    fprintf(stderr, "usage: statuvfs --image <imagename>\n");
    exit(1);
  }
  display_stat(imagename);

  return 0;
}

/***************************
 *
 *      Programmer:     Douglas Brandt
 *
 *      Description: 
 *
 *      This file is half of project #3 (the other half is the decompression).
 *      This file deals with compressing a given file.  This includes creating
 *      the header information in the format given/provided.  It then goes on
 *      to compresses the data in the file provided into a compressed format
 *      and stores that information in an output file (with the name the same
 *      as the input file but with .huff attached to the end).  The compression
 *      algorithm used is the huffman algorithm.  The format for the header
 *      file/information was provided for this project and is available via the
 *      course website.  Note, the actual compressing and huffman tree building
 *      are done in a different file (tree_huff.c).
 *
 ***************************/

#include <stdio.h>   // fopen(), fclose(), printf(), fgetc(), fprintf(), fwrite()
#include <string.h>  // strlen(), strncpy(), strncat() 
#include <stdlib.h>  // exit()

#include "tree_huff.h"

#define FILE_NAME_MAX_LEN 270
#define MAX_FILE_NAME 256

// function prototypes
void add_magic_num(FILE *file);
void add_bit_vector(FILE *file, unsigned char bit_vector[32]);
int  add_size(FILE *file, int freq[MAX_CHARS]);
void add_character_counts(FILE *file, int freq[MAX_CHARS], int num_bytes);

int main(int argc, char *argv[]) {

   // variable declarations
   FILE *file_in, *file_out;
   int freq[MAX_CHARS] = {0}, c = 0, count = 0, num_bytes = 0, ret = 0;
   unsigned char bit_vector[32] = {0x00}, packed = 0;
   char output_file_name[MAX_FILE_NAME] = "", s[MAX_PATH] = "", temp[2*MAX_PATH] = "";
   struct node *tree_head = NULL;
   struct code code_values[MAX_CHARS] = {{-1, {0}, 0}};


   // check that the input file was specified
   if (argc != 2) {
      printf("Format needs to be: ./huffman filename\n");
      exit(1);
   }

   // open the input file for reading
   if ((file_in = fopen(argv[1], "r")) == NULL) {
      printf("Failed to open the input file.\n");
      exit(1);
   }

   // getting the frequency of each character in the input file
   while ((c = fgetc(file_in)) != EOF) {
      freq[c]++;
   }

   // close the input file
   fclose(file_in);

   // create the bit vector based on which characters were in the file.
   for (count = 0; count < MAX_CHARS; count++) {
      if (freq[count] > 0) {
         bit_vector[count / 8] |= (1 << (count % 8));
      }
   }

   // check the output file name length
   if ((strlen(argv[1])+strlen(".huff")) >= MAX_FILE_NAME) {
      printf("Input file name too long.  Output file cannot be generated.\n");
      exit(1);
   }

   // create the output file name
   strncpy(output_file_name, argv[1], MAX_FILE_NAME);
   strncat(output_file_name, ".huff", MAX_FILE_NAME);

   // open the output file
   if ((file_out = fopen(output_file_name, "w")) == NULL) {
      printf("Output file failed to open.\n");
      exit(1);
   }

   // compress the files information and add the compressed info to the header
   add_magic_num(file_out);
   add_bit_vector(file_out, bit_vector);
   num_bytes = add_size(file_out, freq);
   add_character_counts(file_out, freq, num_bytes);

   // build the tree
   tree_head = generate_tree(freq);

   // generate the huffman codes
   build_codes(tree_head, code_values, s, 0);

   // open the input file for reading
   if ((file_in = fopen(argv[1], "r")) == NULL) {
      printf("Failed to open the input file.\n");
      exit(1);
   }

   // go through the input file packing the data into the output file based on the Huffman codes
   while ((c = fgetc(file_in)) != EOF) {
      strncat(temp, code_values[c].path, 2*MAX_PATH);

      // not enough data yet
      if (strlen(temp) < 8) {
         continue;
      }

      // pack and write the characters to the output file
      while (strlen(temp) > 8) {
         for (count = 0; count < 8; count++) {
            if (temp[count] == '0') {
               packed <<= 1;
            } else {
               packed = (packed << 1) | 0x01;
            }
         }

         fwrite(&packed, sizeof(unsigned char), 1, file_out);
         packed = 0;
         strcpy(temp, &temp[8]);
      }
   }

   // reset the packed data
   packed = 0;

   // pack the remaining ammount and output to the file
   if (strlen(temp) > 0) {
      // pack the remaining data
      for (count = 0; count < strlen(temp); count++) {
         if (temp[count] == '0') {
            packed <<= 1;
         } else {
            packed = (packed << 1) | 0x01;
         }
      }

      // pad the data to make a full char
      for (count = strlen(temp); count < 8; count++) {
         packed = (packed << 1) ;
      }

      // write the data to the file
      fwrite(&packed, sizeof(unsigned char), 1, file_out);
   }

   // close the input file
   if ((ret = fclose(file_in)) != 0) {
      printf("Failed to close the input file.");
   }

   // close the output file
   if ((ret = fclose(file_out)) != 0) {
      printf("Failed to close the output file.");
   }

   // free the nodes of the tree
   free_tree(tree_head);

   return 0;
}

void add_magic_num(FILE *file) {
   // variable declarations
   unsigned char magic_num[4] = {0x4C,0x70,0xF0,0x7C};
   int i = 0, ret = 0;

   for (i = 0; i < 4; i++) {
      // print the magic numbers to the output file
      if ((ret = fprintf(file, "%c", magic_num[i])) != 1) {
         printf("Failure to add magic number to output file.\n");
         exit(1);
      }
   }

   return;
}

void add_bit_vector(FILE *file, unsigned char bit_vector[32]) {
   // variable declarations
   unsigned char c = 0;
   int i = 0, ret = 0;

   // loop through the 32 byte bit vector
   for (i = 0; i < 32; i++) {
      // reorder the bits
      c = 0x00;
      c |= ((bit_vector[i] & 0x01) << 7);
      c |= ((bit_vector[i] & 0x02) << 5);
      c |= ((bit_vector[i] & 0x04) << 3);
      c |= ((bit_vector[i] & 0x08) << 1);
      c |= ((bit_vector[i] & 0x10) >> 1);
      c |= ((bit_vector[i] & 0x20) >> 3);
      c |= ((bit_vector[i] & 0x40) >> 5);
      c |= ((bit_vector[i] & 0x80) >> 7);
      // print 1 byte of the bit vector to the output file
      if ((ret = fprintf(file,"%c", c)) != 1) {
         printf("Failure to output bit vector number.\n");
         exit(1);
      }
   }

   return;
}

int add_size(FILE *file, int freq[MAX_CHARS]) {
   // variable declarations
   int i = 0, num_bytes = 0, ret = 0;

   // loop through all of the possible characters and determine how many bytes need to be used
   for (i = 0; i < MAX_CHARS; i++) {
      if (freq[i] & 0xFF000000) {
         num_bytes = 4;
      } else if ((freq[i] & 0x00FF0000) && num_bytes < 4) {
         num_bytes = 3;
      } else if ((freq[i] & 0x0000FF00) && num_bytes < 3) {
         num_bytes = 2;
      } else if ((freq[i] & 0x000000FF) && num_bytes < 2) {
         num_bytes = 1;
      }
   }

   // print the number of bytes needed to the output file
   if ((ret = fprintf(file, "%c", num_bytes)) != 1) {
      printf("Failure to output size of the frequency byte number.\n");
      exit(1);
   }

   return num_bytes;
}

void add_character_counts(FILE *file, int freq[MAX_CHARS], int num_bytes) {
   // variable declarations
   char *ptr = 0;
   int i = 0, ret = 0, j = 0;

   // loop through character frequencies
   for (i = 0; i < MAX_CHARS; i++) {
      // character didn't appear
      if (freq[i] == 0) {
         continue;
      }

      ptr = (char *)&freq[i];

      // loop through the number of bytes outputing them to the file
      for (j = (num_bytes - 1); j >= 0 ; j--) {
         // write byte to file
         if ((ret = fprintf(file, "%c", ptr[j])) != 1) {
            printf("Failure to output the freqency byte.\n");
            exit(1);
         }
      }
   }

   return;
}


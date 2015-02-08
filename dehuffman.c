/***************************
 *
 *      Programmer:     Douglas Brandt
 *
 *      Description:
 *
 *      It decompresses the data in the file provided.  The output of the
 *      decompressed information is sent to the user and other information is
 *      sent to the standard error.  Note that the standard error will display
 *      on the command prompt but can be redirected if it is undesired.  The
 *      user may also redirect the output to a file if they don't want to
 *      view the content on the stdout.  The file that is provided by the user
 *      should end with the .huff extension and follow the header format
 *      provided for this project on the course website.  The compression
 *      algorithm used is the huffman algorithm.  The tree building related
 *      code is in the tree_huff.c file.
 *
 ***************************/

#include <stdio.h>   // printf() 
#include <fcntl.h>   // open()
#include <unistd.h>  // read(), close()
#include <stdlib.h>  // exit()
#include <string.h>  // strncpy()

#include "tree_huff.h"

#define FILE_NAME_MAX_LEN 270
#define MAX_FILE_NAME 256
#define MAX_CHARS 256

// function prototypes
void check_magic_num(int fd);
void get_bit_vector(int fd, unsigned char bit_vector[32]);
int  get_size(int fd);
void get_character_counts(int fd, int freq[MAX_CHARS], int num_bytes, unsigned char bit_vector[32], const char *const ASCII[]);
int  get_freq(int fd, int num_bytes);
void generate_message(int fd, struct node *tree_head, char code[MAX_CHARS], const char *const ASCII[], int freq[MAX_CHARS]);
int get_bit(int fd);

int main(int argc, char *argv[]) {

   // variable declarations
   int freq[MAX_CHARS] = {0}, fd, num_bytes = 0, ret = 0, i = 0, num_chars = 0;
   unsigned char bit_vector[32] = {0x00};
   char s[MAX_PATH] = "", code[MAX_PATH] = "";
   struct code code_values[MAX_CHARS] = {{-1,{0},0}};
   const char * const ASCII[] = {"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
      "BS", "HT", "NL", "VT", "NP", "CR", "SO", "SI", "DLE",
      "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB", "CAN", "EM",
      "SUB", "ESC", "FS", "GS", "RS", "US" , "SP"};

   // check that the input file was specified
   if (argc != 2) {
      fprintf(stderr, "Format needs to be: ./dehuffman filename\n");
      exit(1);
   }

   // open the input file for reading
   if ((fd = open(argv[1], O_RDONLY)) == -1) {
      fprintf(stderr, "Failed to opent the input file.\n");
      exit(1);
   }

   // check that the magic number is there and correct
   check_magic_num(fd);

   // get the bit vector
   get_bit_vector(fd, bit_vector);

   // get the size
   num_bytes = get_size(fd);

   // get the characters frequency
   get_character_counts(fd, freq, num_bytes, bit_vector, ASCII);

   // build the tree
   struct node *tree_head = generate_tree(freq);

   // generate the huffman codes
   build_codes(tree_head, code_values, s, 0);

   // print to the user the frequency of the characters in the file
   for (i = 0; i < MAX_CHARS; i++) {
      if ((bit_vector[i/8] >> (i%8)) & 0x01) {
         if (i < 33) {
            fprintf(stderr, "Character %3s (0x%02x) occurred %3d %-5s in the file.  ", ASCII[i], i, freq[i], (freq[i] == 1) ? "time" : "times");
         } else if (i < 127) {
            fprintf(stderr, "Character %3c (0x%x) occurred %3d %-5s in the file.  ", i, i, freq[i], (freq[i] == 1) ? "time" : "times");
         } else if (i == 127) {
            fprintf(stderr, "Character DEL (0x%x) occurred %3d %-5s in the file.  ", i, freq[i], (freq[i] == 1) ? "time" : "times");
         } else {
            fprintf(stderr, "Character     (0x%x) occurred %3d %-5s in the file.  ", i, freq[i], (freq[i] == 1) ? "time" : "times");
         }

         fprintf(stderr, "Expected encoding is <%s>\n", code_values[i].path);
         num_chars++;
      }
   }

   // generate the original content for the user
   if (tree_head != NULL) {
      for (i = 0; i < tree_head->freq; i++) {
         generate_message(fd, tree_head, code, ASCII, freq);
         strncpy(code, "", MAX_PATH);
      }
   }

   fprintf(stderr, "Normal end of file reached\n");

   // close the input file
   if ((ret = close(fd)) != 0) {
      fprintf(stderr, "Failure to close the input file.\n");
      exit(1);
   }

   return 0;
}

void check_magic_num(int fd) {
   // variable declarations
   unsigned char magic_num[4] = {0x4C,0x70,0xF0,0x7C};
   unsigned char ptr[1] = {0};
   int i = 0, ret = 0;

   // check each magic number character
   for (i = 0; i < 4; i++) {
      //get the magic number character
      if ((ret = read(fd, ptr, sizeof(char))) != 1) {
         fprintf(stderr, "Failure to read magic number.\n");
         exit(1);
      }
      // compare the magic number from the file with the desired magic number
      if (*ptr != magic_num[i]) {
         fprintf(stderr, "Bad magic number in file. 0x%x does not match required 0x%x at byte %d\n", *ptr, magic_num[i], i);
         exit(1);
      }
   }

   return;
}

void get_bit_vector(int fd, unsigned char bit_vector[32]) {
   // variable declarations
   unsigned char ptr[1] = {0};
   int i = 0, ret = 0;

   // for all of the possible characters (ie. 32*8 = 256)
   for (i = 0; i < 32; i++) {
      // read one byte of the bit vector from the input file
      if ((ret = read(fd, ptr, sizeof(unsigned char))) != 1) {
         fprintf(stderr, "Failure to read magic number.\n");
         exit(1);
      }
      // reorder the bits and place in the bit vector
      bit_vector[i] |= ((*ptr & 0x01) << 7);
      bit_vector[i] |= ((*ptr & 0x02) << 5);
      bit_vector[i] |= ((*ptr & 0x04) << 3);
      bit_vector[i] |= ((*ptr & 0x08) << 1);
      bit_vector[i] |= ((*ptr & 0x10) >> 1);
      bit_vector[i] |= ((*ptr & 0x20) >> 3);
      bit_vector[i] |= ((*ptr & 0x40) >> 5);
      bit_vector[i] |= ((*ptr & 0x80) >> 7);
   }

   return;
}

int get_size(int fd) {
   // variable declarations
   int size = 0, ret = 0;
   unsigned char *c = (unsigned char *)&size;

   // get the size (in bytes 1-4) that the maximum frequency character has
   if ((ret = read(fd, c, sizeof(unsigned char))) != 1) {
      fprintf(stderr, "Failure to get the size of the frequency.\n");
      exit(1);
   }

   return size;
}

void get_character_counts(int fd, int freq[MAX_CHARS], int num_bytes,
      unsigned char bit_vector[32], const char *const ASCII[]) {

   // variable declarations
   int i = 0;

   // print which characters are in the file to the user
   for (i = 0; i < MAX_CHARS; i++) {
      // check if that bit is set in the bit vector - if so, print it to the user and get its frequency
      if ((bit_vector[i/8] >> (i%8)) & 0x01) {
         if (i < 33) {
            fprintf(stderr, "Character %3s (0x%02x) is in the file\n", ASCII[i], i);
         } else if (i < 127) {
            fprintf(stderr, "Character %3c (0x%x) is in the file\n", i, i);
         } else if (i == 127) {
            fprintf(stderr, "Character DEL (0x%x) is in the file\n", i);
         } else {
            fprintf(stderr, "Character     (0x%x) is in the file\n", i);
         }

         freq[i] = get_freq(fd, num_bytes);
      }
   }

   fprintf(stderr, "Each char frequency count will be %d %s long\n", num_bytes, (num_bytes == 1) ? "byte" : "bytes");

   return;
}

int get_freq(int fd, int num_bytes) {
   // variable declarations
   int freq = 0, i = 0, ret =0;
   char *ptr = (char *)&freq;


   // loop through the number of bytes specified, creating the frequency of that character
   for (i = (num_bytes - 1); i >= 0; i--) {
      // read in the frequency information
      if ((ret = read(fd, ptr+i, sizeof(char))) != 1) {
         fprintf(stderr, "Failure to read the frequency a certain character.\n");
         exit(1);
      }
   }

   return freq;
}

void generate_message(int fd, struct node *tree_head, char code[MAX_CHARS],
      const char *const ASCII[], int freq[MAX_CHARS]) {

   static int char_count = 0;
   // the node is a character
   if (tree_head->ch != -1) {
      printf("%c", (unsigned char)tree_head->ch);

      if (char_count < 50) {
         char_count++;
         freq[tree_head->ch]--;
         if (tree_head->ch < 33) {
            fprintf(stderr, "%2d. Translating bits <%s> to character %3s (0x%02x)\n", char_count, code, ASCII[tree_head->ch], tree_head->ch);
         } else if(tree_head->ch < 127) {
            fprintf(stderr, "%2d. Translating bits <%s> to character %3c (0x%02x)\n", char_count, code, tree_head->ch, tree_head->ch);
         } else if(tree_head->ch == 127) {
            fprintf(stderr, "%2d. Translating bits <%s> to character DEL (0x%02x)\n", char_count, code, tree_head->ch);
         } else {
            fprintf(stderr, "%2d. Translating bits <%s> to character     (0x%02x)\n", char_count, code, tree_head->ch);
         }

      } else if(char_count == 50) {
         char_count++;
         fprintf(stderr, "etc...\n");
      }
   } else {  // process through the tree depending on if the next bit is a 0 or 1
      if (get_bit(fd) == 0) {
         generate_message(fd, tree_head->p_left, strncat(code, "0", 1), ASCII, freq);
      } else {
         generate_message(fd, tree_head->p_right, strncat(code, "1", 1), ASCII, freq);
      }
   }

   return;
}

int get_bit(int fd) {
   // both variable are static because they are used to keep track of which bit
   // is next and what character we are referencing/looking at
   static int bit_count = 0;
   static unsigned char ch = '\0';

   // read a new character
   if (bit_count-- == 0) {
      read(fd, &ch, sizeof(unsigned char));
      bit_count = 7;
   }

   // return the next bit
   return ((ch >> bit_count) & 0x0001);
}



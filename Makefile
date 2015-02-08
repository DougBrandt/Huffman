
CC = gcc
CFLAGS = -g -c -Wall -Werror

all: huffman dehuffman

huffman: huffman.o tree_huff.o
	$(CC) huffman.o tree_huff.o -o huffman

dehuffman: dehuffman.o tree_huff.o
	$(CC) dehuffman.o tree_huff.o -o dehuffman

huffman.o: huffman.c
	$(CC) $(CFLAGS) -o huffman.o huffman.c

dehuffman.o: dehuffman.c
	$(CC) $(CFLAGS) -o dehuffman.o dehuffman.c

tree_huff.o: tree_huff.c
	$(CC) $(CFLAGS) -o tree_huff.o tree_huff.c

clean:
	rm -rf huffman dehuffman *.o

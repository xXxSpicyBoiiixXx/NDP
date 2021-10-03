#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>
#include <limits.h>

#define DEBUG 1

#define BLOCK_SIZE 4096

#define HASH_LEN 32

struct hashval {
    uint8_t bytes[HASH_LEN]
};    

struct merkle_node {
    struct hashval hv;
};

static inline void usage(char * prog) {
	printf("Usage: %s <n>\n", prog);
	printf("   <n>: number of blocks in file\n");
}

static inline void init_blob(char * a, size_t len) {
    for (int i = 0; i < len; i++) {
        a[i] = rand() % 256;
    }
}


static char *
concat (struct merkle_node * left, struct merkle_node * right) {
    // TODO: concatenate the hashes of the two merkle nodes together into one HASH_LEN*2 size block
    return NULL;
}

// THIS IS NOT A *REAL* HASH FUNCTION BEWARE
static void 
braindead_hash_fn (char * input, size_t len, char * output)
{
    for (int i = 0; i < len; i++) {
        output[i % HASH_LEN] ^= input[i];
    }
}

static struct merkle_node * 
construct_merkle_tree(char * blob, 
                      size_t len, 
                      void (*hash_fn)(char* input, size_t len, char* output)) {
    if (len == BLOCK_SIZE) {
        struct merkle_node * node = malloc(sizeof(struct merkle_node));
	if(node == NULL) 
	{
		printf("Memory allocation failed"); 
		return;
	}
        hash_fn(blob, len, node->hv.bytes);
        return node;
    } else {
        struct merkle_node * node = malloc(sizeof(struct merkle_node));
	if(node == NULL) 
	{
		printf("Memory allocation failed"); 
		return; 
	} 
        hash_fn(concat(construct_merkle_tree(blob, len/2, hash_fn),
                       construct_merkle_tree(blob + len/2, len/2, hash_fn)),
                HASH_LEN*2,
                node->hv.bytes);
        return node;
    }
}

static struct merkle_node *
recompute_merkle_tree(struct merkle_node * root, 
                      char * blob,
                      size_t len, 
                      unsigned changed_block_no, // which block changed?
                      void (*hash_fn)(char* input, size_t len, char* output)) {
    // TODO: 
    return NULL;
}

static inline void mutate_entry(struct merkle_node * tree, 
                                char * blob,
                                size_t len,
                                size_t idx,
                                char newval,
                                void (*hash_fn)(char*,size_t,char*)) {
    blob[idx] = newval;
    recompute_merkle_tree(tree, blob, len, idx / BLOCK_SIZE, hash_fn);
}


int main(int argc, char ** argv) {

    size_t blob_sz; // in bytes

    if (argc < 2) {
        usage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    blob_sz = atoll(argv[1]) * BLOCK_SIZE;

    char * blob = malloc(blob_sz);
    init_blob(blob, blob_sz);

    struct merkle_node * root = construct_merkle_tree(blob, blob_sz, braindead_hash_fn);
    if (!root) {
        fprintf(stderr, "Could not construct Merkle tree\n");
        exit(EXIT_FAILURE);
    }

    unsigned idx = 10; // TODO: get this from user (like array example)
    char newval = 'a'; // ^

    // mutation will automatically trigger recomputation of merkle tree
    mutate_entry(root, blob, blob_sz, idx, newval, braindead_hash_fn);

    return 0;
}


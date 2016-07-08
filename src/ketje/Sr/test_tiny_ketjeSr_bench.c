#include <stdio.h>
#include "tiny_ketjeSr.h"
#include "../../keccak/keccakP400.h"

#define OUTPUT
#define SnP_width 400

void print_in_hex_len(unsigned char* t, int len){
    int i =0;

    for (i = 0; i < len; i++){
        printf("%x ", t[i]);
    }    
    printf("\n");
}

static void displayByteString(FILE *f, const char* synopsis, const unsigned char *data, unsigned int length)
{
    unsigned int i;

    fprintf(f, "%s:", synopsis);
    for(i=0; i<length; i++)
        fprintf(f, " %02x", (unsigned char)data[i]);
    fprintf(f, "\n");
}

void generateSimpleRawMaterial(unsigned char* data, unsigned int length, unsigned char seed1, unsigned int seed2)
{
    unsigned int i;

    for( i=0; i<length; i++) {
        unsigned int iRolled = i*seed1;
        unsigned char byte = (iRolled+length+seed2)%0xFF;
        data[i] = byte;
    }
}

void dynamic_test(){

    int keySizeInBits = 0;  int keyMaxSizeInBits = 376;
     #ifdef OUTPUT
        FILE *f = fopen("dynamic_test_Sr.txt", "w");
    #endif

    for( keySizeInBits=keyMaxSizeInBits; keySizeInBits >=96; keySizeInBits -= (keySizeInBits > 200) ? 96 : ((keySizeInBits > 128) ? 24 : 16)){
        int nonceMaxSizeInBits = keyMaxSizeInBits - keySizeInBits;
        int nonceSizeInBits;
        for(nonceSizeInBits = nonceMaxSizeInBits; nonceSizeInBits >= ((keySizeInBits < 112) ? 0 : nonceMaxSizeInBits); nonceSizeInBits -= (nonceSizeInBits > 128) ? 160 : 64){
            
            Instance ketje1; memset(&ketje1, 0, sizeof(Instance));
            Instance ketje2; memset(&ketje2, 0, sizeof(Instance));
            unsigned char key[50], nonce[50]; memset(key, 0, 50*sizeof(unsigned char)); memset(nonce, 0, 50*sizeof(unsigned char));
            unsigned int ADlen; unsigned int keySize1; unsigned int nonceSize1;
            keySize1 = keySizeInBits / 8; nonceSize1 = nonceSizeInBits / 8;

            generateSimpleRawMaterial(key, keySize1, 0x12+nonceSizeInBits, SnP_width);
            generateSimpleRawMaterial(nonce, nonceSize1, 0x23+keySizeInBits, SnP_width);

            ketje_monkeyduplex_start(&ketje1, key, keySize1, nonce, nonceSize1);
            ketje_monkeyduplex_start(&ketje2, key, keySize1, nonce, nonceSize1);

#ifdef OUTPUT
            fprintf(f, "***\n");
            fprintf(f, "initialize with key of %u bits, nonce of %u bits:\n", keySizeInBits, nonceSizeInBits);
            displayByteString(f, "key", key, keySizeInBits/8);
            displayByteString(f, "nonce", nonce, nonceSizeInBits/8);
            fprintf(f, "\n");
#endif            
            unsigned int Nlen;
            for( ADlen=12; ADlen<40; ADlen++){
                for( Nlen=49-ADlen; Nlen> 0; Nlen--){
                    unsigned char associatedData[400], plaintext[400], ciphertext[400];
                    unsigned char plaintextPrime[400], tag1[16], tag2[16];

                    generateSimpleRawMaterial(associatedData, ADlen, 0x34+ Nlen, 3);
                    generateSimpleRawMaterial(plaintext, Nlen, 0x45+ ADlen, 4);

                    unsigned int ciphertext_size = wrap3(&ketje1, associatedData, ADlen, plaintext, Nlen, ciphertext);
                    unwrap3(&ketje2, associatedData, ADlen, ciphertext, ciphertext_size, plaintextPrime);
                    
                    generate_tag(&ketje1, tag1, 16);
                    generate_tag(&ketje2, tag2, 16);

                    if (memcmp(tag1, tag2, 16) != 0 ){
                        printf("tag1: "); print_in_hex_len(tag1, 16); 
                        printf("tag2: "); print_in_hex_len(tag2, 16);
                        printf("\n");
                    }
#ifdef OUTPUT
                    displayByteString(f, "associated data", associatedData, ADlen);
                    displayByteString(f, "plaintext", plaintext, Nlen);
                    displayByteString(f, "ciphertext", ciphertext, ciphertext_size);
                    displayByteString(f, "tag 1", tag1, 16);
                    displayByteString(f, "tag 2", tag2, 16);
                    fprintf(f, "\n");
#endif
                }
            }
        }
    }
#ifdef OUTPUT
    fclose(f);
    printf("Log wrote to dynamic_test_Jr.txt\n");
#endif
}
int enabled = 0;

unsigned int get_cycles(void) {
	unsigned int value = 0;
	if (!enabled) {
		/*
		 * This relies on a Kernel module described in:
		 * http://stackoverflow.com/questions/3247373/
		 * how-to-measure-program-execution-time-in-arm-cortex-a8-processor
		 */
		asm("mcr p15, 0, %0, c9, c12, 0" :: "r"(17));
		asm("mcr p15, 0, %0, c9, c12, 1" :: "r"(0x8000000f));
		asm("mcr p15, 0, %0, c9, c12, 3" :: "r"(0x8000000f));
		enabled = 1;
	}
	asm("mrc p15, 0, %0, c9, c13, 0" : "=r"(value));
	return value;
}
int main (){ 
    unsigned int cycles= 0;
    printf("%u\n",get_cycles());
    dynamic_test();
    printf("%u\n",get_cycles());
    
	return 0;
}

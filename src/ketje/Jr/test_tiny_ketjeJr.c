#include <stdio.h>
#include "tiny_ketjeJr.h"
#include "../../keccak/keccakP200.h"

#define OUTPUT
#define SnP_width 200


void print_state(unsigned char* state){
    int i =0;
    for (i = 0; i < nrLanes; i++){
        printf("%x ", state[i]);
    }    
    printf("\n");
}

void print_in_hex_len(unsigned char* t, unsigned int len){
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

    //176 for ketjeJr
    int keySizeInBits = 0;  int keyMaxSizeInBits = 176;
    //int keyMaxSizeInBits = SnP_width - 18;
     #ifdef OUTPUT
        FILE *f = fopen("dynamic_test_Jr.txt", "w");
    #endif

    for( keySizeInBits=keyMaxSizeInBits; keySizeInBits >=96; keySizeInBits -= (keySizeInBits > 200) ? 96 : ((keySizeInBits > 128) ? 24 : 16)){
        int nonceMaxSizeInBits = keyMaxSizeInBits - keySizeInBits;
        int nonceSizeInBits;
        for(nonceSizeInBits = nonceMaxSizeInBits; nonceSizeInBits >= ((keySizeInBits < 112) ? 0 : nonceMaxSizeInBits); nonceSizeInBits -= (nonceSizeInBits > 128) ? 161 : 64){
            
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
            for( ADlen=12; ADlen<13; ADlen++){
                for( Nlen=24-ADlen; Nlen> 24-ADlen-1; Nlen--){
                    unsigned char associatedData[400], plaintext[400], ciphertext[400];
                    unsigned char plaintextPrime[400], tag1[16], tag2[16];

                    generateSimpleRawMaterial(associatedData, ADlen, 0x34+ Nlen, 3);
                    generateSimpleRawMaterial(plaintext, Nlen, 0x45+ ADlen, 4);

                    unsigned int ciphertext_size = wrap3(&ketje1, associatedData, ADlen, plaintext, Nlen, ciphertext);
                    unwrap3(&ketje2, associatedData, ADlen, ciphertext, ciphertext_size, plaintextPrime);
                    
                    generate_tag(&ketje1, tag1, 16);
                    generate_tag(&ketje2, tag2, 16);

                    if (memcmp(tag1, tag2, 16) != 0 ){
                        printf("soma: %d\n", ADlen + Nlen);
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

int main (){ 

    // meu_teste();
    dynamic_test();
	
}
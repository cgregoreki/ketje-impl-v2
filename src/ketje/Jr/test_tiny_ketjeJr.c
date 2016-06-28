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

void print_in_hex(unsigned char* t){
    int i =0;

    for (i = 0; i < strlen(t); i++){
        printf("%x ", t[i]);
    }    
    printf("\n");
}

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

int hex2bin( const char *s )
{
    int ret=0;
    int i;
    for( i=0; i<2; i++ )
    {
        char c = *s++;
        int n=0;
        if( '0'<=c && c<='9' )
            n = c-'0';
        else if( 'a'<=c && c<='f' )
            n = 10 + c-'a';
        else if( 'A'<=c && c<='F' )
            n = 10 + c-'A';
        ret = n + ret*16;
    }
    return ret;
}

void getKeyAndNonce(unsigned char * key, unsigned char * nonce, int i){
    const char * keys[33] = { 
        "",
        "",
        "a",
        "esu",
        "jag",
        "eda",
        "mim",
        "vri",    
        "fribble",
        "alberti",
        "hugeous",
        "machado",
        "solutus",
        "preadmission",
        "isodimorphic",
        "quenchlessly",
        "deflationary",
        "compunctious",
        "superinfluencing",
        "classificational",
        "palaeoentomology",
        "conventionalised",
        "indigestibleness",
        "misapprehensiveranging",
        "nondistinguishableness",
        "counterrevolutionaries",
        "phenylethylmalonylurea",
        "noninterchangeableness",
        "",
        "",
        "",
        "",
        ""
    };

    const char * nonces[33]= {
        "",
        "a",
        "",
        "mid",
        "lag",
        "cdr",
        "ben",
        "crl",
        "balaton",
        "sennett",
        "minever",
        "anglify",
        "duotone",
        "commiously",
        "antisepous",
        "unmodeized",
        "nonexempon",
        "parallezed",
        "antico",
        "noncol",
        "subtri",
        "palaeo",
        "undera",
        "",
        "",
        "",
        "",
        "",
        "spectrophotometrically",
        "deoxyribonucleoprotein",
        "pseudoenthusiastically",
        "hexamethylenetetramine",
        "succinylsulphathiazole"
    };
    strcpy(key, keys[i]); strcpy(nonce,nonces[i]);
}

void getAB(char * associatedData, char * dataBody, int i ){

    const char * As[33] = {
        "phlebothrombosis",
        "suberose",
        "antidogmatical",
        "twistedly",
        "denasalizing",
        "precontemporaneous",
        "vigo",
        "hoofiness",
        "nonsane",
        "dandler",
        "sweetmeat",
        "bate",
        "bilateralness",
        "anthracoid",
        "cried",
        "bhishti",
        "gravitationally",
        "anisic",
        "doralice",
        "epileptiform",
        "cbd",
        "gtc",
        "swayback",
        "sabbat",
        "preantepenultimate",
        "traditionally",
        "twerp",
        "undeducible",
        "postbrachium",
        "cedartown",
        "bunchflower",
        "sedan",
        "unpartaking"
    };

    const char * Bs[33] = {
        "hinny",
        "parrakeet",
        "stelae",
        "glarus",
        "counterplotting",
        "changchow",
        "caseworker",
        "pompeian",
        "rawly",
        "methodically",
        "unmuttering",
        "recleansed",
        "maimonidean",
        "imputting",
        "unforeknowable",
        "semiresiny",
        "moshe",
        "noncatechistic",
        "overfertility",
        "upsides",
        "defrock",
        "amortization",
        "crumbum",
        "godey",
        "uredinial",
        "geog",
        "dasyurine",
        "outmost",
        "semipaste",
        "interpenetration",
        "reirrigating",
        "weer",
        "goutiness"
    };

    //memcpy(associatedData, As[i], strlen(As[i])); *dataBody = Bs[i];
    strcpy(associatedData, As[i]); strcpy(dataBody, Bs[i]);
}


int meu_teste(){

    #ifdef OUTPUT
        FILE *f = fopen("out_Jr.txt", "w");
    #endif

    int i, j = 0;

    for (i = 0; i < 33; i++){
        Instance ketje1, ketje2;
        unsigned char key[50], nonce[50];
        memset(key, 0, 50); memset(nonce, 0, 50);

        getKeyAndNonce(key, nonce, i);
        ketje_monkeyduplex_start(&ketje1, key, nonce);
        ketje_monkeyduplex_start(&ketje2, key, nonce);
        

#ifdef OUTPUT
        fprintf(f, "***\n");
        fprintf(f, "initialize with key of %u bits, nonce of %u bits:\n", (unsigned int) strlen(key)*8, (unsigned int) strlen(nonce)*8);
        displayByteString(f, "key", key, strlen(key));
        displayByteString(f, "nonce", nonce, strlen(nonce));
        fprintf(f, "\n");
#endif
        for (j = 0; j < 33; j++){
            // para cada chave, pega todos os bodys and associatedDatas e roda o ketje.
            memset(&ketje1, 0, sizeof(Instance));
            memset(&ketje2, 0, sizeof(Instance));
            ketje_monkeyduplex_start(&ketje1, key, nonce);
            ketje_monkeyduplex_start(&ketje2, key, nonce);    

            unsigned char A[400], B[400], C[400], B2[400], T1[16], T2[16];
            memset(A, 0, 400); memset(B, 0, 400); memset(C, 0, 400); memset(B2, 0, 400);
            memset(T1, 0, 16); memset(T2, 0, 16);

            getAB(A, B, j);

            wrap3(&ketje1, A, B, C);
            unwrap3(&ketje2, A, C, B);

            generate_tag(&ketje1, T1, 16);
            generate_tag(&ketje2, T2, 16);


#ifdef OUTPUT

            displayByteString(f, "associated data", A, strlen(A));
            displayByteString(f, "plaintext", B, strlen(B));
            displayByteString(f, "ciphertext", C, strlen(B));
            displayByteString(f, "tag 1", T1, 16);
            displayByteString(f, "tag 2", T2, 16);
            fprintf(f, "\n");
#endif
        }   
    }
#ifdef OUTPUT
    fclose(f);
    printf("Log wrote to out_Jr.txt\n");
    return 0;
#endif
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

    int soma[25]; memset(soma, 0, 25*sizeof(int));
    //176 for ketjeJr
    int keySizeInBits = 0;  int keyMaxSizeInBits = 176;
    //int keyMaxSizeInBits = SnP_width - 18;
     #ifdef OUTPUT
        FILE *f = fopen("dynamic_test", "w");
    #endif

    for( keySizeInBits=keyMaxSizeInBits; keySizeInBits >=96; keySizeInBits -= (keySizeInBits > 200) ? 96 : ((keySizeInBits > 128) ? 24 : 16)){
        int nonceMaxSizeInBits = keyMaxSizeInBits - keySizeInBits;
        int nonceSizeInBits;
        for(nonceSizeInBits = nonceMaxSizeInBits; nonceSizeInBits >= ((keySizeInBits < 112) ? 0 : nonceMaxSizeInBits); nonceSizeInBits -= (nonceSizeInBits > 128) ? 161 : 64){
            printf("keySizeInBits: %d\t nonceSizeInBits: %d\n", keySizeInBits, nonceSizeInBits);
            Instance ketje1; memset(&ketje1, 0, sizeof(Instance));
            Instance ketje2; memset(&ketje2, 0, sizeof(Instance));
            unsigned char key[50], nonce[50]; memset(key, 0, 50*sizeof(unsigned char)); memset(nonce, 0, 50*sizeof(unsigned char));
            unsigned int ADlen; unsigned int keySize1; unsigned int nonceSize1;
            keySize1 = keySizeInBits / 8; nonceSize1 = nonceSizeInBits / 8;
            //printf("temp: %u\n", temp);

            generateSimpleRawMaterial(key, keySize1, 0x12+nonceSizeInBits, SnP_width);
            generateSimpleRawMaterial(nonce, nonceSize1, 0x23+keySizeInBits, SnP_width);

            printf("keylen: %d\t", (int) strlen(key)); print_in_hex_len(key, keySize1);
            print_in_hex(nonce);

            ketje_monkeyduplex_start(&ketje1, key, nonce);
            ketje_monkeyduplex_start(&ketje2, key, nonce);

            int mult = 1;
            unsigned int Nlen;
            for( ADlen=12; ADlen<20; ADlen++){
                for( Nlen=19-ADlen; Nlen> 0; Nlen--){
                    unsigned char associatedData[400], plaintext[400], ciphertext[400];
                    unsigned char plaintextPrime[400], tag1[16], tag2[16];
                    
                    memset(tag1, 0, sizeof(unsigned char)*16);
                    memset(tag2, 0, sizeof(unsigned char)*16);

                    memset(associatedData, 0, 400*sizeof(unsigned char)); memset(plaintext, 0, 400*sizeof(unsigned char));
                    memset(ciphertext, 0, 400*sizeof(unsigned char)); memset(plaintextPrime, 0, 400*(sizeof(unsigned char)));
                    
                    generateSimpleRawMaterial(associatedData, ADlen, 0x34+ Nlen, 3);
                    generateSimpleRawMaterial(plaintext, Nlen, 0x45+ ADlen, 4);

                    //printf("associatedData: "); print_in_hex(associatedData);

                    wrap3(&ketje1, associatedData, plaintext, ciphertext);
                    unwrap3(&ketje2, associatedData, ciphertext, plaintextPrime);
                    //printf("associatedData len: %d\n", (int)strlen(associatedData));
                    
                    generate_tag(&ketje1, tag1, 16);
                    generate_tag(&ketje2, tag2, 16);

                    if (memcmp(tag1, tag2, 16) != 0 ){
                        // printf("soma: %d\n", ADlen + Nlen);
                        soma[ADlen + Nlen] = soma[ADlen + Nlen] + 1;
                        // printf("tag1: "); print_in_hex_len(tag1, 16); 
                        // printf("tag2: "); print_in_hex_len(tag2, 16);
                        // printf("\n");
                    }
                }
            }
        }
    }

    int i = 0;
    for (i = 0; i < 25; i ++){
        // printf("%d: %d\n", i, soma[i]);
    }
}

int main (){ 

    // meu_teste();
    dynamic_test();
	
}
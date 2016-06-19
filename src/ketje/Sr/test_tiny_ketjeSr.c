#include <stdio.h>
#include "tiny_ketjeSr.h"
#include "../../keccak/keccakP400.h"

#define OUTPUT
#define SnP_width 400


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
    strcpy(associatedData, As[i]); strcpy(dataBody, Bs[i]);
}


int test_sr(){

    #ifdef OUTPUT
        FILE *f = fopen("my_test.txt", "w");
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
        fprintf(f, "initialize with key of %u bits, nonce of %u bits:\n", strlen(key)*8, strlen(nonce)*8);
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

            //printf("after start:\n"); print_state(ketje1.state);  

            unsigned char A[400], B[400], C[400], B2[400], T1[17], T2[17];
            memset(A, 0, 400); memset(B, 0, 400); memset(C, 0, 400); memset(B2, 0, 400);
            memset(T1, 0, 17); memset(T2, 0, 17);

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

    fclose(f);
    return 0;
}

int main (){ 

    test_sr();
	
}
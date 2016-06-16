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

void generateSimpleRawMaterial(unsigned char* data, unsigned int length, unsigned char seed1, unsigned int seed2)
{
    unsigned int i;

    for( i=0; i<length; i++) {
        unsigned int iRolled = i*seed1;
        unsigned char byte = (iRolled+length+seed2)%0xFF;
        data[i] = byte;
    }
}

void assert_2(int condition, char * synopsis)
{
    if (!condition)
    {
        printf("%s\n", synopsis);
        exit(1);
    }
}

unsigned int myMin(unsigned int a, unsigned int b)
{
    return (a < b) ? a : b;
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

void test_ketje(){

	char buf[256];
	FILE *f = fopen("KetjeJr.txt", "r");
	int found_EOL = 0;
	char * p;

	char actual_line[256];

	while (fgets (buf, sizeof(buf), f)) {
		printf("%s", buf);
	}
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

            unsigned char A[400], B[400], C[400], B2[400], T1[17], T2[17];
            memset(A, 0, 400); memset(B, 0, 400); memset(C, 0, 400); memset(B2, 0, 400);
            memset(T1, 0, 17); memset(T2, 0, 17);

            getAB(A, B, j);

            wrap3(&ketje1, A, B, C);
            unwrap3(&ketje2, A, C, B);

            generate_tag(&ketje1, T1, 16);
            generate_tag(&ketje2, T2, 16);
            //T1[16] = '\0'; T2[16] = '\0';

            // if (memcmp(T1, T2, 16) != 0){
            //     printf("problemas!!\n");
            //     printf("key %s len: %d :", key, strlen(key)); print_in_hex(key);
            //     printf("nonce %s len: %d :", nonce, strlen(nonce)); print_in_hex(nonce);
            //     printf("associated data "); print_in_hex(A);
            //     printf("plaintext "); print_in_hex(B);
            //     printf("ciphertext "); print_in_hex(C);
            //     printf("tag1 "); print_in_hex(T1);
            //     printf("tag2 "); print_in_hex(T2);
            // }

#ifdef OUTPUT

            displayByteString(f, "associated data", A, strlen(A));
            displayByteString(f, "plaintext", B, strlen(B));
            displayByteString(f, "ciphertext", C, strlen(B));
            displayByteString(f, "tag 1", T1, 16);
            displayByteString(f, "tag 2", T2, 16);
            fprintf(f, "\n");
#endif
            printf("\nRESULTADO: \n***\n");
            printf("key %s len: %d :", key, strlen(key)); print_in_hex(key);
            printf("nonce %s len: %d :", nonce, strlen(nonce)); print_in_hex(nonce);
            printf("associated data "); print_in_hex(A);
            printf("plaintext "); print_in_hex(B);
            printf("ciphertext "); print_in_hex(C);
            printf("tag1 "); print_in_hex_len(T1, 16);
            printf("tag2 "); print_in_hex_len(T2, 16);
            printf("***\n");
        }   
    }

    fclose(f);
    return 0;
}

void teste_particular(){
    int i = 13; int j = 0;
    Instance ketje1, ketje2;
    unsigned char key[50], nonce[50];
    memset(key, 0, 50); memset(nonce, 0, 50);

    getKeyAndNonce(key, nonce, i);
    ketje_monkeyduplex_start(&ketje1, key, nonce);
    ketje_monkeyduplex_start(&ketje2, key, nonce);

    unsigned char A[400], B[400], C[400], B2[400], T1[16], T2[16];
    memset(A, 0, 400); memset(B, 0, 400); memset(C, 0, 400); memset(B2, 0, 400);
    memset(T1, 0, 16); memset(T2, 0, 16);

    getAB(A, B, j);

    // printf("before wrap: \n");
    // print_state(ketje1.state);

    wrap3(&ketje1, A, B, C);
    // printf("after wrap: \n");
    // print_state(ketje1.state);

    // printf("before unwrap: \n");
    // print_state(ketje2.state);
    unwrap3(&ketje2, A, C, B);

    // printf("unwrap unwrap: \n");
    // print_state(ketje2.state);

    generate_tag(&ketje1, T1, 16);
    generate_tag(&ketje2, T2, 16);

    printf("\nRESULTADO: \n***\n");
    printf("key %s len: %d :", key, strlen(key)); print_in_hex(key);
    printf("nonce %s len: %d :", nonce, strlen(nonce)); print_in_hex(nonce);
    printf("associated data "); print_in_hex(A);
    printf("plaintext "); print_in_hex(B);
    printf("ciphertext "); print_in_hex(C);
    printf("tag1 "); print_in_hex_len(T1,16);
    printf("tag2 "); print_in_hex_len(T2, 16);
    printf("***\n");

}

int main (){ 

    meu_teste();
    teste_particular();
	/*
	unsigned char state[KeccakF_stateSizeInBytes];
  
	Instance ketj;
	Instance ketj_unwrapp;
	unsigned char key[maxKeyBytes] = {
		'C', 'A', 'R', 'L', 'O', 'S'
	};

	unsigned char nonce[maxKeyBytes] = {
		'N', 'A', 'D', 'A'
	};

	ketje_monkeyduplex_start(&ketj, key, nonce);
	ketje_monkeyduplex_start(&ketj_unwrapp, key, nonce);
	printf("after initialize: \n");
	print_state(ketj.state);

	unsigned char *associatedData = "Gregoreki";

	unsigned char text[400];
    unsigned char *aux = "meu texto";
    strcpy(text, aux);

    unsigned char cipher_t[400]; memset(cipher_t, 0, 400);
    unsigned char tag[400]; unsigned char tag2[400]; memset(tag, 0, 400); memset(tag2, 0, 400);
    unsigned char unwrapped[400];
    memset(unwrapped, 0, 400);

	wrap3(&ketj, associatedData, text, cipher_t);
	printf("after wrap\n");
	print_state(ketj.state);
	unwrap3(&ketj_unwrapp, associatedData, cipher_t, unwrapped);

	printf("after UNWRAP\n");
	print_state(ketj_unwrapp.state);
	printf("original: " ); print_in_hex(text);
	printf("recuperada: "); print_in_hex(unwrapped);
	printf("cipher_t: "); print_in_hex(cipher_t);
	printf("strlenC: %d\n", strlen(cipher_t));

	
	generate_tag(&ketj, tag, 16);
	generate_tag(&ketj_unwrapp, tag2, 16);
	printf("tag1:\t"); print_in_hex(tag);
	printf("tag2:\t"); print_in_hex(tag2);
	*/
}
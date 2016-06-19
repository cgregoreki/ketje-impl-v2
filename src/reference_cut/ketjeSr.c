#include <stdio.h>
#include <string.h>

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef UINT16 tKeccakLane;
#define KeccakP400_width 400
#define KeccakF_stateSizeInBytes 50

#define Ketje_BlockSize (2*KeccakP400_width/8/25)

#define FRAMEBITSEMPTY  0x01
#define FRAMEBITS0      0x02
#define FRAMEBITS00     0x04
#define FRAMEBITS10     0x05
#define FRAMEBITS01     0x06
#define FRAMEBITS11     0x07
#define Ket_StartRounds     12
#define Ket_StepRounds      1
#define Ket_StrideRounds    6

#define maxNrRounds 20
tKeccakLane KeccakRoundConstants[maxNrRounds];
#define nrLanes 25
#define OUTPUT
unsigned int KeccakRhoOffsets[nrLanes];

typedef struct {
    /** The state processed by the permutation. */
    unsigned char state[KeccakF_stateSizeInBytes];

    /** The phase. */
    unsigned int phase;

    /** The amount of associated or plaintext data that has been
      * XORed into the state after the last call to step or stride */
    unsigned int dataRemainderSize;
} Ketje_Instance;

enum Phase {
    Ketje_Phase_Virgin          = 0,
    Ketje_Phase_FeedingAssociatedData   = 1,
    Ketje_Phase_Wrapping        = 2,
    Ketje_Phase_Unwrapping      = 4
};

int LFSR86540(UINT8 *LFSR)
{
    int result = ((*LFSR) & 0x01) != 0;
    if (((*LFSR) & 0x80) != 0)
        // Primitive polynomial over GF(2): x^8+x^6+x^5+x^4+1
        (*LFSR) = ((*LFSR) << 1) ^ 0x71;
    else
        (*LFSR) <<= 1;
    return result;
}

void KeccakP400_AddByte(void *state, unsigned char byte, unsigned int offset)
{
    //assert(offset < 50);
    ((unsigned char *)state)[offset] ^= byte;
}

/* ---------------------------------------------------------------- */

void KeccakP400_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    unsigned int i;

    //assert(offset < 50);
    //assert(offset+length <= 50);
    for(i=0; i<length; i++)
        ((unsigned char *)state)[offset+i] ^= data[i];
}

/* ---------------------------------------------------------------- */

void KeccakP400_OverwriteBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    //assert(offset < 50);
    //assert(offset+length <= 50);
    memcpy((unsigned char*)state+offset, data, length);
}


void KeccakP400_InitializeRoundConstants()
{
    UINT8 LFSRstate = 0x01;
    unsigned int i, j, bitPosition;

    for(i=0; i<maxNrRounds; i++) {
        KeccakRoundConstants[i] = 0;
        for(j=0; j<7; j++) {
            bitPosition = (1<<j)-1; //2^j-1
            if (LFSR86540(&LFSRstate) && (bitPosition < (sizeof(tKeccakLane)*8)))
                KeccakRoundConstants[i] ^= (tKeccakLane)(1<<bitPosition);
        }
    }
}


#define index(x, y) (((x)%5)+5*((y)%5))

void KeccakP400_InitializeRhoOffsets()
{
    unsigned int x, y, t, newX, newY;

    KeccakRhoOffsets[index(0, 0)] = 0;
    x = 1;
    y = 0;
    for(t=0; t<24; t++) {
        KeccakRhoOffsets[index(x, y)] = ((t+1)*(t+2)/2) % (sizeof(tKeccakLane) * 8);
        newX = (0*x+1*y) % 5;
        newY = (2*x+3*y) % 5;
        x = newX;
        y = newY;
    }
}
void KeccakP400_Initialize(void *state)
{
    memset(state, 0, nrLanes * sizeof(tKeccakLane));
}


void print_round_constants(){
	int i = 0;
	printf("Round constants:\n");
	for (i = 0; i < 20; i++){
		printf("%x, ", KeccakRoundConstants[i]);
	}
	printf("\n");
}


void printf_rho_offsets(){
    int i = 0;
    printf("rho offsets:\n");
    for (i = 0; i < nrLanes; i++){
        printf("%d, ", KeccakRhoOffsets[i]);
    }
    printf("\n");
}


unsigned char Ket_StateExtractByte( void *state, unsigned int offset )
{
    unsigned char data[1];

    KeccakP400_ExtractBytes(state, data, offset, 1);
    return data[0];
}

void KeccakP400_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length)
{
    //assert(offset < 50);
    //assert(offset+length <= 50);
    memcpy(data, (unsigned char*)state+offset, length);
}

void Ket_StateOverwrite( void *state, unsigned int offset, const unsigned char *data, unsigned int length )
{
    KeccakP400_OverwriteBytes(state, data, offset, length);
}

/* Ketje low level functions    */

void Ket_Step( void *state, unsigned int size, unsigned char frameAndPaddingBits)
{

    KeccakP400_AddByte(state, frameAndPaddingBits, size);
    KeccakP400_AddByte(state, 0x08, Ketje_BlockSize );
    keccakP400NRounds(state, Ket_StepRounds );
}

void Ket_FeedAssociatedDataBlocks( void *state, const unsigned char *data, unsigned int nBlocks )
{

    do
    {
        KeccakP400_AddByte( state, *(data++), 0 );
        KeccakP400_AddByte( state, *(data++), 1 );
        #if (KeccakP400_width == 400 )
        KeccakP400_AddByte( state, *(data++), 2 );
        KeccakP400_AddByte( state, *(data++), 3 );
        #endif
        Ket_Step( state, Ketje_BlockSize, FRAMEBITS00 );
    }
    while ( --nBlocks != 0 );
}

void Ket_UnwrapBlocks( void *state, const unsigned char *ciphertext, unsigned char *plaintext, unsigned int nBlocks )
{
    unsigned char tempBlock[Ketje_BlockSize];
    unsigned char frameAndPaddingBits[1];
    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;

    while ( nBlocks-- != 0 )
    {
        KeccakP400_ExtractBytes(state, tempBlock, 0, Ketje_BlockSize);
        tempBlock[0] = *(plaintext++) = *(ciphertext++) ^ tempBlock[0];
        tempBlock[1] = *(plaintext++) = *(ciphertext++) ^ tempBlock[1];
        #if (KeccakP400_width == 400 )
        tempBlock[2] = *(plaintext++) = *(ciphertext++) ^ tempBlock[2];
        tempBlock[3] = *(plaintext++) = *(ciphertext++) ^ tempBlock[3];
        #endif
        KeccakP400_AddBytes(state, tempBlock, 0, Ketje_BlockSize);
        KeccakP400_AddBytes(state, frameAndPaddingBits, Ketje_BlockSize, 1);
        keccakP400NRounds(state, Ket_StepRounds);
    }
}

void Ket_WrapBlocks( void *state, const unsigned char *plaintext, unsigned char *ciphertext, unsigned int nBlocks )
{
    unsigned char keystream[Ketje_BlockSize];
    unsigned char plaintemp[Ketje_BlockSize];
    unsigned char frameAndPaddingBits[1];
    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;
    //printf("blocks...\n");
    while ( nBlocks-- != 0 )
    {
        KeccakP400_ExtractBytes(state, keystream, 0, Ketje_BlockSize);
        plaintemp[0] = plaintext[0];
        plaintemp[1] = plaintext[1];
        #if (KeccakP400_width == 400 )
        plaintemp[2] = plaintext[2];
        plaintemp[3] = plaintext[3];
        #endif
        *(ciphertext++) = *(plaintext++) ^ keystream[0];
        *(ciphertext++) = *(plaintext++) ^ keystream[1];
        #if (KeccakP400_width == 400 )
        *(ciphertext++) = *(plaintext++) ^ keystream[2];
        *(ciphertext++) = *(plaintext++) ^ keystream[3];
        #endif
        KeccakP400_AddBytes(state, plaintemp, 0, Ketje_BlockSize);
        KeccakP400_AddBytes(state, frameAndPaddingBits, Ketje_BlockSize, 1);
        //print_state(state);
        keccakP400NRounds(state, Ket_StepRounds);
        //print_state(state);
    }
}



int Ketje_Initialize(Ketje_Instance *instance, const unsigned char *key, unsigned int keySizeInBits, const unsigned char *nonce, unsigned int nonceSizeInBits)
{
    unsigned char smallData[1];
    unsigned int keyPackSizeInBits;

    keyPackSizeInBits = 8*((keySizeInBits+16)/8);
    if ( (keyPackSizeInBits + nonceSizeInBits + 2) > KeccakP400_width) {
        return 1;
	}

    instance->phase = Ketje_Phase_FeedingAssociatedData;
    instance->dataRemainderSize = 0;

    //KeccakP400_StaticInitialize();
    KeccakP400_Initialize(instance->state);

    // Key pack
    smallData[0] = keySizeInBits / 8 + 2;
    Ket_StateOverwrite( instance->state, 0, smallData, 1 );
    Ket_StateOverwrite( instance->state, 1, key, keySizeInBits/8 );
    if ((keySizeInBits % 8) == 0)
        smallData[0] = 0x01;
    else {
        unsigned char padding = (unsigned char)1 << (keySizeInBits%8);
        unsigned char mask = padding-1;
        smallData[0] = (key[keySizeInBits/8] & mask) | padding;
    }
    Ket_StateOverwrite( instance->state, 1+keySizeInBits/8, smallData, 1 );

    // Nonce
    Ket_StateOverwrite( instance->state, 1+keySizeInBits/8+1, nonce, nonceSizeInBits / 8 );
    if ((nonceSizeInBits % 8) == 0)
        smallData[0] = 0x01;
    else {
        unsigned char padding = (unsigned char)1 << (nonceSizeInBits%8);
        unsigned char mask = padding-1;
        smallData[0] = (nonce[nonceSizeInBits/8] & mask) | padding;
    }
    Ket_StateOverwrite( instance->state, 1+keySizeInBits/8+1+nonceSizeInBits/8, smallData, 1 );

    KeccakP400_AddByte(instance->state, 0x80, KeccakP400_width / 8 - 1 );
    keccakP400NRounds(instance->state, Ket_StartRounds );

    return 0;
}

int Ketje_FeedAssociatedData(Ketje_Instance *instance, const unsigned char *data, unsigned int dataSizeInBytes)
{
    //printf("before put_headers...\n"); print_state(instance->state);
    unsigned int size;

    if ((instance->phase & Ketje_Phase_FeedingAssociatedData) == 0)
        return 1;

    if ( (instance->dataRemainderSize + dataSizeInBytes) > Ketje_BlockSize )
    {
        if (instance->dataRemainderSize != 0)
        {
            dataSizeInBytes -= Ketje_BlockSize - instance->dataRemainderSize;
            while ( instance->dataRemainderSize != Ketje_BlockSize )
                KeccakP400_AddByte( instance->state, *(data++), instance->dataRemainderSize++ );
            Ket_Step( instance->state, Ketje_BlockSize, FRAMEBITS00 );
            instance->dataRemainderSize = 0;
        }

        if ( dataSizeInBytes > Ketje_BlockSize )
        {
            size = ((dataSizeInBytes + (Ketje_BlockSize - 1)) & ~(Ketje_BlockSize - 1)) - Ketje_BlockSize;
            Ket_FeedAssociatedDataBlocks( instance->state, data, size / Ketje_BlockSize);
            dataSizeInBytes -= size;
            data += size;
        }
    }

    while ( dataSizeInBytes-- != 0 )
        KeccakP400_AddByte( instance->state, *(data++), instance->dataRemainderSize++ );

    //printf("after put_headers...\n"); print_state(instance->state);
    return 0;
}

int Ketje_WrapPlaintext(Ketje_Instance *instance, const unsigned char *plaintext, unsigned char *ciphertext, unsigned int dataSizeInBytes )
{
    unsigned int size;
    unsigned char temp;

    //printf("starting wrap...\n"); print_state(instance->state);
    if ( (instance->phase & Ketje_Phase_FeedingAssociatedData) != 0)
    {
        Ket_Step( instance->state, instance->dataRemainderSize, FRAMEBITS01 );
        instance->dataRemainderSize = 0;
        instance->phase = Ketje_Phase_Wrapping;
    }
    //printf("after STEP 1 wrap...\n"); print_state(instance->state);

    if ( (instance->phase & Ketje_Phase_Wrapping) == 0)
        return 1;

    if ( (instance->dataRemainderSize + dataSizeInBytes) > Ketje_BlockSize )
    {
        // More than a block
        if (instance->dataRemainderSize != 0)
        {
            // Process data remainder
            while ( instance->dataRemainderSize < Ketje_BlockSize )
            {
                temp = *(plaintext++);
                *(ciphertext++) = temp ^ Ket_StateExtractByte( instance->state, instance->dataRemainderSize );
                KeccakP400_AddByte( instance->state, temp, instance->dataRemainderSize++ );
                --dataSizeInBytes;
            }
            Ket_Step( instance->state, Ketje_BlockSize, FRAMEBITS11 );
            instance->dataRemainderSize = 0;
        }

        //  Wrap multiple blocks except last.
        if ( dataSizeInBytes > Ketje_BlockSize )
        {
            //printf("before blocks...\n"); print_state(instance->state);
            size = ((dataSizeInBytes + (Ketje_BlockSize - 1)) & ~(Ketje_BlockSize - 1)) - Ketje_BlockSize;
            Ket_WrapBlocks( instance->state, plaintext, ciphertext, size / Ketje_BlockSize );
            dataSizeInBytes -= size;
            plaintext += size;
            ciphertext += size;
            //printf("after blocks...\n"); print_state(instance->state);
        }
    }

    //  Add remaining data
    while ( dataSizeInBytes-- != 0 )
    {
        temp = *(plaintext++);
        *(ciphertext++) = temp ^ Ket_StateExtractByte( instance->state, instance->dataRemainderSize );
        KeccakP400_AddByte( instance->state, temp, instance->dataRemainderSize++ );
    }

    return 0;
}

int Ketje_UnwrapCiphertext(Ketje_Instance *instance, const unsigned char *ciphertext, unsigned char *plaintext, unsigned int dataSizeInBytes)
{
    unsigned int size;
    unsigned char temp;

    if ( (instance->phase & Ketje_Phase_FeedingAssociatedData) != 0)
    {
        Ket_Step( instance->state, instance->dataRemainderSize, FRAMEBITS01 );
        instance->dataRemainderSize = 0;
        instance->phase = Ketje_Phase_Unwrapping;
    }

    if ( (instance->phase & Ketje_Phase_Unwrapping) == 0)
        return 1;

    if ( (instance->dataRemainderSize + dataSizeInBytes) > Ketje_BlockSize )
    {
        // More than a block
        if (instance->dataRemainderSize != 0)
        {
            // Process data remainder
            while ( instance->dataRemainderSize < Ketje_BlockSize )
            {
                temp = *(ciphertext++) ^ Ket_StateExtractByte( instance->state, instance->dataRemainderSize );
                *(plaintext++) = temp;
                KeccakP400_AddByte( instance->state, temp, instance->dataRemainderSize++ );
                --dataSizeInBytes;
            }
            Ket_Step( instance->state, Ketje_BlockSize, FRAMEBITS11 );
            instance->dataRemainderSize = 0;
        }

        //  Unwrap multiple blocks except last.
        if ( dataSizeInBytes > Ketje_BlockSize )
        {
            size = ((dataSizeInBytes + (Ketje_BlockSize - 1)) & ~(Ketje_BlockSize - 1)) - Ketje_BlockSize;
            Ket_UnwrapBlocks( instance->state, ciphertext, plaintext, size / Ketje_BlockSize );
            dataSizeInBytes -= size;
            plaintext += size;
            ciphertext += size;
        }
    }

    //  Add remaining data
    while ( dataSizeInBytes-- != 0 )
    {
        temp = *(ciphertext++) ^ Ket_StateExtractByte( instance->state, instance->dataRemainderSize );
        *(plaintext++) = temp;
        KeccakP400_AddByte( instance->state, temp, instance->dataRemainderSize++ );
    }

    return 0;
}

int Ketje_GetTag(Ketje_Instance *instance, unsigned char *tag, unsigned int tagSizeInBytes)
{
    unsigned int tagSizePart;
    unsigned int i;

    if ((instance->phase & (Ketje_Phase_Wrapping | Ketje_Phase_Unwrapping)) == 0)
        return 1;

    KeccakP400_AddByte(instance->state, FRAMEBITS10, instance->dataRemainderSize);
    KeccakP400_AddByte(instance->state, 0x08, Ketje_BlockSize);    //padding
    keccakP400NRounds(instance->state, Ket_StrideRounds );
    instance->dataRemainderSize = 0;
    tagSizePart = Ketje_BlockSize;
    if ( tagSizeInBytes < Ketje_BlockSize )
        tagSizePart = tagSizeInBytes;
    for ( i = 0; i < tagSizePart; ++i )
        *(tag++) = Ket_StateExtractByte( instance->state, i );
    tagSizeInBytes -= tagSizePart;

    while(tagSizeInBytes > 0)
    {
        Ket_Step( instance->state, 0, FRAMEBITS0 );
        tagSizePart = Ketje_BlockSize;
        if ( tagSizeInBytes < Ketje_BlockSize )
            tagSizePart = tagSizeInBytes;
        for ( i = 0; i < tagSizePart; ++i )
            *(tag++) = Ket_StateExtractByte( instance->state, i );
        tagSizeInBytes -= tagSizePart;
    }

    instance->phase = Ketje_Phase_FeedingAssociatedData;

    return 0;
}

void print_state(unsigned char* state){
    int i =0;
    for (i = 0; i < nrLanes; i++){
        printf("%x ", state[i]);
    }    
    printf("\n");
}

void test_instance_initialize(tKeccakLane* state){
    int i = 0;
    tKeccakLane aux[6] = {
        'C', 'a', 'r', 'l', 'o', 's'
    };

    for (i = 0; i < 6; i++){
            state[i] = aux[i];
    }
}

void print_in_hex(unsigned char* t){
        int i =0;

        for (i = 0; i < strlen(t); i++){
            printf("%x ", t[i]);
        }    
        printf("\n");
    }

void print_instance_details(Ketje_Instance* instance){
    printf("dataRemainderSize: %d\tphase: %d\n", instance->dataRemainderSize, instance->phase);
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

static void displayByteString(FILE *f, const char* synopsis, const unsigned char *data, unsigned int length)
{
    unsigned int i;

    fprintf(f, "%s:", synopsis);
    for(i=0; i<length; i++)
        fprintf(f, " %02x", (unsigned int)data[i]);
    fprintf(f, "\n");
}

int meu_teste(){

    #ifdef OUTPUT
        FILE *f = fopen("my_test.txt", "w");
    #endif

    int i, j = 0;

    for (i = 0; i < 33; i++){
        Ketje_Instance ketje1, ketje2;
        unsigned char key[50], nonce[50];
        memset(key, 0, 50); memset(nonce, 0, 50);

        getKeyAndNonce(key, nonce, i);
        Ketje_Initialize(&ketje1, key, strlen(key)*8,nonce,strlen(nonce)*8);
        Ketje_Initialize(&ketje2, key, strlen(key)*8,nonce,strlen(nonce)*8);

#ifdef OUTPUT
        fprintf(f, "***\n");
        fprintf(f, "initialize with key of %u bits, nonce of %u bits:\n", strlen(key)*8, strlen(nonce)*8);
        displayByteString(f, "key", key, strlen(key));
        displayByteString(f, "nonce", nonce, strlen(nonce));
        fprintf(f, "\n");
#endif
        for (j = 0; j < 33; j++){
            // para cada chave, pega todos os bodys and associatedDatas e roda o ketje.
            memset(&ketje1, 0, sizeof(Ketje_Instance));
            memset(&ketje2, 0, sizeof(Ketje_Instance));
            Ketje_Initialize(&ketje1, key, strlen(key)*8,nonce,strlen(nonce)*8);
            Ketje_Initialize(&ketje2, key, strlen(key)*8,nonce,strlen(nonce)*8);  

            //printf("after start:\n"); print_state(ketje1.state);  

            unsigned char A[400], B[400], C[400], B2[400], T1[16], T2[16];
            memset(A, 0, 400); memset(B, 0, 400); memset(C, 0, 400); memset(B2, 0, 400);
            memset(T1, 0, 16); memset(T2, 0, 16);

            getAB(A, B, j);

            Ketje_FeedAssociatedData(&ketje1, A, strlen(A));
            Ketje_FeedAssociatedData(&ketje2, A, strlen(A));

            Ketje_WrapPlaintext(&ketje1, B, C, strlen(B));
            Ketje_UnwrapCiphertext(&ketje2, C, B2, strlen(C));

            Ketje_GetTag(&ketje1, T1, 16);
            Ketje_GetTag(&ketje2, T2, 16);

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
            //getchar();        
        }   
    }

    fclose(f);
    return 0;
}

void print_in_hex_len(unsigned char* t, int len){
    int i =0;

    for (i = 0; i < len; i++){
        printf("%x ", t[i]);
    }    
    printf("\n");
}



void teste_particular(){
    int i = 32 ; int j = 32;
    Ketje_Instance ketje1, ketje2;
    unsigned char key[50], nonce[50];
    memset(key, 0, 50); memset(nonce, 0, 50);

    getKeyAndNonce(key, nonce, i);
    Ketje_Initialize(&ketje1, key, strlen(key)*8,nonce,strlen(nonce)*8);
    Ketje_Initialize(&ketje2, key, strlen(key)*8,nonce,strlen(nonce)*8);

    unsigned char A[400]; unsigned char B[400]; unsigned char C[400]; unsigned char B2[400];
    unsigned char T1[16]; unsigned char T2[16];
    memset(A, 0, 400); memset(B, 0, 400); memset(C, 0, 400); memset(B2, 0, 400);
    memset(T1, 0, 16); memset(T2, 0, 16);

    getAB(A, B, j);

    Ketje_FeedAssociatedData(&ketje1, A, strlen(A));
    Ketje_FeedAssociatedData(&ketje2, A, strlen(A));

    Ketje_WrapPlaintext(&ketje1, B, C, strlen(B));
    Ketje_UnwrapCiphertext(&ketje2, C, B2, strlen(C));

    Ketje_GetTag(&ketje1, T1, 16);
    Ketje_GetTag(&ketje2, T2, 16);

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

int main(){
    meu_teste();
    //teste_particular();
	return 0;
}
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define SnP_width 200

typedef unsigned char UINT8;
typedef unsigned int UINT32;
typedef UINT8 tKeccakLane;


#define KeccakF_stateSizeInBytes 25

#define maxNrRounds 18
tKeccakLane KeccakRoundConstants[maxNrRounds];
#define nrLanes 25
unsigned int KeccakRhoOffsets[nrLanes];

#define Ket_StartRounds     12
#define Ket_StepRounds      1
#define Ket_StrideRounds    6

#define Ketje_BlockSize (2*SnP_width/8/25)

#define FRAMEBITSEMPTY  0x01
#define FRAMEBITS0      0x02
#define FRAMEBITS00     0x04
#define FRAMEBITS10     0x05
#define FRAMEBITS01     0x06
#define FRAMEBITS11     0x07

#define OUTPUT

enum Phase {
    Ketje_Phase_Virgin          = 0,
    Ketje_Phase_FeedingAssociatedData   = 1,
    Ketje_Phase_Wrapping        = 2,
    Ketje_Phase_Unwrapping      = 4
};

typedef struct {
    /** The state processed by the permutation. */
    unsigned char state[KeccakF_stateSizeInBytes];

    /** The phase. */
    unsigned int phase;

    /** The amount of associated or plaintext data that has been
      * XORed into the state after the last call to step or stride */
    unsigned int dataRemainderSize;
} Ketje_Instance;

void assert_2(unsigned int prep, unsigned char * synopsis){ 
    //do nothing
}

void KeccakP200_InitializeRoundConstants()
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

void KeccakP200_InitializeRhoOffsets()
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

static void displayByteString(FILE *f, const char* synopsis, const unsigned char *data, unsigned int length)
{
    unsigned int i;

    fprintf(f, "%s:", synopsis);
    for(i=0; i<length; i++)
        fprintf(f, " %02x", (unsigned int)data[i]);
    fprintf(f, "\n");
}

void KeccakP200_Initialize(void *state)
{
    memset(state, 0, nrLanes * sizeof(tKeccakLane));
}

void KeccakP200_StaticInitialize()
{
    KeccakP200_InitializeRoundConstants();
    KeccakP200_InitializeRhoOffsets();
}

void KeccakP200_OverwriteBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    assert(offset < 25);
    assert(offset+length <= 25);
    memcpy((unsigned char*)state+offset, data, length);
}

void Ket_StateOverwrite( void *state, unsigned int offset, const unsigned char *data, unsigned int length )
{
    KeccakP200_OverwriteBytes(state, data, offset, length);
}

void KeccakP200_AddByte(void *state, unsigned char byte, unsigned int offset)
{
    assert(offset < 25);
    ((unsigned char *)state)[offset] ^= byte;
}

static void fromWordsToBytes(unsigned char *state, const tKeccakLane *stateAsWords)
{
    unsigned int i, j;

    for(i=0; i<nrLanes; i++)
        for(j=0; j<sizeof(tKeccakLane); j++)
            state[i*sizeof(tKeccakLane)+j] = (stateAsWords[i] >> (8*j)) & 0xFF;
}

static void fromBytesToWords(tKeccakLane *stateAsWords, const unsigned char *state)
{
    unsigned int i, j;

    for(i=0; i<nrLanes; i++) {
        stateAsWords[i] = 0;
        for(j=0; j<sizeof(tKeccakLane); j++)
            stateAsWords[i] |= (tKeccakLane)(state[i*sizeof(tKeccakLane)+j]) << (8*j);
    }
}


#define ROL8(a, offset) ((offset != 0) ? ((((tKeccakLane)a) << offset) ^ (((tKeccakLane)a) >> (sizeof(tKeccakLane)*8-offset))) : a)

void theta(tKeccakLane *A)
{
    unsigned int x, y;
    tKeccakLane C[5], D[5];

    for(x=0; x<5; x++) {
        C[x] = 0;
        for(y=0; y<5; y++) {
            C[x] ^= A[index(x, y)];
        }
    }
    for(x=0; x<5; x++){
                D[x] = ROL8(C[(x+1)%5], 1) ^ C[(x+4)%5];
            }
            for(x=0; x<5; x++)
                for(y=0; y<5; y++){
                    A[index(x, y)] ^= D[x];
                }
}

void rho(tKeccakLane *A)
{
    unsigned int x, y;

    for(x=0; x<5; x++) for(y=0; y<5; y++)
        A[index(x, y)] = ROL8(A[index(x, y)], KeccakRhoOffsets[index(x, y)]);
}

void pi(tKeccakLane *A)
{
    unsigned int x, y;
    tKeccakLane tempA[25];

    for(x=0; x<5; x++) for(y=0; y<5; y++)
        tempA[index(x, y)] = A[index(x, y)];
    for(x=0; x<5; x++) for(y=0; y<5; y++)
        A[index(0*x+1*y, 2*x+3*y)] = tempA[index(x, y)];
}

void chi(tKeccakLane *A)
{
    unsigned int x, y;
    tKeccakLane C[5];

    for(y=0; y<5; y++) {
        for(x=0; x<5; x++)
            C[x] = A[index(x, y)] ^ ((~A[index(x+1, y)]) & A[index(x+2, y)]);
        for(x=0; x<5; x++)
            A[index(x, y)] = C[x];
    }
}

void iota(tKeccakLane *A, unsigned int indexRound)
{
    A[index(0, 0)] ^= KeccakRoundConstants[indexRound];
}

void KeccakP200Round(tKeccakLane *state, unsigned int indexRound)
{
    theta(state);
    rho(state);
    pi(state);
    chi(state);
    iota(state, indexRound);
}

void KeccakP200OnWords(tKeccakLane *state, unsigned int nrRounds)
{
    unsigned int i;
    for(i=(maxNrRounds-nrRounds); i<maxNrRounds; i++){
       KeccakP200Round(state, i);
    }
}

void KeccakP200_Permute_Nrounds(void *state, unsigned int nrounds)
{
#if (__BYTE_ORDER != __LITTLE_ENDIAN)
    tKeccakLane stateAsWords[nrLanes];
#endif

#if (__BYTE_ORDER == __LITTLE_ENDIAN)
    KeccakP200OnWords((tKeccakLane*)state, nrounds);
#else
    fromBytesToWords(stateAsWords, (const unsigned char *)state);
    KeccakP200OnWords(stateAsWords, nrounds);
    fromWordsToBytes((unsigned char *)state, stateAsWords);
#endif
}

int Ketje_Initialize(Ketje_Instance *instance, const unsigned char *key, unsigned int keySizeInBits, const unsigned char *nonce, unsigned int nonceSizeInBits)
{
    unsigned char smallData[1];
    unsigned int keyPackSizeInBits;

    keyPackSizeInBits = 8*((keySizeInBits+16)/8);
    if ( (keyPackSizeInBits + nonceSizeInBits + 2) > SnP_width){
        
        return 1;
    }

    instance->phase = Ketje_Phase_FeedingAssociatedData;

    instance->dataRemainderSize = 0;
    KeccakP200_StaticInitialize();
    KeccakP200_Initialize(instance->state);

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

    KeccakP200_AddByte(instance->state, 0x80, SnP_width / 8 - 1 );
    KeccakP200_Permute_Nrounds(instance->state, Ket_StartRounds);

    return 0;
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

unsigned int myMin(unsigned int a, unsigned int b)
{
    return (a < b) ? a : b;
}

void Ket_Step( void *state, unsigned int size, unsigned char frameAndPaddingBits)
{

    KeccakP200_AddByte(state, frameAndPaddingBits, size);
    KeccakP200_AddByte(state, 0x08, Ketje_BlockSize );
    KeccakP200_Permute_Nrounds(state, Ket_StepRounds );
}

void Ket_FeedAssociatedDataBlocks( void *state, const unsigned char *data, unsigned int nBlocks )
{
    do
    {
        KeccakP200_AddByte( state, *(data++), 0 );
        KeccakP200_AddByte( state, *(data++), 1 );        
        Ket_Step( state, Ketje_BlockSize, FRAMEBITS00 );
        
    }
    while ( --nBlocks != 0 );
}

int Ketje_FeedAssociatedData(Ketje_Instance *instance, const unsigned char *data, unsigned int dataSizeInBytes)
{
    
    unsigned int size;
    if ((instance->phase & Ketje_Phase_FeedingAssociatedData) == 0){
        
        return 1;
    }

    if ( (instance->dataRemainderSize + dataSizeInBytes) > Ketje_BlockSize )
    {
        if (instance->dataRemainderSize != 0)
        {
            dataSizeInBytes -= Ketje_BlockSize - instance->dataRemainderSize;
            while ( instance->dataRemainderSize != Ketje_BlockSize ) {
                KeccakP200_AddByte( instance->state, *(data++), instance->dataRemainderSize++ );
            }
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
        KeccakP200_AddByte( instance->state, *(data++), instance->dataRemainderSize++ );
    return 0;
}

void KeccakP200_AddBytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
{
    unsigned int i;

    assert(offset < 25);
    assert(offset+length <= 25);
    
    for(i=0; i<length; i++)
        ((unsigned char *)state)[offset+i] ^= data[i];
}


void KeccakP200_ExtractBytes(const void *state, unsigned char *data, unsigned int offset, unsigned int length)
{
    assert(offset < 25);
    assert(offset+length <= 25);
    memcpy(data, (unsigned char*)state+offset, length);
}

unsigned int Ket_WrapBlocks( void *state, const unsigned char *plaintext, unsigned char *ciphertext, unsigned int nBlocks )
{
    unsigned int ciphertext_size_on_blocks = 0;
    unsigned char keystream[Ketje_BlockSize];
    unsigned char plaintemp[Ketje_BlockSize];
    unsigned char frameAndPaddingBits[1];
    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;


    while ( nBlocks-- != 0 )
    {
        KeccakP200_ExtractBytes(state, keystream, 0, Ketje_BlockSize);
        plaintemp[0] = plaintext[0];
        plaintemp[1] = plaintext[1];
        *(ciphertext++) = *(plaintext++) ^ keystream[0];
        *(ciphertext++) = *(plaintext++) ^ keystream[1];
        ciphertext_size_on_blocks += 2;
        KeccakP200_AddBytes(state, plaintemp, 0, Ketje_BlockSize);
        KeccakP200_AddBytes(state, frameAndPaddingBits, Ketje_BlockSize, 1);
        KeccakP200_Permute_Nrounds(state, Ket_StepRounds);
    }
    return ciphertext_size_on_blocks;
}

unsigned char Ket_StateExtractByte( void *state, unsigned int offset )
{
    unsigned char data[1];

    KeccakP200_ExtractBytes(state, data, offset, 1);
    return data[0];
}


unsigned int Ketje_WrapPlaintext(Ketje_Instance *instance, const unsigned char *plaintext, unsigned char *ciphertext, unsigned int dataSizeInBytes )
{
    unsigned int size;
    unsigned char temp;
    unsigned int ciphertext_size = 0;

    if ( (instance->phase & Ketje_Phase_FeedingAssociatedData) != 0)
    {
        Ket_Step( instance->state, instance->dataRemainderSize, FRAMEBITS01 );
        instance->dataRemainderSize = 0;
        instance->phase = Ketje_Phase_Wrapping;
    }
    if ( (instance->phase & Ketje_Phase_Wrapping) == 0){
        
        return 1;
    }

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
                ciphertext_size++;
                KeccakP200_AddByte( instance->state, temp, instance->dataRemainderSize++ );
                --dataSizeInBytes;
            }
            Ket_Step( instance->state, Ketje_BlockSize, FRAMEBITS11 );
            instance->dataRemainderSize = 0;
        }

        //  Wrap multiple blocks except last.
        if ( dataSizeInBytes > Ketje_BlockSize )
        {
            size = ((dataSizeInBytes + (Ketje_BlockSize - 1)) & ~(Ketje_BlockSize - 1)) - Ketje_BlockSize;
            ciphertext_size += Ket_WrapBlocks( instance->state, plaintext, ciphertext, size / Ketje_BlockSize );
            dataSizeInBytes -= size;
            plaintext += size;
            ciphertext += size;
        }
     }

    //  Add remaining data
    while ( dataSizeInBytes-- != 0 )
    {
        temp = *(plaintext++);
        *(ciphertext++) = temp ^ Ket_StateExtractByte( instance->state, instance->dataRemainderSize );
        ciphertext_size++;
        KeccakP200_AddByte( instance->state, temp, instance->dataRemainderSize++ );
    }

    return ciphertext_size;
}

void Ket_UnwrapBlocks( void *state, const unsigned char *ciphertext, unsigned char *plaintext, unsigned int nBlocks )
{
    unsigned char tempBlock[Ketje_BlockSize];
    unsigned char frameAndPaddingBits[1];
    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;
    while ( nBlocks-- != 0 )
    {
        KeccakP200_ExtractBytes(state, tempBlock, 0, Ketje_BlockSize);
        tempBlock[0] = *(plaintext++) = *(ciphertext++) ^ tempBlock[0];
        tempBlock[1] = *(plaintext++) = *(ciphertext++) ^ tempBlock[1];
        KeccakP200_AddBytes(state, tempBlock, 0, Ketje_BlockSize);
        KeccakP200_AddBytes(state, frameAndPaddingBits, Ketje_BlockSize, 1);
        KeccakP200_Permute_Nrounds(state, Ket_StepRounds);
    }
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
    if ( (instance->phase & Ketje_Phase_Unwrapping) == 0) {
        
        return 1;
        }
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
                KeccakP200_AddByte( instance->state, temp, instance->dataRemainderSize++ );
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
        KeccakP200_AddByte( instance->state, temp, instance->dataRemainderSize++ );
    }
    return 0;
}


int Ketje_GetTag(Ketje_Instance *instance, unsigned char *tag, unsigned int tagSizeInBytes)
{
    unsigned int tagSizePart;
    unsigned int i;

    if ((instance->phase & (Ketje_Phase_Wrapping | Ketje_Phase_Unwrapping)) == 0)
        return 1;

    KeccakP200_AddByte(instance->state, FRAMEBITS10, instance->dataRemainderSize);
    KeccakP200_AddByte(instance->state, 0x08, Ketje_BlockSize);    //padding
    KeccakP200_Permute_Nrounds(instance->state, Ket_StrideRounds );
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

void print_state(tKeccakLane* state){
    int i =0;
    for (i = 0; i < nrLanes; i++){
        printf("%x ", state[i]);
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

void dynamic_test(){

    //176 for ketjeJr
    int keySizeInBits = 0;  int keyMaxSizeInBits = 176;
    //int keyMaxSizeInBits = SnP_width - 18;
     #ifdef OUTPUT
        FILE *f = fopen("dynamic_test_ref_Jr.txt", "w");
    #endif

    for( keySizeInBits=keyMaxSizeInBits; keySizeInBits >=96; keySizeInBits -= (keySizeInBits > 200) ? 96 : ((keySizeInBits > 128) ? 24 : 16)){
        int nonceMaxSizeInBits = keyMaxSizeInBits - keySizeInBits;
        int nonceSizeInBits;
        for(nonceSizeInBits = nonceMaxSizeInBits; nonceSizeInBits >= ((keySizeInBits < 112) ? 0 : nonceMaxSizeInBits); nonceSizeInBits -= (nonceSizeInBits > 128) ? 160 : 64){
            
            Ketje_Instance ketje1; memset(&ketje1, 0, sizeof(Ketje_Instance));
            Ketje_Instance ketje2; memset(&ketje2, 0, sizeof(Ketje_Instance));
            unsigned char key[50], nonce[50]; memset(key, 0, 50*sizeof(unsigned char)); memset(nonce, 0, 50*sizeof(unsigned char));
            unsigned int ADlen; unsigned int keySize1; unsigned int nonceSize1;
            keySize1 = keySizeInBits / 8; nonceSize1 = nonceSizeInBits / 8;

            generateSimpleRawMaterial(key, keySize1, 0x12+nonceSizeInBits, SnP_width);
            generateSimpleRawMaterial(nonce, nonceSize1, 0x23+keySizeInBits, SnP_width);

            Ketje_Initialize(&ketje1, key, keySizeInBits, nonce, nonceSizeInBits);
            Ketje_Initialize(&ketje2, key, keySizeInBits, nonce, nonceSizeInBits);

#ifdef OUTPUT
            fprintf(f, "***\n");
            fprintf(f, "initialize with key of %u bits, nonce of %u bits:\n", keySizeInBits, nonceSizeInBits);
            displayByteString(f, "key", key, keySizeInBits/8);
            displayByteString(f, "nonce", nonce, nonceSizeInBits/8);
            fprintf(f, "\n");
#endif            

            unsigned int Nlen;
            for( ADlen=12; ADlen<20; ADlen++){
                for( Nlen=24-ADlen; Nlen> 0 ; Nlen--){
                    unsigned char associatedData[400], plaintext[400], ciphertext[400];
                    unsigned char plaintextPrime[400], tag1[16], tag2[16];

                    generateSimpleRawMaterial(associatedData, ADlen, 0x34+ Nlen, 3);
                    generateSimpleRawMaterial(plaintext, Nlen, 0x45+ ADlen, 4);

                    Ketje_FeedAssociatedData(&ketje1, associatedData, ADlen);                   
                    Ketje_FeedAssociatedData(&ketje2, associatedData, ADlen);

                    unsigned int ciphertext_size = Ketje_WrapPlaintext(&ketje1, plaintext, ciphertext, Nlen);
                    Ketje_UnwrapCiphertext(&ketje2, ciphertext, plaintextPrime, ciphertext_size);
                    
                    Ketje_GetTag(&ketje1, tag1, 16);
                    Ketje_GetTag(&ketje2, tag2, 16);

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
    printf("Log wrote to dynamic_test_ref_Jr.txt\n");
#endif
}



int main(void){
    dynamic_test();
    return 0; 
}


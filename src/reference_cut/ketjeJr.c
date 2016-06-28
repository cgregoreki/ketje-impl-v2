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

void Ket_WrapBlocks( void *state, const unsigned char *plaintext, unsigned char *ciphertext, unsigned int nBlocks )
{
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
        KeccakP200_AddBytes(state, plaintemp, 0, Ketje_BlockSize);
        KeccakP200_AddBytes(state, frameAndPaddingBits, Ketje_BlockSize, 1);
        KeccakP200_Permute_Nrounds(state, Ket_StepRounds);
    }
}

unsigned char Ket_StateExtractByte( void *state, unsigned int offset )
{
    unsigned char data[1];

    KeccakP200_ExtractBytes(state, data, offset, 1);
    return data[0];
}


int Ketje_WrapPlaintext(Ketje_Instance *instance, const unsigned char *plaintext, unsigned char *ciphertext, unsigned int dataSizeInBytes )
{
    unsigned int size;
    unsigned char temp;

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
            Ket_WrapBlocks( instance->state, plaintext, ciphertext, size / Ketje_BlockSize );
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
        KeccakP200_AddByte( instance->state, temp, instance->dataRemainderSize++ );
    }

    return 0;
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



void test_ketje( const char *file, const unsigned char *expected )
{
	int keySizeInBits;
    int keyMaxSizeInBits = SnP_width - 18;

    Ketje_Instance checksum;
    unsigned char overallChecksum[16];

    Ketje_Initialize(&checksum, 0, 0, 0, 0);

    #ifdef OUTPUT
        FILE *f = fopen(file, "w");
    #endif

    for( keySizeInBits=keyMaxSizeInBits; keySizeInBits >=96; keySizeInBits -= (keySizeInBits > 200) ? 100 : ((keySizeInBits > 128) ? 27 : 16))
    {
        int nonceMaxSizeInBits = keyMaxSizeInBits - keySizeInBits;
        int nonceSizeInBits;
        for(nonceSizeInBits = nonceMaxSizeInBits; nonceSizeInBits >= ((keySizeInBits < 112) ? 0 : nonceMaxSizeInBits); nonceSizeInBits -= (nonceSizeInBits > 128) ? 161 : 65)
        {
            Ketje_Instance ketje1;
            Ketje_Instance ketje2;
            unsigned char key[50];
            unsigned char nonce[50];
            unsigned int ADlen;

#ifdef OUTPUT
            #define kname(nnn) #nnn
            printf( "%s, key length is %u bits, nonce length is %u bits\n", kname(prefix), keySizeInBits, nonceSizeInBits );
            #undef kname
#endif

            generateSimpleRawMaterial(key, 50, 0x12+nonceSizeInBits, SnP_width);
            generateSimpleRawMaterial(nonce, 50, 0x23+keySizeInBits, SnP_width);

            assert_2( Ketje_Initialize( &ketje1, key, keySizeInBits, nonce, nonceSizeInBits) == 0, "Ketje_Initialize 1 did not return zero" );
            assert_2( Ketje_Initialize( &ketje2, key, keySizeInBits, nonce, nonceSizeInBits) == 0, "Ketje_Initialize 2 did not return zero" );

            if ( (keySizeInBits % 8) != 0)
            {
                printf("keysize is not mod 8\n");
                key[keySizeInBits / 8] &= (1 << (keySizeInBits % 8)) - 1;

            }
            if ( (nonceSizeInBits % 8) != 0)
            {
                printf("nonce size is not mod 8\n");
                nonce[nonceSizeInBits / 8] &= (1 << (nonceSizeInBits % 8)) - 1;
            }

#ifdef OUTPUT
            fprintf(f, "***\n");
            fprintf(f, "initialize with key of %u bits, nonce of %u bits:\n", keySizeInBits, nonceSizeInBits );
            displayByteString(f, "key", key, (keySizeInBits+7)/8);
            displayByteString(f, "nonce", nonce, (nonceSizeInBits+7)/8);
            fprintf(f, "\n");
#endif

            for( ADlen=0; ADlen<=400; ADlen=ADlen+ADlen/3+1+(keySizeInBits-96)+nonceSizeInBits/32)
            {
                unsigned int Mlen;
                for( Mlen=0; Mlen<=400; Mlen=Mlen+Mlen/2+1+ADlen+((ADlen == 0) ? (keySizeInBits-96) : (nonceSizeInBits/4+keySizeInBits/2)))
                {
                    unsigned char associatedData[400];
                    unsigned char plaintext[400];
                    unsigned char ciphertext[400];
                    unsigned char plaintextPrime[400];
                    unsigned char tag1[16], tag2[16];

                    generateSimpleRawMaterial(associatedData, ADlen, 0x34+Mlen, 3);
                    generateSimpleRawMaterial(plaintext, Mlen, 0x45+ADlen, 4);

                    printf("Mlen: %d\tADlen: %d\n", Mlen, ADlen);

                    {
                        unsigned int split = myMin(ADlen/4, (unsigned int)200);
                        unsigned int i;
                        printf("split: %d\n", split);

                        for(i=0; i<split; i++)
                            assert_2( Ketje_FeedAssociatedData( &ketje1, associatedData+i, 1) == 0, "Ketje_FeedAssociatedData 1a did not return zero" );
                        if (split < ADlen)
                            assert_2( Ketje_FeedAssociatedData( &ketje1, associatedData+split, ADlen-split) == 0, "Ketje_FeedAssociatedData 1b did not return zero" );
                    }

                    assert_2( Ketje_FeedAssociatedData( &ketje2, associatedData, ADlen) == 0, "Ketje_FeedAssociatedData 2 did not return zero" );

                    {
                        unsigned int split = Mlen/3;
                        memcpy(ciphertext, plaintext, split);
                        assert_2( Ketje_WrapPlaintext( &ketje1, ciphertext, ciphertext, split) == 0, "Ketje_WrapPlaintext 1a did not return zero" ); // in place
                        assert_2( Ketje_WrapPlaintext( &ketje1, plaintext+split, ciphertext+split, Mlen-split) == 0, "Ketje_WrapPlaintext 1b did not return zero" );
                    }

                    {
                        unsigned int split = Mlen/3*2;
                        memcpy(plaintextPrime, ciphertext, split);
                        assert_2( Ketje_UnwrapCiphertext(&ketje2, plaintextPrime, plaintextPrime, split) == 0, "Ketje_UnwrapCiphertext 2a did not return zero" ); // in place
                        assert_2( Ketje_UnwrapCiphertext(&ketje2, ciphertext+split, plaintextPrime+split, Mlen-split) == 0, "Ketje_UnwrapCiphertext 2b did not return zero" );
                    }
#ifdef OUTPUT
                    if (memcmp(plaintext, plaintextPrime, Mlen) != 0) { // !!!
                        printf("keySizeInBits: %d\n", keySizeInBits);
                        printf("nonceSizeInBits: %d\n", nonceSizeInBits);
                        printf("ADlen: %d\n", ADlen);
                        printf("Mlen: %d\n", Mlen);
                        displayByteString(stdout, "plaintext     ", plaintext, Mlen);
                        displayByteString(stdout, "plaintextPrime", plaintextPrime, Mlen);
                    }
#endif

                    assert_2( !memcmp(plaintext, plaintextPrime, Mlen), "Unwrapped plaintext does not match" );
                    assert_2( Ketje_GetTag( &ketje1, tag1, 16) == 0, "Ketje_GetTag 1 did not return zero" );
                    assert_2( Ketje_GetTag( &ketje2, tag2, 16) == 0, "Ketje_GetTag 2 did not return zero" );

#ifdef OUTPUT
                    if (memcmp(tag1, tag2, 16) != 0) { // !!!
                        printf("keySizeInBits: %d\n", keySizeInBits);
                        printf("nonceSizeInBits: %d\n", nonceSizeInBits);
                        printf("ADlen: %d\n", ADlen);
                        printf("Mlen: %d\n", Mlen);
                        displayByteString(stdout, "tag1", tag1, 16);
                        displayByteString(stdout, "tag2", tag2, 16);
                    }
#endif

                    assert_2( !memcmp(tag1, tag2, 16), "Tags do not match" );

                    Ketje_FeedAssociatedData(&checksum, ciphertext, Mlen);
                    Ketje_FeedAssociatedData(&checksum, tag1, 16);

#ifdef OUTPUT
                    displayByteString(f, "associated data", associatedData, ADlen);
                    displayByteString(f, "plaintext", plaintext, Mlen);
                    displayByteString(f, "ciphertext", ciphertext, Mlen);
                    displayByteString(f, "tag", tag1, 16);
                    fprintf(f, "\n");
#endif
                }
            }
        }
    }
    Ketje_WrapPlaintext(&checksum, 0, 0, 0);
    Ketje_GetTag(&checksum, overallChecksum, 16);

#ifdef OUTPUT
    displayByteString(f, "overall checksum", overallChecksum, 16);
    fclose(f);
#endif
    assert_2(memcmp(overallChecksum, expected, 16) == 0, "Wrong checksum");

}

void print_state(tKeccakLane* state){
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

void print_in_hex(tKeccakLane* t){
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
    int i = 13 ; int j = 0;
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

int main(void){

    test_ketje("KetjeJr.txt", "\x3b\x7d\xea\x9d\xf3\xe0\x58\x06\x98\x92\xc3\xc0\x05\x0f\x4b\xfd");
    
    ///meu_teste();
    //teste_particular();
    return 0; 
}


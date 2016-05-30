#include <stdio.h>
#include <string.h>
#define maxKeyBytes 400
#define nstart 12
#define nstep 1
#define nstride 6
#define Ketje_BlockSize 4
#define nrLanes 25
	
#define FRAMEBITS0     	0x02 //0010 
#define FRAMEBITS00     0x04 //0100 
#define FRAMEBITS10     0x05 //0101 
#define FRAMEBITS01     0x06 //0110 
#define FRAMEBITS11     0x07 //0111  



	#define KeccakF_stateSizeInBytes 50
	#define maxNumberOfRounds 20
	#define nrLanes 25
	#define state_width	400

	#define maxNrRounds 20
	unsigned short KeccakRoundConstants[maxNrRounds] = 
	{
		0x0001, 0x8082, 0x808a, 0x8000, 0x808b, 0x0001, 0x8081, 0x8009, 0x008a, 0x0088, 0x8009, 0x000a, 0x808b, 0x008b, 0x8089, 0x8003, 0x8002, 0x0080, 0x800a, 0x000a 
	};

	unsigned int KeccakRhoOffsets[nrLanes] = {
		0, 1, 14, 12, 11, 4, 12, 6, 7, 4, 3, 10, 11, 9, 7, 9, 13, 15, 5, 8, 2, 2, 13, 8, 14	
	};


// 	--- definition of the Ketje Instance --- //
	typedef struct {
	    unsigned char state[50];
	    unsigned int dataRemainderSize;
	} Instance;

// ----- Parte de funcoes auxiliares ----- //


// 	--- auxiliar functions --- //
	#define index(x, y) (((x)%5)+5*((y)%5))
	#define ROL16(a, offset) ((offset != 0) ? ((((unsigned short)a) << offset) ^ (((unsigned short)a) >> (sizeof(unsigned short)*8-offset))) : a)

	void initialize_mem_state(void *state)
	{
	    memset(state, 0, nrLanes * sizeof(unsigned char) * 2);
	}

	void print_state(unsigned char* state){
	    int i =0;
	    for (i = 0; i < nrLanes; i++){
	        printf("%x ", state[i]);
	    }    
	    printf("\n");
	}

	// addBytes não é overwrite. É soma através de um XOR.
	void add_Bytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length)
	{
	    unsigned int i;	    
	    for(i=0; i<length; i++)
	        ((unsigned char *)state)[offset+i] ^= data[i];
	}

	void add_Byte(void *state, unsigned char byte, unsigned int offset)
	{
	    ((unsigned char *)state)[offset] ^= byte;
	}


void theta(unsigned short *A)
{
    unsigned int x, y;
    unsigned short C[5], D[5];

    for(x=0; x<5; x++) {
        C[x] = 0;
        for(y=0; y<5; y++)
            C[x] ^= A[index(x, y)];
    }
    for(x=0; x<5; x++)
        D[x] = ROL16(C[(x+1)%5], 1) ^ C[(x+4)%5];
    for(x=0; x<5; x++)
        for(y=0; y<5; y++)
            A[index(x, y)] ^= D[x];
}

	 void rho(unsigned short *A)
{
    unsigned int x, y;

    for(x=0; x<5; x++) for(y=0; y<5; y++)
        A[index(x, y)] = ROL16(A[index(x, y)], KeccakRhoOffsets[index(x, y)]);
}
	void pi(unsigned short *A)
{
    unsigned int x, y;
    unsigned short tempA[25];

    for(x=0; x<5; x++) for(y=0; y<5; y++)
        tempA[index(x, y)] = A[index(x, y)];
    for(x=0; x<5; x++) for(y=0; y<5; y++)
        A[index(0*x+1*y, 2*x+3*y)] = tempA[index(x, y)];
}

	void chi(unsigned short *A)
{
    unsigned int x, y;
    unsigned short C[5];

    for(y=0; y<5; y++) {
        for(x=0; x<5; x++)
            C[x] = A[index(x, y)] ^ ((~A[index(x+1, y)]) & A[index(x+2, y)]);
        for(x=0; x<5; x++)
            A[index(x, y)] = C[x];
    }
}

	void iota(unsigned short *A, unsigned int indexRound)
{
    A[index(0, 0)] ^= KeccakRoundConstants[indexRound];
}


	void Round400(unsigned short* state, unsigned int rnd){
		theta(state);
		rho(state);
		pi(state);
		chi(state);
		iota(state, rnd);
	}

	void keccakP400NRounds(void* state, int rounds){
		int i = 0;
		for(i=(maxNrRounds-rounds); i<maxNrRounds; i++){
			Round400(state, i);
		}
	}

	// ----- 		FIM 			----- //

// --- Parte de funçoes auxiliares ----//


	unsigned char extract_byte(void *state, unsigned int offset){
		unsigned char data[1];
		memcpy(data, (unsigned char*)state+offset, 1);
		return data[0];
	}

	void extract_bytes(void* state, unsigned char * data, unsigned int offset, unsigned int length){
		memcpy(data, (unsigned char*)state+offset, length);
	}

	void write_data_to_pointer_on_offset(void *state, unsigned char *data, unsigned int offset, unsigned int length)
	{
	    memcpy((unsigned char*)state+offset, data, length);
	}


// ----- Parte de Ketje ----- //

	
	void init_keypack(unsigned char * key_p, unsigned char * key){
		// inicializa o vetor de keypack com zeros.
		memset(key_p, 0, maxKeyBytes+2);

		unsigned char pad = 0x01;
		unsigned char keypack_sizeInBytes = ((strlen(key)) + 2);

		//copia o tamanho de keypack para sua primeira posicao
		key_p[0] = keypack_sizeInBytes;

		// copia a chave para key_pack;
		write_data_to_pointer_on_offset(key_p, key, 1, (strlen(key)));

		/* 	admitimos que as chaves sao sempre multiplas de 8
			bits, portanto, admite apenas um byte de paddding.*/
		write_data_to_pointer_on_offset(key_p, &pad, 1 + (strlen(key)), 1);
	}

	void Ketje_step(void *state, int block_size, unsigned char padding){
		// coloca o padding
		add_Byte(state, padding, block_size);
		// coloca o segundo padding
		add_Byte(state, 0x08, Ketje_BlockSize);

		// executa o round do keccak n_Step vezes.
		int i = 0;
		for(i=(maxNrRounds-nstep); i<maxNrRounds; i++){
			Round400(state, i);
		}
	}

	void Ketje_stride(void *state, int size, unsigned char padding){
		add_Byte(state, padding, size);
	    add_Byte(state, 0x08, Ketje_BlockSize);    //padding
	    keccakP400NRounds(state, nstride );
	}

	void ketje_monkeyduplex_start(Instance* ketje_inst, unsigned char * key, unsigned char * nonce){

		unsigned int count = 0;
		unsigned char pad = 0x01;
		unsigned char pad_final = 0x80;

		unsigned int keySize = strlen(key);
		unsigned int nonceSize = strlen(nonce);

		unsigned char key_pack[maxKeyBytes + 2];

		ketje_inst->dataRemainderSize = 0;

		initialize_mem_state(ketje_inst->state);
		init_keypack(key_pack, key);

		// escrevemos keypack no estado
		write_data_to_pointer_on_offset(ketje_inst->state, key_pack,0,strlen(key_pack));

		// escrevemos o nonce no estado
		write_data_to_pointer_on_offset(ketje_inst->state, nonce, strlen(key_pack), strlen(nonce));
		// colocamos o padding de nonce no final de nonce
		write_data_to_pointer_on_offset(ketje_inst->state, &pad, strlen(key_pack) + strlen(nonce), 1);

		//agora colocamos o padding final do estado.
		write_data_to_pointer_on_offset(ketje_inst->state, &pad_final, state_width/8 -1, 1);

		//agora chama a funcao esponja (keccakP400) nstart vezes.
		for(count = (maxNrRounds - nstart); count < maxNrRounds; count++)
			Round400(ketje_inst->state, count);		
	}

	void put_headers(Instance * instance, unsigned char *A){
		unsigned int i = 0; unsigned int dataSizeInBytes_A = strlen(A);
		int nblocks_A = (((dataSizeInBytes_A + (Ketje_BlockSize - 1)) & ~(Ketje_BlockSize - 1)) - Ketje_BlockSize)/Ketje_BlockSize;
		unsigned char temp[Ketje_BlockSize];
		for (i = 0; i < nblocks_A;i++){
			temp[0] = *(A++); temp[1] = *(A++); temp[2] = *(A++); temp[3] = *(A++);
			add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
			Ketje_step(instance->state, Ketje_BlockSize, FRAMEBITS00);
		}
		int rem = dataSizeInBytes_A - nblocks_A*Ketje_BlockSize;
		while(rem-- > 0){
			add_Byte( instance->state, *(A++), instance->dataRemainderSize++ );
		}
	}

	void wrap3(Instance * instance, unsigned char * A, unsigned char * B, unsigned char *C){
		unsigned int i; unsigned int rem; unsigned char temp[Ketje_BlockSize];

		unsigned char frameAndPaddingBits[1];
	    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;

	    put_headers(instance, A);

	    printf("after associated data:\n");
	    print_state(instance->state);

		Ketje_step(instance->state, instance->dataRemainderSize, FRAMEBITS01);
		instance->dataRemainderSize = 0;

		int dataSizeInBytes_B = strlen(B);
		int nblocks_B = (((dataSizeInBytes_B + (Ketje_BlockSize - 1)) & ~(Ketje_BlockSize - 1)) - Ketje_BlockSize)/Ketje_BlockSize;
		printf("blocks wrap: %d\n", nblocks_B);
		printf("blocks...\n");
		if (nblocks_B > 0){
			for (i = 0; i < nblocks_B;i++){
				temp[0] = B[0]; temp[1] = B[1]; temp[2] = B[2]; temp[3] = B[3];
				*(C++) = *(B++) ^ extract_byte(instance->state, 0);
				*(C++) = *(B++) ^ extract_byte(instance->state, 1);
				*(C++) = *(B++) ^ extract_byte(instance->state, 2);
				*(C++) = *(B++) ^ extract_byte(instance->state, 3);
				
				add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
				add_Bytes(instance->state, frameAndPaddingBits, Ketje_BlockSize, 1);
				keccakP400NRounds(instance->state, nstep);

				print_state(instance->state);
			}
		}

		rem = dataSizeInBytes_B - nblocks_B*Ketje_BlockSize;
		while(rem-- > 0){
			temp[0] = *(B++);
			*(C++) = temp[0] ^ extract_byte( instance->state, instance->dataRemainderSize);
			add_Byte(instance->state, temp[0], instance->dataRemainderSize++);
		}
	}

	void unwrap3(Instance * instance, unsigned char * A, unsigned char * C, unsigned char * B){
		unsigned int i; unsigned int rem; unsigned char temp[Ketje_BlockSize];
		unsigned char frameAndPaddingBits[1];
	    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;

	    put_headers(instance, A);
	    printf("after associated data unwrap:\n");
	    print_state(instance->state);


		Ketje_step(instance->state, instance->dataRemainderSize, FRAMEBITS01);
		instance->dataRemainderSize = 0; 

		int dataSizeInBytes_C = strlen(C);
		int nblocks_C = (((dataSizeInBytes_C + (Ketje_BlockSize - 1)) & ~(Ketje_BlockSize - 1)) - Ketje_BlockSize)/Ketje_BlockSize;
		printf("blocks unwrap: %d\n", nblocks_C);
		printf("blocks...\n");
		if (nblocks_C > 0){
			for (i = 0; i < nblocks_C;i++){

				extract_bytes(instance->state, temp, 0, Ketje_BlockSize);
				temp[0] = *(B++) = *(C++) ^ temp[0];
				temp[1] = *(B++) = *(C++) ^ temp[1];
				temp[2] = *(B++) = *(C++) ^ temp[2];
				temp[3] = *(B++) = *(C++) ^ temp[3];
				
				add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
				add_Bytes(instance->state, frameAndPaddingBits, Ketje_BlockSize, 1);
				keccakP400NRounds(instance->state, nstep);

				print_state(instance->state);
			}
		}

		rem = dataSizeInBytes_C - nblocks_C*Ketje_BlockSize;
		while(rem-- > 0){
			temp[0] = *(C++) ^ extract_byte( instance->state, instance->dataRemainderSize );
	        *(B++) = temp[0];
	        add_Byte(instance->state, temp[0], instance->dataRemainderSize++ );
		}
	}

	void generate_tag(Instance *instance, unsigned char *T, unsigned int tagSizeInBytes)
	{
	    unsigned int tagSizePart;
	    unsigned int i;

	    Ketje_stride(instance->state, instance->dataRemainderSize, FRAMEBITS10);

	    instance->dataRemainderSize = 0;
	    tagSizePart = Ketje_BlockSize > tagSizeInBytes ? tagSizeInBytes : Ketje_BlockSize;
	    
	    for ( i = 0; i < tagSizePart; ++i )
	        *(T++) = extract_byte( instance->state, i );
	    tagSizeInBytes -= tagSizePart;

	    while(tagSizeInBytes > 0)
	    {
	        Ketje_step( instance->state, 0, FRAMEBITS0 );
	        tagSizePart = Ketje_BlockSize;
	        if ( tagSizeInBytes < Ketje_BlockSize )
	            tagSizePart = tagSizeInBytes;
	        for ( i = 0; i < tagSizePart; ++i )
	            *(T++) = extract_byte( instance->state, i );
	        tagSizeInBytes -= tagSizePart;
	    }
	}

	void print_in_hex(unsigned char* t){
	    int i =0;

	    for (i = 0; i < strlen(t); i++){
	        printf("%x ", t[i]);
	    }    
	    printf("\n");
	}

int main (){ 


	unsigned char key[400], nonce[400];
	unsigned char * key_aux = "83nv4eh";
	unsigned char * nonce_aux = "90m546brj";

	memset(key, 0, 400);
	memset(nonce, 0, 400);

	strcpy(key, key_aux);
	strcpy(nonce, nonce_aux);

	int keySizeInBits, nonceSizeInBits;
    int associatedDataSize = 0;
    int keyMaxSizeInBits = 400 - 18;

    unsigned char *associatedData = "Gregoreki";

    Instance instance;
    Instance instance_unwrap;
    
    keySizeInBits = strlen(key)*8;
    nonceSizeInBits = strlen(nonce)*8;

    ketje_monkeyduplex_start(&instance, key, nonce );
    ketje_monkeyduplex_start(&instance_unwrap, key, nonce);

    printf("after initialize: \n");
    print_state(instance.state);
    //Ketje_FeedAssociatedData(&instance, associatedData, strlen(associatedData) );
    //Ketje_FeedAssociatedData(&instance_unwrap, associatedData, strlen(associatedData) );
    //printf("after associated data: \n");
    //print_state(instance.state);

    unsigned char *text = "meu texto";
    
    unsigned char cipher_t[400];
    memset(cipher_t, 0, sizeof(cipher_t));

    wrap3(&instance, associatedData, text, cipher_t);
    printf("after wrap:\n");
    print_state(instance.state);

    unsigned char unwrapped[400];
    memset(unwrapped, 0, 400);
    unwrap3(&instance_unwrap, associatedData, cipher_t, unwrapped);
    printf("after unwrap:\n");
    print_state(instance_unwrap.state);
    printf("cipher_t: \t"); print_in_hex(cipher_t);

    printf("original: " ); print_in_hex(text);
    printf("recuperada: "); print_in_hex(unwrapped);
    printf("cipher_t: "); print_in_hex(cipher_t);
    printf("strlenC: %d\n", strlen(cipher_t));

    unsigned char tag[400]; unsigned char tag2[400]; memset(tag, 0, 400); memset(tag2, 0, 400);
    generate_tag(&instance, tag, 16);
    generate_tag(&instance_unwrap, tag2, 16);
    printf("tag1:\t"); print_in_hex(tag);
    printf("tag2:\t"); print_in_hex(tag2);

	return (0);
}
#include <stdio.h>
#include <string.h>

typedef unsigned char lane;

#define KeccakF_stateSizeInBytes 25

#define maxKeyBytes 400
#define state_width	200


#define nrLanes 25
unsigned int KeccakRhoOffsets[nrLanes] = {
	0, 1, 6, 4, 3, 4, 4, 6, 7, 4, 3, 2, 3, 1, 7, 1, 5, 7, 5, 0, 2, 2, 5, 0, 6 
};

#define maxNrRounds 18
lane KeccakRoundConstants[maxNrRounds] = 
{
	0x01, 0x82, 0x8a, 0x00, 0x8b, 0x01, 0x81, 0x09, 0x8a, 0x88, 0x09, 0x0a, 0x8b, 0x8b, 0x89, 0x03, 0x02, 0x80
};

// ----- Parte de funcoes auxiliares ----- //

	#define index(x, y) (((x)%5)+5*((y)%5))

	void printf_rho_offsets(){
		int i = 0;
		printf("Round constants:\n");
		for (i = 0; i < nrLanes; i++){
			printf("%d ", KeccakRhoOffsets[i]);
		}
		printf("\n");

	}

	void printf_round_constants(){
		int i = 0;
		printf("Round constants:\n");
		for (i = 0; i < maxNrRounds; i++){
			printf("%x ", KeccakRoundConstants[i]);
		}
		printf("\n");

	}	

	void initialize_mem_state(void *state)
	{
	    memset(state, 0, nrLanes * sizeof(lane));
	}

	#define ROL8(a, offset) ((offset != 0) ? ((((lane)a) << offset) ^ (((lane)a) >> (sizeof(lane)*8-offset))) : a)


	void print_state(lane* state){
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

	// ----- 		FIM 			----- //

// ----- Parte das funcoes de keccakP200 -----//

	void theta(lane *A)
	{
	    unsigned int x, y;
	    lane C[5], D[5];

	    for(x=0; x<5; x++) {
	        C[x] = A[x] ^ A[5 + x] ^ A[10 + x] ^ A[15 + x] ^ A[20 + x];
	    }
	    for(x=0; x<5; x++)
	        D[x] = ROL8(C[(x+1)%5], 1) ^ C[(x+4)%5];

	    for(x=0; x<5; x++)
	        for(y=0; y<5; y++)
	            A[index(x, y)] ^= D[x];
	}

	void rho(lane *A)
	{
	    unsigned int x, y;

	    for(x=0; x<5; x++) for(y=0; y<5; y++)
	        A[index(x, y)] = ROL8(A[index(x, y)], KeccakRhoOffsets[index(x, y)]);
	}

	void pi(lane *A)
	{
	    unsigned int x, y;
	    lane tempA[25];

	    for(x=0; x<5; x++) for(y=0; y<5; y++)
	        tempA[index(x, y)] = A[index(x, y)];
	    for(x=0; x<5; x++) for(y=0; y<5; y++)
	        A[index(0*x+1*y, 2*x+3*y)] = tempA[index(x, y)];
	}

	void chi(lane *A)
	{
	    unsigned int x, y;
	    lane C[5];

	    for(y=0; y<5; y++) {
	        for(x=0; x<5; x++)
	            C[x] = A[index(x, y)] ^ ((~A[index(x+1, y)]) & A[index(x+2, y)]);
	        for(x=0; x<5; x++)
	            A[index(x, y)] = C[x];
	    }
	}

	void iota(lane *A, unsigned int indexRound)
	{
	    A[index(0, 0)] ^= KeccakRoundConstants[indexRound];
	}

	void Round200(lane* state, unsigned int rnd){
		theta(state);
		rho(state);
		pi(state);
		chi(state);
		iota(state, rnd);
	}

	// ----- FIM ----- //


// --- Parte de funçoes auxiliares ----//


	unsigned char extract_byte(void *state, unsigned int offset){
		unsigned char data[1];
		memcpy(data, (unsigned char*)state+offset, 1);
		return data[0];
	}

	void extract_bytes(void* state, unsigned char * data, unsigned int offset, unsigned int length){
		memcpy(data, (unsigned char*)state+offset, length);
	}

	void keccakP200NRounds(void* state, int rounds){
		int i = 0;
		for(i=(maxNrRounds-rounds); i<maxNrRounds; i++){
			Round200(state, i);
		}
	}


// ----- Parte de Ketje ----- //

	#define nstart 12
	#define nstep 1
	#define nstride 6
	#define Ketje_BlockSize (2)


	#define FRAMEBITS00     0x04 //0100 
	#define FRAMEBITS10     0x05 //0101 
	#define FRAMEBITS01     0x06 //0110 
	#define FRAMEBITS11     0x07 //0111  


	enum Phase {
	    Ketje_Phase_Virgin          = 0,
	    Ketje_Phase_FeedingAssociatedData   = 1,
	    Ketje_Phase_Wrapping        = 2,
	    Ketje_Phase_Unwrapping      = 4
	};

	typedef struct {
	    lane state[KeccakF_stateSizeInBytes];
	    unsigned int phase;
	    unsigned int dataRemainderSize;
	} Instance;


	void write_data_to_pointer_on_offset(void *state, unsigned char *data, unsigned int offset, unsigned int length)
	{
	    memcpy((unsigned char*)state+offset, data, length);
	}

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
			Round200(state, i);
		}
	}

	/* 	para KetjeJr, é necessário obter um tamanho correto para adicionar
	  	bytes de associatedData por blocos. Essa função é diferente para KetjeSr. */
	int return_ketjeJrSize(int len){
		if (len % 2 == 0){
			return len - 2;
		} else {
			return len - 1;
		}
	}

	void put_associatedDataToStateByBlocks(void *state, const unsigned char *data, unsigned int number_ofBlocks){
		do 
		{
			// coloca o primeiro byte
			add_Byte(state, *(data++), 0);
			// coloca o segundo byte
			add_Byte(state, *(data++), 1);

			printf("state before step:\n");
	        print_state(state);
			Ketje_step(state, Ketje_BlockSize, FRAMEBITS00);
			printf("state after step:\n");
	        print_state(state);

		} while (--number_ofBlocks != 0);
	}

	// função que adiciona o associatedData à matriz de estados. 
	void put_associatedDataToState(Instance *instance, const unsigned char *data){
		int dataSizeInBytes;
		dataSizeInBytes = strlen(data);

		/* 	se o total de bytes é maior que o tamanho do bloco de Ketje, 
		 	então adiciona os blocos um por um, fazendo step a cada bloco. */
		if (dataSizeInBytes > Ketje_BlockSize){
			printf("dataSizeInBytes > Ketje_BlockSize\t \n");
			unsigned int size = return_ketjeJrSize(dataSizeInBytes);
			printf("size: %d\t size/Ketje_BlockSize: %d\n", size, size/Ketje_BlockSize);
			printf("state before FEED\n");
			print_state(instance->state);
			put_associatedDataToStateByBlocks(instance->state, data, size/Ketje_BlockSize);
			dataSizeInBytes -= size; 
			data += size;
		}

		/* 	aquilo que não foi adicionado por blocos ao estado, é adicionado agora.
		 	como não faz ketje_step, remaindersize é incrementado a cada byte que
		 	sobra. */
		while ( dataSizeInBytes-- != 0 )
	        add_Byte( instance->state, *(data++), instance->dataRemainderSize++ );
	}

	void ketje_monkeyduplex_start(Instance* ketje_inst, unsigned char * key, unsigned char * nonce){

		unsigned int count = 0;
		unsigned char pad = 0x01;
		unsigned char pad_final = 0x80;

		unsigned int keySize = strlen(key);
		unsigned int nonceSize = strlen(nonce);

		unsigned char key_pack[maxKeyBytes + 2];

		ketje_inst->phase = Ketje_Phase_FeedingAssociatedData;
		ketje_inst->dataRemainderSize = 0;

		initialize_mem_state(ketje_inst->state);
		init_keypack(key_pack, key);

		// escrevemos keypack no estado
		write_data_to_pointer_on_offset(ketje_inst->state, key_pack,0,strlen(key_pack));
		printf("after keypack:\n");
		print_state(ketje_inst->state);

		// escrevemos o nonce no estado
		write_data_to_pointer_on_offset(ketje_inst->state, nonce, strlen(key_pack), strlen(nonce));
		// colocamos o padding de nonce no final de nonce
		write_data_to_pointer_on_offset(ketje_inst->state, &pad, strlen(key_pack) + strlen(nonce), 1);

		//agora colocamos o padding final do estado.
		write_data_to_pointer_on_offset(ketje_inst->state, &pad_final, state_width/8 -1, 1);

		printf("after nonce and last padding:\n");
    	print_state(ketje_inst->state);
		//agora chama a funcao esponja (keccakP200) nstart vezes.
		for(count = (maxNrRounds - nstart); count < maxNrRounds; count++)
			Round200(ketje_inst->state, count);		
	}

	void wrap3(Instance * instance, unsigned char * A, unsigned char * B, unsigned char *C){

		unsigned char frameAndPaddingBits[1];
	    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;
		//put associated data.
		//inicio de monkeyWrap.wrap 
		unsigned int i = 0; unsigned int dataSizeInBytes_A = strlen(A);
		int nblocks_A = return_ketjeJrSize(dataSizeInBytes_A)/Ketje_BlockSize;
		unsigned char temp[Ketje_BlockSize];
		// (linha 6 e 7) begin
		for (i = 0; i < nblocks_A;i++){
			temp[0] = *(A++); 	temp[1] = *(A++);
			add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
			Ketje_step(instance->state, Ketje_BlockSize, FRAMEBITS00);
		}
		int rem = dataSizeInBytes_A - nblocks_A*Ketje_BlockSize;
		while(rem-- > 0){
			add_Byte( instance->state, *(A++), instance->dataRemainderSize++ );
		}
		printf("after associatedData\n");
		print_state(instance->state);
		// end associated data.
		//end of (linha 6 e 7)
		//begin (linha 8)
		Ketje_step(instance->state, instance->dataRemainderSize, FRAMEBITS01);
		instance->dataRemainderSize = 0; // ??
		//end (linha 8)
		//begin (linha 9)

		int nblocks_B = return_ketjeJrSize(strlen(B))/Ketje_BlockSize;
		int dataSizeInBytes_B = strlen(B);
		for (i = 1; i <= nblocks_B;i++){
			temp[0] = B[0]; temp[1] = B[1];
			*(C++) = *(B++) ^ extract_byte(instance->state, 0);
			*(C++) = *(B++) ^ extract_byte(instance->state, 1);
			
			add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
			add_Bytes(instance->state, frameAndPaddingBits, Ketje_BlockSize, 1);
			keccakP200NRounds(instance->state, nstep);
		}

		rem = dataSizeInBytes_B - nblocks_B*Ketje_BlockSize;
		while(rem-- > 0){
			temp[0] = *(B++);
			*(C++) = temp[0] ^ extract_byte( instance->state, instance->dataRemainderSize);
			add_Byte(instance->state, temp[0], instance->dataRemainderSize++);
		}

		// inicio da parte para gerar a TAG
	}

	unwrap3(Instance * instance, unsigned char * A, unsigned char * C, unsigned char *T, unsigned char * B){

		unsigned char frameAndPaddingBits[1];
	    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;

		unsigned int i = 0; unsigned int dataSizeInBytes_A = strlen(A); 
		unsigned int nblocks_A = return_ketjeJrSize(dataSizeInBytes_A)/Ketje_BlockSize;

		unsigned char temp[Ketje_BlockSize];

		if (nblocks_A > 0) {
			for (i = 0; i < nblocks_A;i++){
				temp[0] = *(A++); 	temp[1] = *(A++);
				add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
				Ketje_step(instance->state, Ketje_BlockSize, FRAMEBITS00);
			}
		}
		
		unsigned int rem = dataSizeInBytes_A - nblocks_A*Ketje_BlockSize;
		while (rem-- > 0) {
			add_Byte( instance->state, *(A++), instance->dataRemainderSize++ );
		}

		Ketje_step(instance->state, instance->dataRemainderSize, FRAMEBITS01);
		instance->dataRemainderSize = 0; 

		int nblocks_C = return_ketjeJrSize(strlen(C))/Ketje_BlockSize;
		int dataSizeInBytes_C = strlen(C);
		printf("before blocks: \n");
	    print_state(instance->state);
	    printf("nblocks_C: %d\n", nblocks_C);
		for (i = 0; i < nblocks_C;i++){

			extract_bytes(instance->state, temp, 0, Ketje_BlockSize);
			temp[0] = *(B++) = *(C++) ^ temp[0];
			temp[1] = *(B++) = *(C++) ^ temp[1];
			
			add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
			add_Bytes(instance->state, frameAndPaddingBits, Ketje_BlockSize, 1);
			keccakP200NRounds(instance->state, nstep);
		}

		rem = dataSizeInBytes_C - nblocks_C*Ketje_BlockSize;
		while(rem-- > 0){
			temp[0] = *(C++) ^ extract_byte( instance->state, instance->dataRemainderSize );
	        *(B++) = temp[0];
	        add_Byte(instance->state, temp[0], instance->dataRemainderSize++ );
		}
	}

int main (){ 

	return (0);
}
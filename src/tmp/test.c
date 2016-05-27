/*
	General information: 

	this code is intended to run
	with words and keys such that 
		sizeof(key) % 8 == 0;
		and
		sizeof(word) % 8 == 0;
*/


#include <stdio.h>
#include <string.h>	

#define maxKeyBytes 400
#define state_width	200
#define Ketje_BlockSize 2
// ----- Parte de KeccakP200 ----- //

	typedef unsigned char UINT8;
	typedef UINT8 lane;

	#define KeccakF_stateSizeInBytes 25
	#define maxNumberOfRounds 18


	#define nrLanes 25
	unsigned int KeccakRhoOffsets[nrLanes] = {
		0, 1, 6, 4, 3, 4, 4, 6, 7, 4, 3, 2, 3, 1, 7, 1, 5, 7, 5, 0, 2, 2, 5, 0, 6 
	};

	#define maxNrRounds 18
	lane KeccakRoundConstants[maxNrRounds] = 
	{
		0x01, 0x82, 0x8a, 0x00, 0x8b, 0x01, 0x81, 0x09, 0x8a, 0x88, 0x09, 0x0a, 0x8b, 0x8b, 0x89, 0x03, 0x02, 0x80
	};

	#define index(x, y) (((x)%5)+5*((y)%5))

	// ----- Parte de funcoes auxiliares ----- //

		void initialize_mem_state(void *state)
		{
		    memset(state, 0, nrLanes * sizeof(lane));
		}

		#define ROL8(a, offset) ((offset != 0) ? ((((lane)a) << offset) ^ (((lane)a) >> (sizeof(lane)*8-offset))) : a)

	// ----- 		FIM 			----- //

	// ----- Parte das funcoes de keccakP200 -----//

		void theta(lane *A)
		{
		    unsigned int x, y;
		    lane C[5], D[5];

		    for(x=0; x<5; x++) {
		        C[x] = 0;
		        for(y=0; y<5; y++){
		            C[x] ^= A[index(x, y)];
		    	}
		    }
		    for(x=0; x<5; x++){
		        D[x] = ROL8(C[(x+1)%5], 1) ^ C[(x+4)%5];
		    }
		    for(x=0; x<5; x++){
		        for(y=0; y<5; y++){
		            A[index(x, y)] ^= D[x];
		        }
		    }
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

	// ----- Fim da Parte de KeccakP200 ----- //


// ----- Parte importante para testes de debug -----//
	void print_state(lane* state){
	    int i =0;
	    for (i = 0; i < nrLanes; i++){
	        printf("%x ", state[i]);
	    }    
	    printf("\n");
	}

	void print_in_hex(lane* t){
	    int i =0;

	    for (i = 0; i < strlen(t); i++){
	        printf("%x ", t[i]);
	    }    
	    printf("\n");
	}

	void test_instance_initialize(lane* state){
	    int i = 0;
	    lane aux[6] = {
	        'C', 'a', 'r', 'l', 'o', 's'
	    };

	    for (i = 0; i < 6; i++){
	            state[i] = aux[i];
	    }
	}

	// ----- Fim da Parte importante para debug -----//


// ----- Parte de Ketje ----- //

	#define nstart 12
	#define nstep 1
	#define nstride 6

	enum Phase {
	    Ketje_Phase_Virgin          = 0,
	    Ketje_Phase_FeedingAssociatedData   = 1,
	    Ketje_Phase_Wrapping        = 2,
	    Ketje_Phase_Unwrapping      = 4
	};

	typedef struct {
	    unsigned char state[KeccakF_stateSizeInBytes];
	    unsigned int phase;
	    unsigned int dataRemainderSize;
	} Instance;

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


	void write_data_to_pointer_on_offset(void *state, unsigned char *data, unsigned int offset, unsigned int length)
	{
	    memcpy((unsigned char*)state+offset, data, length);
	}

	void init_keypack(unsigned char * key_p, unsigned char * key){
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

		// escrevemos o nonce no estado
		write_data_to_pointer_on_offset(ketje_inst->state, nonce, strlen(key_pack), strlen(nonce));
		// colocamos o padding de nonce no final de nonce
		write_data_to_pointer_on_offset(ketje_inst->state, &pad, strlen(key_pack) + strlen(nonce), 1);

		//agora colocamos o padding final do estado.
		write_data_to_pointer_on_offset(ketje_inst->state, &pad_final, state_width/8 -1, 1);

		//agora chama a funcao esponja (keccakP200) nstart vezes.
		for(count = (maxNrRounds - nstart); count < maxNrRounds; count++){
			Round200(ketje_inst->state, count);	
		}
	}	


	// ----- Fim da Parte do Ketje

int return_ketjeJrSize(int len){
	int result = 0;
	if (len % 2 == 0){
		result = len - 2;
	} else {
		result = len - 1;
	}
	if (result >= 0)
		return result;
	else return 0;
}

char * string_teste(int i, char * teste){
	int j = 0;
	for (j = 0; j < i; j++){
		strcat(teste, "a");
	}
	return teste;
}

#define FRAMEBITS00     0x04 //0100 
#define FRAMEBITS10     0x05 //0101 
#define FRAMEBITS01     0x06 //0110 
#define FRAMEBITS11     0x07 //0111  
#define n_Step 1
#define n_Stride 6

void Ketje_step(void *state, int size, unsigned char padding){
	// coloca o padding
	add_Byte(state, padding, size); 
	// coloca o segundo padding
	add_Byte(state, 0x08, Ketje_BlockSize);

	// executa o round do keccak n_Step vezes.
	int i = 0;
	for(i=(maxNrRounds-n_Step); i<maxNrRounds; i++){
		Round200(state, i);
	}
}

void Ketje_stride(void *state, int size, unsigned char padding){
	// coloca o padding
	add_Byte(state, padding, size); 
	// coloca o segundo padding
	add_Byte(state, 0x08, Ketje_BlockSize);

	// executa o round do keccak n_Step vezes.
	int i = 0;
	for(i=(maxNrRounds- n_Stride); i<maxNrRounds; i++){
		Round200(state, i);
	}
}

void put_associatedDataToStateByBlocks(void *state, const unsigned char *data, unsigned int number_ofBlocks){
	do 
	{
		// coloca o primeiro byte
		add_Byte(state, *(data++), 0);
		// coloca o segundo byte
		add_Byte(state, *(data++), 1);

		Ketje_step(state, Ketje_BlockSize, FRAMEBITS00);

	} while (--number_ofBlocks != 0);
}

unsigned char extract_byte(void *state, unsigned int offset){
	unsigned char data[1];
	memcpy(data, (unsigned char*)state+offset, 1);
	return data[0];
}

void extract_bytes(void* state, unsigned char * data, unsigned int offset, unsigned int length){
	memcpy(data, (unsigned char*)state+offset, length);
}

void put_associatedDataToState(Instance *instance, const unsigned char *data){
	/* 
		esta função faz parte de wrap na especificação. Parte onde:
			for i = 0 to ||A|| -2 do
				D.step(A||00, 0)
	*/
	int dataSizeInBytes;
	dataSizeInBytes = strlen(data);

	if (dataSizeInBytes > Ketje_BlockSize){
		unsigned int size = return_ketjeJrSize(dataSizeInBytes);
		put_associatedDataToStateByBlocks(instance->state, data, size/Ketje_BlockSize);
		dataSizeInBytes -= size; 
		data += size;
	}

	while ( dataSizeInBytes-- != 0 )
        add_Byte( instance->state, *(data++), instance->dataRemainderSize++ );
}

void wrap_blocks(void * state, unsigned char * text, unsigned char * cipher_t, unsigned int number_ofBlocks){
	unsigned char keystream[Ketje_BlockSize];
    unsigned char plaintemp[Ketje_BlockSize];
    unsigned char frameAndPaddingBits[1];
    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11; // 1000 or 0111 = 1111

    while ( number_ofBlocks-- != 0 )
    {
    	extract_bytes(state, keystream, 0, Ketje_BlockSize);
        plaintemp[0] = text[0];
        plaintemp[1] = text[1];

        *(cipher_t++) = *(text++) ^ keystream[0];
        *(cipher_t++) = *(text++) ^ keystream[1];

        add_Bytes(state, plaintemp, 0, Ketje_BlockSize);
        add_Bytes(state, frameAndPaddingBits, Ketje_BlockSize, 1);
        
        int i = 0;
		for(i=(maxNrRounds-n_Step); i<maxNrRounds; i++){
			Round200(state, i);
		}
    }
}

void wrap(Instance * instance, unsigned char * text, unsigned char *cipher_t){

	unsigned int dataSizeInBytes = strlen(text);
	unsigned int size;
	unsigned int temp;

	//if (instance->phase == Ketje_Phase_FeedingAssociatedData){
	/*
		Z = D.step(A[||A||-1]||01, |B0|)
	*/
	Ketje_step(instance->state, instance->dataRemainderSize, FRAMEBITS01);		
	printf("after first step\n");
	print_state(instance->state);
	instance->dataRemainderSize = 0;
	instance->phase = Ketje_Phase_Wrapping;
	//}

	if (instance->dataRemainderSize + dataSizeInBytes > Ketje_BlockSize) {
		if (instance->dataRemainderSize != 0){
			// se é diferente de zero, então é igual a 1. faz uma vez:
			// begin:
			temp = *(text++);
			// C[0] = B[0] XOR Z
			*(cipher_t++) = temp ^ extract_byte(instance->state, instance->dataRemainderSize);
			add_Byte(instance->state, temp, instance->dataRemainderSize++);
			--dataSizeInBytes;
			//end
			Ketje_step(instance->state, Ketje_BlockSize, FRAMEBITS11);
			instance->dataRemainderSize = 0;
		}

		if (dataSizeInBytes > Ketje_BlockSize) {
			size = return_ketjeJrSize(dataSizeInBytes);
			wrap_blocks(instance->state, text, cipher_t, size/Ketje_BlockSize);
			dataSizeInBytes-= size;
			text += size;
			cipher_t += size;
		}

	}
	while (dataSizeInBytes-- != 0){
		printf("dataSizeInBytes: %d\n", dataSizeInBytes);
		temp = *(text++);
		*(cipher_t++) = temp ^ extract_byte(instance->state, instance->dataRemainderSize);
		add_Byte(instance->state, temp, instance->dataRemainderSize++);
	}

}
	
void wrap2(Instance * instance, unsigned char * text, unsigned char *cipher_t){
	unsigned int dataSizeInBytes = strlen(text);

		//linha 8, wrap, especificacao
		Ketje_step(instance->state, instance->dataRemainderSize, FRAMEBITS01);
		instance->dataRemainderSize = 0;

		// linha 9, wrap, especificacao
		cipher_t[0] = text[0] ^ extract_byte(instance->state, 0);
		
		if (dataSizeInBytes > Ketje_BlockSize){
			// se eh tem mais que um bloco de bytes, entao cifra por blocos.
			
			unsigned int size = return_ketjeJrSize(dataSizeInBytes)/Ketje_BlockSize;
			unsigned char frameAndPaddingBits[1];
		    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11; // 0001 1110 = 1111

		    unsigned char keystream[Ketje_BlockSize];
		    unsigned char plaintemp[Ketje_BlockSize];

		    // linha 10, wrap, especificacao
		    unsigned int i = 0;
			for (i = 0; i < size; i++){
				
				extract_bytes(instance->state, keystream, 0, Ketje_BlockSize);

				plaintemp[0] = text[0];
		        plaintemp[1] = text[1];
		        // como cifra em blocos, e um bloco é de tamanho 2...
		        *(cipher_t++) = *(text++) ^ keystream[0];
		        *(cipher_t++) = *(text++) ^ keystream[1];

		        // parte de ket_step dentro de wrap. linha 11 da especificacao
		        //começo
		        add_Bytes(instance->state, plaintemp, 0, Ketje_BlockSize);
		        //coloca padding
		        add_Bytes(instance->state, frameAndPaddingBits, Ketje_BlockSize, 1);

		        int j = 0;
				for(j=(maxNrRounds-n_Step); j<maxNrRounds; j++){
					Round200(instance->state, j);
				}		
				//fim
			}
			dataSizeInBytes -= size*Ketje_BlockSize;
		}
	    unsigned int temp;
	    printf("dataSizeInBytes: %d\n", dataSizeInBytes);
	    while ( dataSizeInBytes-- != 0 )
	    {
	        temp = *(text++);
	        *(cipher_t++) = temp ^ extract_byte( instance->state, instance->dataRemainderSize );
	        add_Byte( instance->state, temp, instance->dataRemainderSize++);
	    }
	    // a esta altura, ciphertext esteh em cipher_t
}

void keccakP200NRounds(void* state, int rounds){
	int i = 0;
	for(i=(maxNrRounds-rounds); i<maxNrRounds; i++){
		Round200(state, i);
	}
}

void unwrap(Instance * instance, unsigned char * text, unsigned char *cipher_t){
	unsigned int dataSizeInBytes = strlen(text);
	unsigned char frameAndPaddingBits[1];
	frameAndPaddingBits[0] = 0x08 | FRAMEBITS11; // 0001 1110 = 1111


	//linha 8, wrap, especificacao
	Ketje_step(instance->state, instance->dataRemainderSize, FRAMEBITS01);
	instance->dataRemainderSize = 0;

	// linha 9, wrap, especificacao
	cipher_t[0] = text[0] ^ extract_byte(instance->state, 0);
	
	if (dataSizeInBytes > Ketje_BlockSize){
		// se eh tem mais que um bloco de bytes, entao cifra por blocos.
		
		unsigned int size = return_ketjeJrSize(dataSizeInBytes)/Ketje_BlockSize;
		
	    unsigned char keystream[Ketje_BlockSize];
	    unsigned char plaintemp[Ketje_BlockSize];

	    // linha 10, wrap, especificacao
	    unsigned int i = 0;
		for (i = 0; i < size; i++){
			
			extract_bytes(instance->state, keystream, 0, Ketje_BlockSize);

			plaintemp[0] = text[0];
	        plaintemp[1] = text[1];
	        // como cifra em blocos, e um bloco é de tamanho 2...
	        *(cipher_t++) = *(text++) ^ keystream[0];
	        *(cipher_t++) = *(text++) ^ keystream[1];

	        // parte de ket_step dentro de wrap. linha 11 da especificacao
	        //começo
	        add_Bytes(instance->state, plaintemp, 0, Ketje_BlockSize);
	        //coloca padding
	        add_Bytes(instance->state, frameAndPaddingBits, Ketje_BlockSize, 1);

	        int j = 0;
			for(j=(maxNrRounds-n_Step); j<maxNrRounds; j++){
				Round200(instance->state, j);
			}		
			//fim
		}
		dataSizeInBytes -= size*Ketje_BlockSize;
	}
    unsigned int temp;
    printf("dataSizeInBytes: %d\n", dataSizeInBytes);
    while ( dataSizeInBytes-- != 0 )
    {
        temp = *(text++);
        printf("temp: %d\n", temp);
        *(cipher_t++) = temp ^ extract_byte( instance->state, instance->dataRemainderSize );
        add_Byte( instance->state, temp, instance->dataRemainderSize++);
    }

    // a esta altura, ciphertext esteh em cipher_t
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
		keccakP200NRounds(instance->state, n_Step);
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
		keccakP200NRounds(instance->state, n_Step);
	}

	rem = dataSizeInBytes_C - nblocks_C*Ketje_BlockSize;
	while(rem-- > 0){
		temp[0] = *(C++) ^ extract_byte( instance->state, instance->dataRemainderSize );
        *(B++) = temp[0];
        add_Byte(instance->state, temp[0], instance->dataRemainderSize++ );
	}
}


int main (){ 

	lane state[KeccakF_stateSizeInBytes];
  
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

	int i = 0;
	char teste[50];
	for (i = 0; i < 30; i++){
	    strcpy(teste, string_teste(i, teste));
	    int dataSizeInBytes = strlen(teste);
	    int size = ((dataSizeInBytes + (Ketje_BlockSize - 1)) & ~(Ketje_BlockSize - 1)) - Ketje_BlockSize;
	    //printf("strlen: %d\tsize: %d\t size/Ketje_BlockSize: %d\n", strlen(teste),size, size/Ketje_BlockSize);
	    strcpy(teste, "");
	    }

	unsigned char text[400];
    unsigned char *aux = "meu texto";
    int j= 0;
    strcpy(text, aux);
    unsigned char cipher_t[400];
    unsigned char tag[400];
    unsigned char unwrapped[400];
    memset(unwrapped, 0, 400);



	wrap3(&ketj, associatedData, text, cipher_t);
	printf("after wrap\n");
	print_state(ketj.state);
	unwrap3(&ketj_unwrapp, associatedData, cipher_t, tag, unwrapped);

	printf("after UNWRAP\n");
	print_state(ketj_unwrapp.state);
	printf("original: " ); print_in_hex(text);
	printf("recuperada: "); print_in_hex(unwrapped);
	printf("cipher_t: "); print_in_hex(cipher_t);
    
}
	//printf("after associated data: \n");

	//print_state(ketj.state);

    //printf("instance.dataRemainderSize: %d\n", ketj.dataRemainderSize);

    //memset(text, 0, sizeof(cipher_t));
	
    //wrap(&ketj, text, cipher_t);
    //wrap2(&ketj, text, cipher_t);
    //printf("after wrap:\n");
    //print_state(ketj.state);

    
#include <stdio.h>
#include <string.h>
#include "tiny_ketjeJr.h"

// ----- Parte de funcoes auxiliares ----- //

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

	unsigned char extract_byte(void *state, unsigned int offset){
		unsigned char data[1];
		memcpy(data, (unsigned char*)state+offset, 1);
		return data[0];
	}

	void extract_bytes(void* state, unsigned char * data, unsigned int offset, unsigned int length){
		memcpy(data, (unsigned char*)state+offset, length);
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


// ----- Parte de Ketje ----- //

	
	unsigned int init_keypack(unsigned char * key_p, unsigned char * key, unsigned int keySizeInBytes){

		// inicializa o vetor de keypack com zeros.
		memset(key_p, 0, maxKeyBytes+2);

		unsigned int result = keySizeInBytes + 2;

		unsigned char pad = 0x01;
		unsigned char keypack_sizeInBytes = (keySizeInBytes + 2);

		//copia o tamanho de keypack para sua primeira posicao
		key_p[0] = keypack_sizeInBytes;

		// copia a chave para key_pack;
		memcpy((unsigned char*)key_p+1, key, keySizeInBytes);

		/* 	admitimos que as chaves sao sempre multiplas de 8
			bits, portanto, admite apenas um byte de paddding.*/
		memcpy((unsigned char*)key_p+(1+keySizeInBytes), &pad, 1);

		return result;
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

	void Ketje_stride(void *state, int size, unsigned char padding){
		add_Byte(state, padding, size);
	    add_Byte(state, 0x08, Ketje_BlockSize);    //padding
	    keccakP200NRounds(state, nstride );
	}

	void ketje_monkeyduplex_start(Instance* ketje_inst, unsigned char * key, unsigned int keySize, unsigned char * nonce, unsigned int nonceSize){

		unsigned int count = 0;
		unsigned char pad = 0x01;
		unsigned char pad_final = 0x80;

		// unsigned int keySize = strlen(key);
		// unsigned int nonceSize = strlen(nonce);

		unsigned char key_pack[maxKeyBytes + 2];

		ketje_inst->dataRemainderSize = 0;

		// inicializamos o estado com zeros.
		memset(ketje_inst->state, 0, nrLanes * sizeof(unsigned char));

		unsigned int key_Pack_size = init_keypack(key_pack, key, keySize);

		// escrevemos keypack no estado
		memcpy((unsigned char*)ketje_inst->state, key_pack, key_Pack_size);

		// escrevemos o nonce no estado
		memcpy((unsigned char*)ketje_inst->state + key_Pack_size, nonce, nonceSize);

		// colocamos o padding de nonce no final de nonce
		memcpy((unsigned char*)ketje_inst->state + key_Pack_size + nonceSize, &pad, 1);

		//agora colocamos o padding final do estado.
		//memcpy((unsigned char*)ketje_inst->state + state_width/8 -1, &pad_final, 1);
		add_Byte(ketje_inst->state, pad_final, state_width/8 -1);

		//agora chama a funcao esponja (keccakP200) nstart vezes.
		for(count = (maxNrRounds - nstart); count < maxNrRounds; count++)
			Round200(ketje_inst->state, count);		
	}

	void put_headers(Instance * instance, unsigned char *A, unsigned int dataSizeInBytes_A){
		unsigned int i = 0;
		int nblocks_A = return_ketjeJrSize(dataSizeInBytes_A)/Ketje_BlockSize;
		unsigned char temp[Ketje_BlockSize];
		for (i = 0; i < nblocks_A;i++){
			temp[0] = *(A++); 	temp[1] = *(A++);
			add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
			Ketje_step(instance->state, Ketje_BlockSize, FRAMEBITS00);
		}
		int rem = dataSizeInBytes_A - nblocks_A*Ketje_BlockSize;
		while(rem-- > 0){
			add_Byte( instance->state, *(A++), instance->dataRemainderSize++ );
		}
	}

	unsigned int wrap3(Instance * instance, unsigned char * A, unsigned int dataSizeInBytes_A, unsigned char * B, unsigned char dataSizeInBytes_B, unsigned char *C){
		unsigned int i; unsigned int rem; unsigned char temp[Ketje_BlockSize];
		unsigned int C_size = 0;
		unsigned char frameAndPaddingBits[1];
	    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;

	    put_headers(instance, A, dataSizeInBytes_A);
		Ketje_step(instance->state, instance->dataRemainderSize, FRAMEBITS01);
		instance->dataRemainderSize = 0;

		int nblocks_B = return_ketjeJrSize(dataSizeInBytes_B)/Ketje_BlockSize;
		
		if (nblocks_B > 0){
			for (i = 1; i <= nblocks_B;i++){
				temp[0] = B[0]; temp[1] = B[1];
				*(C++) = *(B++) ^ extract_byte(instance->state, 0);
				*(C++) = *(B++) ^ extract_byte(instance->state, 1);
				C_size += 2;
				
				add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
				add_Bytes(instance->state, frameAndPaddingBits, Ketje_BlockSize, 1);
				keccakP200NRounds(instance->state, nstep);
			}
		}

		rem = dataSizeInBytes_B - nblocks_B*Ketje_BlockSize;
		while(rem-- > 0){
			temp[0] = *(B++);
			*(C++) = temp[0] ^ extract_byte( instance->state, instance->dataRemainderSize);
			C_size += 1;
			add_Byte(instance->state, temp[0], instance->dataRemainderSize++);
		}
		return C_size;
	}

	void unwrap3(Instance * instance, unsigned char * A, unsigned int dataSizeInBytes_A, unsigned char * C, unsigned int dataSizeInBytes_C, unsigned char * B){
		unsigned int i; unsigned int rem; unsigned char temp[Ketje_BlockSize];
		unsigned char frameAndPaddingBits[1];
	    frameAndPaddingBits[0] = 0x08 | FRAMEBITS11;

	    put_headers(instance, A, dataSizeInBytes_A);

		Ketje_step(instance->state, instance->dataRemainderSize, FRAMEBITS01);
		instance->dataRemainderSize = 0; 
		
		int nblocks_C = return_ketjeJrSize(dataSizeInBytes_C)/Ketje_BlockSize;
		
		if (nblocks_C > 0){
			for (i = 0; i < nblocks_C;i++){

				extract_bytes(instance->state, temp, 0, Ketje_BlockSize);
				temp[0] = *(B++) = *(C++) ^ temp[0];
				temp[1] = *(B++) = *(C++) ^ temp[1];
				
				add_Bytes(instance->state, temp, 0, Ketje_BlockSize);
				add_Bytes(instance->state, frameAndPaddingBits, Ketje_BlockSize, 1);
				keccakP200NRounds(instance->state, nstep);
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
#include <stdio.h>
#include <string.h>

// 	--- definitions --- //
	#define maxKeyBytes 400
	#define nstart 12
	#define nstep 1
	#define nstride 6
	#define Ketje_BlockSize 4
	#define nrLanes 25
	#define maxNrRounds 20

	#define state_width 400

	#define FRAMEBITS0     	0x02 //0010 
	#define FRAMEBITS00     0x04 //0100 
	#define FRAMEBITS10     0x05 //0101 
	#define FRAMEBITS01     0x06 //0110 
	#define FRAMEBITS11     0x07 //0111  

// 	--- definition of the Ketje Instance --- //
	typedef struct {
	    unsigned char state[50];
	    unsigned int dataRemainderSize;
	} Instance;

//	--- auxiliar functions --- //
	void initialize_mem_state(void *state);
	void add_Bytes(void *state, const unsigned char *data, unsigned int offset, unsigned int length);
	void add_Byte(void *state, unsigned char byte, unsigned int offset);
	unsigned char extract_byte(void *state, unsigned int offset);
	void extract_bytes(void* state, unsigned char * data, unsigned int offset, unsigned int length);

//	--- Ketje Functions --- //
	unsigned int init_keypack(unsigned char * key_p, unsigned char * key, unsigned int keySizeInBytes);
	// monkey duplex //
	void Ketje_step(void *state, int block_size, unsigned char padding);
	void Ketje_stride(void *state, int size, unsigned char padding);
	void ketje_monkeyduplex_start(Instance* ketje_inst, unsigned char * key, unsigned int keySize, unsigned char * nonce, unsigned int nonceSize);
	// monkey wrap //
	unsigned int wrap3(Instance * instance, unsigned char * A, unsigned int dataSizeInBytes_A, unsigned char * B, unsigned char dataSizeInBytes_B, unsigned char *C);
	void unwrap3(Instance * instance, unsigned char * A, unsigned int dataSizeInBytes_A, unsigned char * C, unsigned int dataSizeInBytes_C, unsigned char * B);
	void generate_tag(Instance *instance, unsigned char *T, unsigned int tagSizeInBytes);















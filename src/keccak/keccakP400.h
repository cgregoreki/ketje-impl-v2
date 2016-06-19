#include <stdio.h>
#include <string.h>	

//	---	definitions --- //
	#define maxNumberOfRounds 20
	#define nrLanes 25
	#define state_width	400

	#define maxNrRounds 20
	static unsigned short KeccakRoundConstants[maxNrRounds] = 
	{
		0x0001, 0x8082, 0x808a, 0x8000, 0x808b, 0x0001, 0x8081, 0x8009, 0x008a, 0x0088, 0x8009, 0x000a, 0x808b, 0x008b, 0x8089, 0x8003, 0x8002, 0x0080, 0x800a, 0x000a 
	};

	static unsigned int KeccakRhoOffsets[nrLanes] = {
		0, 1, 14, 12, 11, 4, 12, 6, 7, 4, 3, 10, 11, 9, 7, 9, 13, 15, 5, 8, 2, 2, 13, 8, 14	
	};


// 	--- auxiliar functions --- //
	#define index(x, y) (((x)%5)+5*((y)%5))
	#define ROL16(a, offset) ((offset != 0) ? ((((unsigned short)a) << offset) ^ (((unsigned short)a) >> (sizeof(unsigned short)*8-offset))) : a)

// 	--- functions --- //
	void theta(unsigned short *A);
	void rho(unsigned short *A);
	void pi(unsigned short *A);
	void chi(unsigned short *A);
	void iota(unsigned short *A, unsigned int round);
	void Round400(unsigned short *state, unsigned int rnd);
	void keccakP400NRounds(void *state, int rounds);


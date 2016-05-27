#include <stdio.h>
#include <string.h>	

//	---	definitions --- //
	#define KeccakF_stateSizeInBytes 25
	#define maxNumberOfRounds 18
	#define nrLanes 25
	#define state_width	200

	#define maxNrRounds 18
	unsigned char KeccakRoundConstants[maxNrRounds] = 
	{
		0x01, 0x82, 0x8a, 0x00, 0x8b, 0x01, 0x81, 0x09, 0x8a, 0x88, 0x09, 0x0a, 0x8b, 0x8b, 0x89, 0x03, 0x02, 0x80
	};

	#define nrunsigned chars 25
	unsigned int KeccakRhoOffsets[nrunsigned chars] = {
		0, 1, 6, 4, 3, 4, 4, 6, 7, 4, 3, 2, 3, 1, 7, 1, 5, 7, 5, 0, 2, 2, 5, 0, 6 
	};


// 	--- auxiliar functions --- //
	#define index(x, y) (((x)%5)+5*((y)%5))
	#define ROL8(a, offset) ((offset != 0) ? ((((unsigned char)a) << offset) ^ (((unsigned char)a) >> (sizeof(unsigned char)*8-offset))) : a)

// 	--- functions --- //
	void theta(unsigned char *A);
	void rho(unsigned char *A);
	void pi(unsigned char *A);
	void chi(unsigned char *A);
	void iota(unsigned char *A, unsigned int round);
	void Round200(unsigned char *state, unsigned int rnd);
	void keccakP200NRounds(void *state, int rounds);


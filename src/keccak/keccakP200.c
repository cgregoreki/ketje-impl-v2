#include "keccakP200.h"

void theta(unsigned char *A)
	{
	    unsigned int x, y;
	    unsigned char C[5], D[5];

	    for(x=0; x<5; x++) {
	        C[x] = A[x] ^ A[5 + x] ^ A[10 + x] ^ A[15 + x] ^ A[20 + x];
	    }
	    for(x=0; x<5; x++)
	        D[x] = ROL8(C[(x+1)%5], 1) ^ C[(x+4)%5];

	    for(x=0; x<5; x++)
	        for(y=0; y<5; y++)
	            A[index(x, y)] ^= D[x];
	}

	void rho(unsigned char *A)
	{
	    unsigned int x, y;

	    for(x=0; x<5; x++) for(y=0; y<5; y++)
	        A[index(x, y)] = ROL8(A[index(x, y)], KeccakRhoOffsets[index(x, y)]);
	}

	void pi(unsigned char *A)
	{
	    unsigned int x, y;
	    unsigned char tempA[25];

	    for(x=0; x<5; x++) for(y=0; y<5; y++)
	        tempA[index(x, y)] = A[index(x, y)];
	    for(x=0; x<5; x++) for(y=0; y<5; y++)
	        A[index(0*x+1*y, 2*x+3*y)] = tempA[index(x, y)];
	}

	void chi(unsigned char *A)
	{
	    unsigned int x, y;
	    unsigned char C[5];

	    for(y=0; y<5; y++) {
	        for(x=0; x<5; x++)
	            C[x] = A[index(x, y)] ^ ((~A[index(x+1, y)]) & A[index(x+2, y)]);
	        for(x=0; x<5; x++)
	            A[index(x, y)] = C[x];
	    }
	}

	void iota(unsigned char *A, unsigned int indexRound)
	{
	    A[index(0, 0)] ^= KeccakRoundConstants[indexRound];
	}

	void Round200(unsigned char* state, unsigned int rnd){
		theta(state);
		rho(state);
		pi(state);
		chi(state);
		iota(state, rnd);
	}

	void keccakP200NRounds(void* state, int rounds){
		int i = 0;
		for(i=(maxNrRounds-rounds); i<maxNrRounds; i++){
			Round200(state, i);
		}
	}

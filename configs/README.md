Here be files that describe launch options for the executable.
For now it is single line with 3 values
N S B
Where:
* N is positive integer, denoting total count of number to generate
* S is seed for srandom (and later on - srand48)
* B is number of used bytes of unsigned long (from 1 to CHAR_BIT * sizeof(unsigned long))
More options to come, probably - we need to create more working modes for program 

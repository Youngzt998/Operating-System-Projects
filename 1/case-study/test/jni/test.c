#include <stdio.h>
int main()
{
	printf("This is a test:\n\n");
	int i=syscall(356,123,"test string");
	printf("Answer is %d!\n",i);
	printf("TEST End!\n\n");
	return 0;
}
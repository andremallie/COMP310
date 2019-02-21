#include <stdio.h>

int main () {
	char name[20];	

	scanf("%s", name);
	
	fprintf(stdout, "The users name is: %s\n", name);

	fprintf(stderr, "This is an error message\n");

	return 0;
	


}

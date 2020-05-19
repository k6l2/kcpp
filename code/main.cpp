#include <cstdlib>
#include <cstdio>
int main(int argc, char** argv)
{
	if(argc == 1)
	{
		printf("KC++: An extremely lightweight language extension to C++.\n");
		printf("Usage: kc++ [input code tree directory] "
		       "[desired output directory]\n");
		return EXIT_SUCCESS;
	}
	return EXIT_SUCCESS;
}
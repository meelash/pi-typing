#include "kernel.h"
#include <circle/startup.h>

int main(void)
{
	// Objects in CKernel cannot be destroyed, so never return from here.
	static CKernel Kernel;
	if (!Kernel.Initialize()) {
		halt();
		return EXIT_HALT;
	}
	Kernel.Run();
	halt();
	return EXIT_HALT;
}

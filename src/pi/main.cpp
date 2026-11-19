#include "kernel.h"
#include <circle/startup.h>

int main(void)
{
	// Static: CKernel's objects are never destroyed. Returning reboots.
	static CKernel Kernel;
	if (!Kernel.Initialize()) {
		halt();
		return EXIT_HALT;
	}
	Kernel.Run();
	// Back to the boot menu: restart the Pi (the firmware loads the menu again).
	return EXIT_REBOOT;
}

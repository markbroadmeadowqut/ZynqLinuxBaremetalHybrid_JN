/**
 * Partially based on XAPP1078 documentation from Xilinx
 */

#include <stdio.h>
#include "xparameters.h"
#include "xil_mmu.h"
#include "xil_cache.h"
#include "xil_exception.h"

#define COMM_FLAG_LINUX (*(volatile unsigned long *)(0xFFFF8000)) //set 1 if data from Linux sent
#define COMM_DATA_LINUX (*(volatile unsigned long *)(0xFFFF8004)) //Linux data location
#define COMM_FLAG_BAREMETAL (*(volatile unsigned long *)(0xFFFF8008)) //set 1 if data from baremetal sent
#define COMM_DATA_BAREMETAL (*(volatile unsigned long *)(0xFFFF800C)) //Baremetal data location

extern u32 MMUTable;


/**
 * Taken from XAPP1078
 * Modified DCacheFlush to prevent L2 Cache controller access
 */
void MyXil_DCacheFlush(void)
{
	Xil_L1DCacheFlush();
	//Xil_L2CacheFlush();
}

/**
 * Taken from XAPP1078
 * Modified SetTlbAttributes to call MyXil_DCacheFlush in order
 * to prevent L2 Cache controller accesses
 */
void MyXil_SetTlbAttributes(u32 addr, u32 attrib)
{
	u32 *ptr;
	u32 section;

	mtcp(XREG_CP15_INVAL_UTLB_UNLOCKED, 0);
	dsb();

	mtcp(XREG_CP15_INVAL_BRANCH_ARRAY, 0);
	dsb();
	MyXil_DCacheFlush();

	section = addr / 0x100000;
	ptr = &MMUTable + section;
	*ptr = (addr & 0xFFF00000) | attrib;
	dsb();
}


int main() {
	u32 value;
	int n,i,j;
	// Disable L1 cache for OCM
	MyXil_SetTlbAttributes(0xFFFF0000,0x04de2);		// S=b0 TEX=b100 AP=b11, Domain=b1111, C=b0, B=b0

	while(1) {
		if (COMM_FLAG_LINUX) {
			value = COMM_DATA_LINUX;
			break;
		}
	}

	//find next prime number
	n = value;

    for(i=n+1;;i++)
    {
        for(j=2;j<i;j++)
        {
            if(i%j==0)
                break;
        }

        if(j==i)
        {
            value = i;
            break;
        }
    }
    COMM_DATA_BAREMETAL = value;
    COMM_FLAG_BAREMETAL = 1;

	return 0;


}

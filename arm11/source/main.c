#include <types.h>
#include <vram.h>
#include <arm.h>
#include <pxi.h>

#include "arm/gic.h"
#include "hw/gpulcd.h"
#include "hw/i2c.h"
#include "hw/mcu.h"

#define LEGACY_BOOT_ENTRY	((vu32*)0x1FFFFFFC)
#define LEGACY_BOOT_MAGIC	(0xDEADDEAD)

void PXI_RX_Handler(u32 __attribute__((unused)) irqn)
{
	u32 ret, msg, cmd, argc, args[PXI_MAX_ARGS];

	msg = PXI_Recv();
	cmd = msg & 0xFFFF;
	argc = msg >> 16;

	if (argc > PXI_MAX_ARGS) {
		PXI_Send(0xFFFFFFFF);
		return;
	}

	PXI_RecvArray(args, argc);

	switch (cmd) {
		case PXI_LEGACY_MODE:
		{
			*LEGACY_BOOT_ENTRY = LEGACY_BOOT_MAGIC;
			ret = 0;
			break;
		}

		case PXI_GET_SHMEM:
		{
			//ret = (u32)SHMEM_GetGlobalPointer();
			ret = 0xFFFFFFFF;
			break;
		}

		case PXI_SCREENINIT:
		{
			GPU_Init();
			GPU_PSCFill(VRAM_START, VRAM_END, 0);
			GPU_SetFramebuffers((u32[]){VRAM_TOP_LA, VRAM_TOP_LB,
										VRAM_TOP_RA, VRAM_TOP_RB,
										VRAM_BOT_A,  VRAM_BOT_B});

			GPU_SetFramebufferMode(0, PDC_RGB24);
			GPU_SetFramebufferMode(1, PDC_RGB24);
			ret = 0;
			break;
		}

		case PXI_BRIGHTNESS:
		{
			LCD_SetBrightness(args[0]);
			ret = args[0];
			break;
		}

		case PXI_I2C_READ:
		{
			ret = I2C_readRegBuf(args[0], args[1], (u8*)args[2], args[3]);
			break;
		}

		case PXI_I2C_WRITE:
		{
			ret = I2C_writeRegBuf(args[0], args[1], (u8*)args[2], args[3]);
			break;
		}

		/* New CMD template:
		case CMD_ID:
		{
			<var declarations/assignments>
			<execute the command>
			<set the return value>
			break;
		}
		*/

		default:
			ret = 0xFFFFFFFF;
			break;
	}

	PXI_Send(ret);
	return;
}

void MPCoreMain(void)
{
	u32 entry;

	GIC_Enable(IRQ_PXI_RX, BIT(0), GIC_HIGHEST_PRIO, PXI_RX_Handler);
	*LEGACY_BOOT_ENTRY = 0;

	PXI_Reset();
	I2C_init();
	//MCU_init();

	PXI_Barrier(ARM11_READY_BARRIER);
	ARM_EnableInterrupts();

	// Process IRQs until the ARM9 tells us it's time to boot something else
	do {
		ARM_WFI();
	} while(*LEGACY_BOOT_ENTRY != LEGACY_BOOT_MAGIC);

	// Perform any needed deinit stuff
	ARM_DisableInterrupts();
	GIC_GlobalReset();
	GIC_LocalReset();

	do {
		entry = *LEGACY_BOOT_ENTRY;
	} while(entry == LEGACY_BOOT_MAGIC);

	((void (*)())(entry))();
}

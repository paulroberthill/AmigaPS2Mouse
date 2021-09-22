#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <devices/input.h>
#include <devices/inputevent.h>
#include <clib/exec_protos.h>
#include <clib/input_protos.h>
#include <clib/dos_protos.h>
#include <dos/dos.h>
#include <stdio.h>

#define NM_WHEEL_UP 0x7A
#define NM_WHEEL_DOWN 0x7B

#define MM_NOTHING 0
#define MM_WHEEL_DOWN 1
#define MM_WHEEL_UP 2
#define MM_MIDDLEMOUSE_DOWN 3
#define MM_MIDDLEMOUSE_UP 4

extern void VertBServer();  /* proto for asm interrupt server */

struct {
    UWORD potgo;			// 0 0xdff016
    UWORD joy0dat;			// 2
    UBYTE pra;				// 4 0xbfe001
    UBYTE pad;				// 5
    ULONG sigbit;			// 6	
    struct Task *task;		// 10
    APTR potgoResource;		// 14
} mousedata;

struct Interrupt vertblankint = {
    { 0, 0, NT_INTERRUPT, 10-10, "Arduino PS/2 mouse adapter" },
    &mousedata,
	VertBServer
};

void CreateMouseEvents(int t);
int AllocResources();
void FreeResources();
extern struct CIA ciaa, ciab;

struct IOStdReq   *InputIO;
struct MsgPort    *InputMP;
struct InputEvent *MouseEvent;
struct InputBase  *InputBase;
BYTE intsignal;

void main(void)
{
	if (AllocResources())
	{
		mousedata.task = FindTask(0);
		mousedata.sigbit = 1 << intsignal;
		   
		AddIntServer(INTB_VERTB, &vertblankint); 

		SetTaskPri(mousedata.task, 20); /* same as input.device */
		while (1)
		{
			ULONG signals = Wait (mousedata.sigbit | SIGBREAKF_CTRL_C);
			if (signals & SIGBREAKF_CTRL_C)
			{
				PutStr("Exiting\n");
				break;
			}
			if (signals & mousedata.sigbit)
			{
				int code = MM_NOTHING
				if (!(mousedata.potgo & 0x400)) code |= MM_WHEEL_DOWN;
				if (mousedata.potgo & 0x100) code |= MM_WHEEL_UP;
				if (!(mousedata.pra & 0x40)) code |= MM_MIDDLEMOUSE_UP;

				CreateMouseEvents(code);
			}
		}		
	}
	FreeResources();
}

int AllocResources()
{
	if (intsignal = AllocSignal (-1))
	{
		if (InputMP = CreateMsgPort())
		{
			if (MouseEvent = AllocMem(sizeof(struct InputEvent),MEMF_PUBLIC))
			{
				if (InputIO = CreateIORequest(InputMP,sizeof(struct IOStdReq)))
				{
					if (!OpenDevice("input.device",NULL,
								(struct IORequest *)InputIO,NULL))
					{
						InputBase = (struct InputBase *)InputIO->io_Device;
						if (mousedata.potgoResource = OpenResource("potgo.resource"))
						{
							return 1;
						}
						else
							PutStr("Error: Could not open potgo.resource\n");
					}
					else
						PutStr("Error: Could not open input.device\n");
				}
				else
					PutStr("Error: Could not create IORequest\n");
			}
			else
				PutStr("Error: Could not allocate memory for MouseEvent\n");
		}
		else
			PutStr("Error: Could not create message port\n");
	}
	else
		PutStr("Error: Could not allocate signal\n");
	return 0;
}

void FreeResources()
{
	if (InputIO) 
	{
		CloseDevice((struct IORequest *)InputIO);
		DeleteIORequest(InputIO);
	}
	if (MouseEvent) FreeMem(MouseEvent, sizeof(struct InputEvent));
	if (InputMP) DeleteMsgPort(InputMP);
	if (intsignal) FreeSignal(intsignal);
	RemIntServer(INTB_VERTB, &vertblankint);
}

int down=0;
void CreateMouseEvents(int t)
{
	if (t == 0)
	{
		return;
	}
	MouseEvent->ie_EventAddress = NULL;
	MouseEvent->ie_NextEvent = NULL;
	MouseEvent->ie_Class = IECLASS_RAWKEY;
	MouseEvent->ie_SubClass = 0;
	MouseEvent->ie_Qualifier = PeekQualifier();		//???
	switch (t)
	{
		case MM_WHEEL_DOWN:
			MouseEvent->ie_Code = NM_WHEEL_DOWN;
			break;
		case MM_WHEEL_UP:
			MouseEvent->ie_Code = NM_WHEEL_UP;
			break;
		case MM_MIDDLEMOUSE_DOWN:
			MouseEvent->ie_Code = IECODE_MBUTTON;
			MouseEvent->ie_Class = IECLASS_RAWMOUSE;
			MouseEvent->ie_X = 0;
			MouseEvent->ie_Y = 0;
		break;
		case MM_MIDDLEMOUSE_UP:
			MouseEvent->ie_Code = IECODE_LBUTTON|IECODE_UP_PREFIX;			// fixme!!!!!
			MouseEvent->ie_Class = IECLASS_RAWMOUSE;
			MouseEvent->ie_X = 0;
			MouseEvent->ie_Y = 0;
			break;
	}
	
	InputIO->io_Data = (APTR)MouseEvent;
	InputIO->io_Length = sizeof(struct InputEvent);
	InputIO->io_Command = IND_WRITEEVENT;
	// InputIO->io_Flags = IOF_QUICK;

	DoIO((struct IORequest *)InputIO);
}

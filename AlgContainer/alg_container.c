#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned int    VOS_UINT32;
typedef char   			VOS_CHAR;
typedef unsigned char   VOS_UINT8;
typedef void            VOS_VOID;


#define VOS_NULL        0
#define VOS_NULL_BYTE   0xFF
#define VOS_NULL_WORD   0xFFFF
#define VOS_NULL_DWORD  0xFFFFFFFF
#define VOS_NULL_PTR    0
#define VOS_OK          0
#define VOS_ERR         1


#define MAX_NAME_LEN   50
#define VOS_MemAlloc(size)              malloc(size)
#define VOS_MemFree(ptr)                free(ptr)
#define VOS_MemSet(ptr, value, num)     memset(ptr, value, num)
#define VOS_MemCopy(dst, src, num)      memcopy(dst, src, num)

typedef enum ENUM_ITEM_STATUS {
	ENUM_ITEM_IDLE = 0,
	ENUM_ITEM_USED,

	ENUM_ITEM_BUTT
}ENUM_ITEM_STATUS;

typedef struct CONTROL_ITEM_STRU {
	ENUM_ITEM_STATUS enStatus;
    VOS_UINT32  	udwPrev;
    VOS_UINT32  	udwNext;
} CONTROL_ITEM;

typedef struct DATA_ITEM_STRU {
    VOS_UINT32  udwId;
    VOS_CHAR    aucName[MAX_NAME_LEN];
} DATA_ITEM;

typedef struct ALG_CONTAINER_STRU {
    VOS_UINT32      udwHead;
    VOS_UINT32      udwTail;
    VOS_UINT32      udwMaxNum;
    VOS_UINT32      udwUsedNum;
    VOS_UINT32      udwItemSize;
    CONTROL_ITEM    *pstControlItem;
    VOS_VOID        *pstDataBaseAddr;
}ALG_CONTAINER;

#define MAX_ITEM_NUM  10
ALG_CONTAINER  g_stAlgContainer;

VOS_UINT32 InitAlgContainer(ALG_CONTAINER *pstAlgContainer, VOS_UINT32 udwNum, VOS_UINT32 udwDataSize);
VOS_UINT32 AllocAlgContainerItem(ALG_CONTAINER *pstAlgContainer);
VOS_VOID* GetAlgContainerItem(ALG_CONTAINER *pstAlgContainer, VOS_UINT32 udwIndex);
VOS_UINT32 FreeAlgContainerItem(ALG_CONTAINER *pstAlgContainer, VOS_UINT32 udwIndex);
VOS_VOID PrintAlgContainer(ALG_CONTAINER *pstAlgContainer);
int main()
{
    VOS_UINT32 udwRetCode;
    VOS_UINT32 udwCount = 0;
    VOS_UINT32 udwIndex;

    udwRetCode = InitAlgContainer(&g_stAlgContainer, MAX_ITEM_NUM, sizeof(DATA_ITEM));
    if(VOS_OK != udwRetCode)
    {
    	printf("fail to init alg container\r\n");
    	return 0;
    }

    PrintAlgContainer(&g_stAlgContainer);

    for(udwCount = 0; udwCount < MAX_ITEM_NUM; udwCount++)
    {
    	udwIndex = AllocAlgContainerItem(&g_stAlgContainer);
    	printf("new item, index = %d\n", udwIndex);
    	PrintAlgContainer(&g_stAlgContainer);
    }

    FreeAlgContainerItem(&g_stAlgContainer, 5);
    PrintAlgContainer(&g_stAlgContainer);

    FreeAlgContainerItem(&g_stAlgContainer, 1);
    PrintAlgContainer(&g_stAlgContainer);

    FreeAlgContainerItem(&g_stAlgContainer, 9);
    PrintAlgContainer(&g_stAlgContainer);

    return 0;
}

VOS_UINT32 InitAlgContainer(ALG_CONTAINER *pstAlgContainer, VOS_UINT32 udwNum, VOS_UINT32 udwDataSize)
{
    VOS_UINT32      udwIndex = 0;
    VOS_VOID        *pBuffer        = VOS_NULL_PTR;
    CONTROL_ITEM    *pstControlItem = VOS_NULL_PTR;

    pBuffer = VOS_MemAlloc(udwNum * (sizeof(CONTROL_ITEM) + udwDataSize));
    pstControlItem = (CONTROL_ITEM *)pBuffer;

    /* 初始化时构建空闲链 */
    pstControlItem[0].udwPrev 	= VOS_NULL_DWORD;
    pstControlItem[0].udwNext 	= 1;
    pstControlItem[0].enStatus 	= ENUM_ITEM_IDLE;
    for(udwIndex = 1; udwIndex < udwNum - 1; udwIndex++)
    {
    	pstControlItem[udwIndex].enStatus = ENUM_ITEM_IDLE;
        pstControlItem[udwIndex].udwPrev = udwIndex - 1;
        pstControlItem[udwIndex].udwNext = udwIndex + 1;
    }
    pstControlItem[udwIndex].udwPrev 	= udwIndex - 1;
    pstControlItem[udwIndex].udwNext 	= VOS_NULL_DWORD;
    pstControlItem[udwIndex].enStatus 	= ENUM_ITEM_IDLE;

    pstAlgContainer->udwHead           = 0;
    pstAlgContainer->udwTail           = udwNum - 1;
    pstAlgContainer->udwMaxNum         = udwNum;
    pstAlgContainer->udwUsedNum        = 0;
    pstAlgContainer->udwItemSize   	= udwDataSize;
    pstAlgContainer->pstControlItem    = pstControlItem;
    pstAlgContainer->pstDataBaseAddr   = pstControlItem + udwNum;

    return VOS_OK;
}

VOS_UINT32 AllocAlgContainerItem(ALG_CONTAINER *pstAlgContainer)
{
    CONTROL_ITEM    *pstControlItem = VOS_NULL_PTR;
    VOS_UINT32 		udwIndex = VOS_NULL_DWORD;

    if(VOS_NULL_PTR == pstAlgContainer)
    {
        return VOS_ERR;
    }

    if(pstAlgContainer->udwHead >= pstAlgContainer->udwMaxNum)
    {
        return VOS_ERR;
    }

    pstControlItem  = pstAlgContainer->pstControlItem;
    udwIndex = pstAlgContainer->udwHead;

    if(pstControlItem[udwIndex].udwNext < pstAlgContainer->udwMaxNum)
    {
		pstAlgContainer->udwHead = pstControlItem[udwIndex].udwNext;
		pstControlItem[pstAlgContainer->udwHead].udwPrev = VOS_NULL_DWORD;
    }
    else
    {
        pstAlgContainer->udwHead = VOS_NULL_DWORD;
        pstAlgContainer->udwTail = VOS_NULL_DWORD;
    }

    pstControlItem[udwIndex].udwPrev = VOS_NULL_DWORD;
	pstControlItem[udwIndex].udwNext = VOS_NULL_DWORD;
	pstControlItem[udwIndex].enStatus = ENUM_ITEM_USED;
    pstAlgContainer->udwUsedNum++;

    return udwIndex;
}

VOS_VOID* GetAlgContainerItem(ALG_CONTAINER *pstAlgContainer, VOS_UINT32 udwIndex)
{
    if(udwIndex >= pstAlgContainer->udwMaxNum)
    {
        return VOS_NULL_PTR;
    }

    return (VOS_UINT8 *)pstAlgContainer->pstDataBaseAddr + udwIndex * pstAlgContainer->udwItemSize;
}

VOS_UINT32 FreeAlgContainerItem(ALG_CONTAINER *pstAlgContainer, VOS_UINT32 udwIndex)
{
    CONTROL_ITEM    *pstControlItem = VOS_NULL_PTR;

    pstControlItem  = pstAlgContainer->pstControlItem;

    if(VOS_NULL_DWORD == pstAlgContainer->udwTail)
    {
        pstControlItem[udwIndex].udwPrev = VOS_NULL_DWORD;
        pstControlItem[udwIndex].udwNext = VOS_NULL_DWORD;
        pstAlgContainer->udwHead = udwIndex;
        pstAlgContainer->udwTail = udwIndex;
    }
    else
    {
    	pstControlItem[udwIndex].udwPrev = pstAlgContainer->udwTail;
    	pstControlItem[udwIndex].udwNext = VOS_NULL_DWORD;
		pstControlItem[pstAlgContainer->udwTail].udwNext = udwIndex;
		pstAlgContainer->udwTail = udwIndex;
    }

    pstControlItem[udwIndex].enStatus = ENUM_ITEM_IDLE;
    pstAlgContainer->udwUsedNum--;

    return VOS_OK;
}

VOS_VOID PrintAlgContainer(ALG_CONTAINER *pstAlgContainer)
{
	CONTROL_ITEM    	*pstControlItem = VOS_NULL_PTR;
	VOS_UINT32      	udwIndex = VOS_NULL_DWORD;
	ENUM_ITEM_STATUS	enStatus = ENUM_ITEM_BUTT;
	VOS_CHAR        	*apucStatusStr[ENUM_ITEM_BUTT + 1] = {"idle", "used", "null"};

	printf("----------------------------------------------\n");
	printf("udwMaxNum  = %-10u, udwUsedNum = %-10u\n", pstAlgContainer->udwMaxNum, pstAlgContainer->udwUsedNum);
	printf("udwHead    = %-10u, udwTail    = %-10u\n", pstAlgContainer->udwHead, pstAlgContainer->udwTail);

	printf("---begin idle link---\n");
	printf("%-10s | %-10s | %-10s | %s\n", "status", "index", "prev", "next");
	pstControlItem = pstAlgContainer->pstControlItem;
	udwIndex = pstAlgContainer->udwHead;
	while(udwIndex < pstAlgContainer->udwMaxNum)
	{
		enStatus = pstControlItem[udwIndex].enStatus;
		printf("%-10s | %-10u | ", apucStatusStr[enStatus], udwIndex);
		printf("%-10u | %u\n", pstControlItem[udwIndex].udwPrev, pstControlItem[udwIndex].udwNext);
		udwIndex = pstControlItem[udwIndex].udwNext;
	}
	printf("---end idle link---\n");
	printf("---begin used link---\n");
	printf("%-10s | %-10s | %-10s | %s\n", "status", "index", "prev", "next");
	for(udwIndex = 0; udwIndex < pstAlgContainer->udwMaxNum; udwIndex++)
	{
		enStatus = pstControlItem[udwIndex].enStatus;
		if(ENUM_ITEM_IDLE == enStatus)
		{
			continue;
		}
			printf("%-10s | %-10u | ", apucStatusStr[enStatus], udwIndex);
			printf("%-10u | %u\n", pstControlItem[udwIndex].udwPrev, pstControlItem[udwIndex].udwNext);
	}
	printf("---end used link---\n");
	printf("----------------------------------------------\n");

	return;
}

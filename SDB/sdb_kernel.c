#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef unsigned int    VOS_UINT32;
typedef char            VOS_CHAR;
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

typedef struct LINK_ITEM_STRU {
	ENUM_ITEM_STATUS 	enStatus;
    VOS_UINT32      	udwPrev;
    VOS_UINT32      	udwNext;
} LINK_ITEM;

typedef struct CONFLICT_LINK_ITEM_STRU {
	VOS_UINT32		udwHead;
	VOS_UINT32 		udwConflictLen;
    VOS_UINT32      udwPrev;
    VOS_UINT32      udwNext;
} CONFLICT_LINK_ITEM;

typedef struct DATA_ITEM_STRU {
    VOS_UINT32  udwId;
    VOS_CHAR    aucName[MAX_NAME_LEN];
} DATA_ITEM;

typedef struct ALG_CONTAINER_STRU {
    VOS_UINT32      udwIdleHead;
    VOS_UINT32      udwIdleTail;
    VOS_UINT32      udwUsedHead;
    VOS_UINT32      udwUsedTail;
    VOS_UINT32      udwMaxNum;
    VOS_UINT32      udwUsedNum;
    VOS_UINT32      udwItemSize;
    VOS_UINT8       *pucPhysicalMem;
}SDB_CONTAINER;

#define MAX_LINK_NUM  15

#define MAX_ITEM_NUM  10
SDB_CONTAINER  g_stSdbContainer;

VOS_UINT32 InitSdbContainer(SDB_CONTAINER *pstSdbContainer, VOS_UINT32 udwNum, VOS_UINT32 udwDataSize);
VOS_UINT32 SDB_InitIdleLink(SDB_CONTAINER *pstSdbContainer);
VOS_UINT32 SDB_InitConflictLink(SDB_CONTAINER *pstSdbContainer);
VOS_VOID SDB_PrintContainer(SDB_CONTAINER *pstSdbContainer);
VOS_UINT32 SDB_AllocItem(SDB_CONTAINER *pstSdbContainer);
VOS_UINT32 SDB_FreeItem(SDB_CONTAINER *pstSdbContainer, VOS_UINT32 udwIndex);

int main()
{
    VOS_UINT32 udwRetCode;
    VOS_UINT32 udwCount = 0;
    VOS_UINT32 udwIndex;

    udwRetCode = InitSdbContainer(&g_stSdbContainer, MAX_ITEM_NUM, sizeof(DATA_ITEM));
    if(VOS_OK != udwRetCode)
    {
        printf("fail to init alg container\r\n");
        return 0;
    }

    SDB_PrintContainer(&g_stSdbContainer);

    for(udwCount = 0; udwCount < MAX_ITEM_NUM; udwCount++)
    {
        udwIndex = SDB_AllocItem(&g_stSdbContainer);
        printf("new item, index = %d\n", udwIndex);
        SDB_PrintContainer(&g_stSdbContainer);
    }

    SDB_FreeItem(&g_stSdbContainer, 0);
    SDB_PrintContainer(&g_stSdbContainer);

    return 0;
}

/************************************************
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
| idle/used_link     |       conflict_link * 15       | data block |
| status, prev, next | head, conflict_len, prev, next | data       |
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
************************************************/
VOS_UINT32 InitSdbContainer(SDB_CONTAINER *pstSdbContainer, VOS_UINT32 udwMaxNum, VOS_UINT32 udwDataSize)
{
	VOS_UINT32      udwRetCode = VOS_OK;
    VOS_UINT8       *pucPhysicalMem = VOS_NULL_PTR;
    VOS_UINT32      udwLen = 0;

    udwLen = sizeof(LINK_ITEM) + sizeof(CONFLICT_LINK_ITEM) * MAX_LINK_NUM + udwDataSize;

    pucPhysicalMem = (VOS_UINT8 *)VOS_MemAlloc(udwMaxNum * udwLen);
	if(VOS_NULL_PTR == pucPhysicalMem)
	{
		printf("alloc mem fail\n");
		return VOS_ERR;
	}


    pstSdbContainer->udwMaxNum      = udwMaxNum;
    pstSdbContainer->udwUsedNum     = 0;
    pstSdbContainer->udwItemSize    = udwDataSize;
    pstSdbContainer->pucPhysicalMem	= pucPhysicalMem;

    udwRetCode = SDB_InitIdleLink(pstSdbContainer);
    if(VOS_OK != udwRetCode)
    {
    	printf("fail to init idle link");
    	return VOS_ERR;
    }

    udwRetCode = SDB_InitConflictLink(pstSdbContainer);
    if(VOS_OK != udwRetCode)
	{
		printf("fail to init conflict link");
		return VOS_ERR;
	}

    return VOS_OK;
}

VOS_UINT32 SDB_InitIdleLink(SDB_CONTAINER *pstSdbContainer)
{
    VOS_UINT8       *pucPhysicalMem     = VOS_NULL_PTR;
    LINK_ITEM       *pstIdleLink        = VOS_NULL_PTR;
    VOS_UINT32 		udwIndex = 0;
    VOS_UINT32      udwLen = 0;
    VOS_UINT32      udwOffset = 0;

    pucPhysicalMem = pstSdbContainer->pucPhysicalMem;
    udwLen = sizeof(LINK_ITEM) + sizeof(CONFLICT_LINK_ITEM) * MAX_LINK_NUM
    		+ pstSdbContainer->udwItemSize;

    /* 初始化时构建空闲链 */
	udwOffset = 0;
	pstSdbContainer->udwIdleHead = 0;
	pstSdbContainer->udwIdleTail = pstSdbContainer->udwMaxNum - 1;
	pstSdbContainer->udwUsedHead = VOS_NULL_DWORD;
	pstSdbContainer->udwUsedTail = VOS_NULL_DWORD;

	pstIdleLink = (LINK_ITEM *)(pucPhysicalMem + udwOffset);
	pstIdleLink->enStatus = ENUM_ITEM_IDLE;
	pstIdleLink->udwPrev = VOS_NULL_DWORD;
	pstIdleLink->udwNext = 1;
	for(udwIndex = 1; udwIndex < pstSdbContainer->udwMaxNum - 1; udwIndex++)
	{
		pstIdleLink = (LINK_ITEM *)(pucPhysicalMem + udwOffset + udwLen * udwIndex);
		pstIdleLink->enStatus = ENUM_ITEM_IDLE;
		pstIdleLink->udwPrev = udwIndex - 1;
		pstIdleLink->udwNext = udwIndex + 1;
	}
	pstIdleLink = (LINK_ITEM *)(pucPhysicalMem + udwOffset + udwLen * udwIndex);
	pstIdleLink->enStatus = ENUM_ITEM_IDLE;
	pstIdleLink->udwPrev = udwIndex - 1;
	pstIdleLink->udwNext = VOS_NULL_DWORD;

	return VOS_OK;
}

VOS_UINT32 SDB_InitConflictLink(SDB_CONTAINER *pstSdbContainer)
{
    VOS_UINT8       	*pucPhysicalMem     = VOS_NULL_PTR;
    CONFLICT_LINK_ITEM  *pstConflictLink    = VOS_NULL_PTR;
    VOS_UINT32      	udwCount = 0;
    VOS_UINT32 			udwIndex = 0;
    VOS_UINT32      	udwLen = 0;
    VOS_UINT32      	udwOffset = 0;


    pucPhysicalMem = pstSdbContainer->pucPhysicalMem;
    udwLen = sizeof(LINK_ITEM) + sizeof(CONFLICT_LINK_ITEM) * MAX_LINK_NUM
    		+ pstSdbContainer->udwItemSize;

    for(udwCount = 0; udwCount < MAX_LINK_NUM; udwCount++)
    {
        udwOffset = sizeof(LINK_ITEM) + sizeof(CONFLICT_LINK_ITEM) * udwCount;
        for(udwIndex = 0; udwIndex < pstSdbContainer->udwMaxNum; udwIndex++)
        {
            pstConflictLink = (CONFLICT_LINK_ITEM *)(pucPhysicalMem + udwLen * udwIndex + udwOffset);
            pstConflictLink->udwHead 		= VOS_NULL_DWORD;
            pstConflictLink->udwConflictLen = VOS_NULL;
            pstConflictLink->udwPrev 		= VOS_NULL_DWORD;
            pstConflictLink->udwNext 		= VOS_NULL_DWORD;
        }
    }

	return VOS_OK;
}

VOS_UINT32 SdbHashFunc(VOS_UINT8 *pDataMem)
{
	return 1;
}

VOS_VOID SDB_PrintContainer(SDB_CONTAINER *pstSdbContainer)
{
    VOS_UINT8  	*pucPhysicalMem     = VOS_NULL_PTR;
    LINK_ITEM   *pstLinkItem        = VOS_NULL_PTR;
    VOS_UINT32 	udwLen              = VOS_NULL;
    VOS_UINT32	udwIndex			= VOS_NULL;
    VOS_CHAR    *apucStatusStr[ENUM_ITEM_BUTT + 1] = {"idle", "used", "null"};

    pucPhysicalMem = pstSdbContainer->pucPhysicalMem;
    udwLen = sizeof(LINK_ITEM) + sizeof(CONFLICT_LINK_ITEM) * MAX_LINK_NUM
    		+ pstSdbContainer->udwItemSize;

    /* 打印空闲链 */
    printf("idle link, head = %u\n", pstSdbContainer->udwIdleHead);
    udwIndex = pstSdbContainer->udwIdleHead;
    while(udwIndex < pstSdbContainer->udwMaxNum)
    {
    	pstLinkItem = (LINK_ITEM *)(pucPhysicalMem + udwLen * udwIndex);
    	printf("%-10s | %-10u | ", apucStatusStr[pstLinkItem->enStatus], udwIndex);
    	printf("%-10u | %u\n", pstLinkItem->udwPrev, pstLinkItem->udwNext);
    	udwIndex = pstLinkItem->udwNext;
    }
    printf("idle link, tail = %u\n", pstSdbContainer->udwIdleTail);
    fflush(stdout);

    /* 打印占用链 */
    printf("used link, head = %u\n", pstSdbContainer->udwUsedHead);
	udwIndex = pstSdbContainer->udwUsedHead;
	while(udwIndex < pstSdbContainer->udwMaxNum)
	{
		pstLinkItem = (LINK_ITEM *)(pucPhysicalMem + udwLen * udwIndex);
		printf("%-10s | %-10u | ", apucStatusStr[pstLinkItem->enStatus], udwIndex);
		printf("%-10u |%u\n", pstLinkItem->udwPrev, pstLinkItem->udwNext);
		udwIndex = pstLinkItem->udwNext;
	}
	printf("used link, tail = %u\n", pstSdbContainer->udwUsedTail);
	fflush(stdout);

    return;
}

VOS_UINT32 SDB_AllocItem(SDB_CONTAINER *pstSdbContainer)
{
	VOS_UINT8       *pucPhysicalMem     = VOS_NULL_PTR;
	LINK_ITEM       *pstCurrLinkItem    = VOS_NULL_PTR;
	LINK_ITEM       *pstIdleNext    	= VOS_NULL_PTR;
	LINK_ITEM       *pstUsedTail    	= VOS_NULL_PTR;
	VOS_UINT32      udwLen         		= 0;
	VOS_UINT32      udwCurr 			= VOS_NULL_DWORD;
	VOS_UINT32      udwIdleNext 		= VOS_NULL_DWORD;
	VOS_UINT32      udwUsedTail 		= VOS_NULL_DWORD;

	pucPhysicalMem = pstSdbContainer->pucPhysicalMem;
    udwLen = sizeof(LINK_ITEM) + sizeof(CONFLICT_LINK_ITEM) * MAX_LINK_NUM
    		+ pstSdbContainer->udwItemSize;


    if(VOS_NULL_PTR == pstSdbContainer)
    {
        return VOS_NULL_DWORD;
    }

    if(pstSdbContainer->udwIdleHead >= pstSdbContainer->udwMaxNum)
    {
    	printf("table is full\n");
        return VOS_NULL_DWORD;
    }

    udwCurr = pstSdbContainer->udwIdleHead;
    pstCurrLinkItem = (LINK_ITEM *)(pucPhysicalMem + udwCurr * udwLen);

    /* 移除空闲链的第一个节点 */
    udwIdleNext = pstCurrLinkItem->udwNext;
    if(udwIdleNext < pstSdbContainer->udwMaxNum)
    {
    	pstIdleNext = (LINK_ITEM *)(pucPhysicalMem + udwIdleNext * udwLen);
    	pstIdleNext->udwPrev = VOS_NULL_DWORD;
    	pstSdbContainer->udwIdleHead = udwIdleNext;
    }
    else
    {
    	pstSdbContainer->udwIdleHead = VOS_NULL_DWORD;
    	pstSdbContainer->udwIdleTail = VOS_NULL_DWORD;
    }

    /* 将当前节点，添加到占用链的尾部 */
    udwUsedTail = pstSdbContainer->udwUsedTail;
    if(udwUsedTail < pstSdbContainer->udwMaxNum)
    {
    	pstUsedTail = (LINK_ITEM *)(pucPhysicalMem + udwUsedTail * udwLen);
    	pstUsedTail->udwNext = udwCurr;
    	pstSdbContainer->udwUsedTail = udwCurr;
    	pstCurrLinkItem->udwPrev = udwUsedTail;
    	pstCurrLinkItem->udwNext = VOS_NULL_DWORD;
    }
    else
    {
    	pstSdbContainer->udwUsedHead = udwCurr;
    	pstSdbContainer->udwUsedTail = udwCurr;
    	pstCurrLinkItem->udwPrev     = VOS_NULL_DWORD;
    	pstCurrLinkItem->udwNext     = VOS_NULL_DWORD;
    }

    pstCurrLinkItem->enStatus = ENUM_ITEM_USED;

    return udwCurr;
}


VOS_UINT32 SDB_FreeItem(SDB_CONTAINER *pstSdbContainer, VOS_UINT32 udwIndex)
{
	VOS_UINT8       *pucPhysicalMem     = VOS_NULL_PTR;
	LINK_ITEM       *pstLinkItem    	= VOS_NULL_PTR;
	LINK_ITEM       *pstIdleTail    	= VOS_NULL_PTR;
	LINK_ITEM       *pstUsedPrev        = VOS_NULL_PTR;
	LINK_ITEM       *pstUsedNext        = VOS_NULL_PTR;
	VOS_UINT32      udwLen         		= 0;

	pucPhysicalMem = pstSdbContainer->pucPhysicalMem;
    udwLen = sizeof(LINK_ITEM) + sizeof(CONFLICT_LINK_ITEM) * MAX_LINK_NUM
    		+ pstSdbContainer->udwItemSize;


    if(VOS_NULL_PTR == pstSdbContainer)
    {
    	printf("null input ptr");
        return VOS_ERR;
    }

    if(udwIndex >= pstSdbContainer->udwMaxNum)
    {
    	printf("invalid index = %d", udwIndex);
    	return VOS_ERR;
    }

    pstLinkItem = (LINK_ITEM *)(pucPhysicalMem + udwLen * udwIndex);
    if(ENUM_ITEM_USED != pstLinkItem->enStatus)
    {
    	printf("status = %d, is unexpected", pstLinkItem->enStatus);
    	return VOS_ERR;
    }

    /* 将当前节点，从占用链中移除，修改previous node */
    if(pstLinkItem->udwPrev < pstSdbContainer->udwMaxNum)
    {
    	pstUsedPrev = (LINK_ITEM *)(pucPhysicalMem + pstLinkItem->udwPrev * udwLen);
    	pstUsedPrev->udwNext = pstLinkItem->udwNext;
    }
    else
    {
    	pstSdbContainer->udwUsedHead = pstLinkItem->udwNext;
    }

    /* 将当前节点，从占用链中移除，修改next node */
    if(pstLinkItem->udwNext < pstSdbContainer->udwMaxNum)
    {
    	pstUsedNext = (LINK_ITEM *)(pucPhysicalMem + pstLinkItem->udwNext * udwLen);
    	pstUsedNext->udwPrev = pstLinkItem->udwPrev;
    }
    else
    {
    	pstSdbContainer->udwUsedTail = pstLinkItem->udwPrev;
    }

    /* 将当前节点，加入到空闲链的尾部 */
    if(pstSdbContainer->udwIdleTail < pstSdbContainer->udwMaxNum)
    {
    	pstIdleTail = (LINK_ITEM *)(pucPhysicalMem + pstSdbContainer->udwIdleTail * udwLen);
    	pstIdleTail->udwNext = udwIndex;
    	pstLinkItem->udwPrev = pstSdbContainer->udwIdleTail;
    	pstLinkItem->udwNext = VOS_NULL_DWORD;
    	pstSdbContainer->udwIdleTail = udwIndex;
    }
    else
    {
    	pstSdbContainer->udwIdleHead = udwIndex;
    	pstSdbContainer->udwIdleTail = udwIndex;
    	pstLinkItem->udwNext = VOS_NULL_DWORD;
    	pstLinkItem->udwPrev = VOS_NULL_DWORD;
    }

    pstLinkItem->enStatus = ENUM_ITEM_IDLE;

    return VOS_OK;
}

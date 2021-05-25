#include "stdafx.h"

#include <iostream>

#include "GCASSERT.h"
#include "Heap.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap CHeap
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// responsible for default initializing any state variables used
// Memory managed by CHeap is passed in via initialize(..)
//////////////////////////////////////////////////////////////////////////
CHeap::CHeap( void )
{
	m_EHeapErrorLast = EHeapError_NotInitialised;
	m_iNumOfAllocs = -1;
	m_pMemorySpace = nullptr;
	m_iMemorySize = 0;
	m_iAllocatedMem = 0;
}


//////////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////////
CHeap::~CHeap( void )
{
}



//////////////////////////////////////////////////////////////////////////
// This function initializes the memory managed by CHeap.
// CHeap is not usable before this function has been called and 
// CHeap::GetLastError() returns EHeapError_Ok
// pRawMemory must be aligned to a power of 2 >= CHEAP_PLATFORM_MIN_ALIGN
//////////////////////////////////////////////////////////////////////////
void CHeap::Initialise( u8* pRawMemory, u32 uMemorySizeInBytes )
{
	if (IsPowerOfTwo( uMemorySizeInBytes ))
	{
		m_pMemorySpace = pRawMemory;
		m_iMemorySize = uMemorySizeInBytes;
		m_iAllocatedMem = 0;
		m_EHeapErrorLast = EHeapError_Ok;
		m_iNumOfAllocs = 0;
		for (u32 i = 0; i < uMemorySizeInBytes; i++)
		{
			m_pMemorySpace[i] = 'o';
		}
	}
	else
	{
		m_EHeapErrorLast = EHeapError_Init_BadAlign;
	}
}

//////////////////////////////////////////////////////////////////////////
// Explicit destructor. 
// Your implementation will determine if this function is required.
//////////////////////////////////////////////////////////////////////////
void CHeap::Shutdown( void )
{
	delete m_pMemorySpace;
	m_pMemorySpace = nullptr;
	m_EHeapErrorLast = EHeapError_NotInitialised;
}


//////////////////////////////////////////////////////////////////////////
// proxies the call to AllocateAligned() with default platform alignment
//////////////////////////////////////////////////////////////////////////
void* CHeap::Allocate( u32 uNumBytes )
{
	return AllocateAligned( uNumBytes, CHEAP_PLATFORM_MIN_ALIGN );
}


//////////////////////////////////////////////////////////////////////////
// Allocates the specified size of memory, with the specified alignment
// and returns a pointer to it.
// 
// N.B. as extra data is required to track allocated & free memory blocks 
// the actual size of the chunk of memory you use out of your free memory 
// will be > uNumBytes
//////////////////////////////////////////////////////////////////////////
void* CHeap::AllocateAligned( u32 uNumBytes, u32 uAlignment )
{
	if (m_EHeapErrorLast == EHeapError_NotInitialised)
	{
		return nullptr;
	}

	if (!IsPowerOfTwo( uAlignment ))
	{
		m_EHeapErrorLast = EHeapError_Alloc_BadAlign;
		return nullptr;
	}

	u32 BytesToAllocate = uNumBytes;
	//If user tries to allocate less than the minimum alignment allocation, allocate at least 
	//that much, to avoid memory being unallocated (left to 'o') within an allocated area
	if (uNumBytes < CHEAP_PLATFORM_MIN_ALIGN)
	{
		BytesToAllocate = CHEAP_PLATFORM_MIN_ALIGN;
	}

	//Remember to start the allocated area with the char sequence 'eegg' and end it with 'ggee'
	//This gives us 8 bytes of overhead to play with (nowhere near the 64 we have for this test)

	u8* pPointerToReturn = m_pMemorySpace;
	u32 uFree = -1; //If this is 0, the manager omits the first 4 bytes.

	do
	{
		pPointerToReturn += (uFree + 5);
		u32 delta = CalculateAlignmentDelta( pPointerToReturn, uAlignment );
		pPointerToReturn = pPointerToReturn + delta;

		if (pPointerToReturn >= (m_pMemorySpace + (m_iMemorySize - 1)))
		{
			m_EHeapErrorLast = EHeapError_Alloc_NoLargeEnoughBlocks;
			return nullptr;
		}

		uFree = CountFreeFromPointer( pPointerToReturn );

	} while (uFree < BytesToAllocate + 4);

	*(pPointerToReturn - 4) = 'e';
	*(pPointerToReturn - 3) = 'e';
	*(pPointerToReturn - 2) = 'g';
	*(pPointerToReturn - 1) = 'g';

	u32 i;

	for (i = 0; i < BytesToAllocate; i++)
	{
		*(pPointerToReturn + i) = 'c';
	}


	//Not the first one, because it has been incremented when the for loop exited
	*(pPointerToReturn + i) = 'g';
	*(pPointerToReturn + ++i) = 'g';
	*(pPointerToReturn + ++i) = 'e';
	*(pPointerToReturn + ++i) = 'e';

	m_iAllocatedMem += (BytesToAllocate + 8);
	++m_iNumOfAllocs;


	//Remember to return the pointer to AFTER the initialisation sequence 'eegg'.
	if (0 == uNumBytes)
	{
		m_EHeapErrorLast = EHeapError_Alloc_WarnZeroSize;
		//return nullptr;
	}
	else
	{
		m_EHeapErrorLast = EHeapError_Ok;
	}
	return pPointerToReturn;
}

//////////////////////////////////////////////////////////////////////////
// deallocates the memory pointed to by pMemory and returns it to the 
// free memory stored in the heap.
//////////////////////////////////////////////////////////////////////////
void CHeap::Deallocate( void* pPointerToDelete )
{

	if (!pPointerToDelete)
	{
		m_EHeapErrorLast = EHeapError_Dealloc_WarnNULL;
		return;
	}

	//Verify Guard Bytes at start (all 4). If not, set error and return
	u8* pMemory = (u8*)pPointerToDelete;

	if ('d' == *pMemory)
	{
		m_EHeapErrorLast = EHeapError_Dealloc_AlreadyDeallocated;
		return;
	}

	bool check = ('e' == *(pMemory - 4) && 'e' == *(pMemory - 3) && 'g' == *(pMemory - 2) && 'g' == *(pMemory - 1));

	if (!check)
	{
		m_EHeapErrorLast = EHeapError_Dealloc_OverwriteUnderrun;
		return;
	}

	//Reset guard bytes to 'o'
	*(pMemory - 4) = 'o';
	*(pMemory - 3) = 'o';
	*(pMemory - 2) = 'o';
	*(pMemory - 1) = 'o';

	//Reset bytes to d until you encounter a 'g'. If you encounter 'e' before 'g', the heap is corrupted. Set error and return.

	bool pointerEnd = false;
	u32 sizeDeleted = 0;
	do
	{
		*(pMemory++) = 'd';
		++sizeDeleted;

		//reached end of memory without finding the guard bytes
		if (pMemory >= m_pMemorySpace + (m_iMemorySize))
		{
			m_EHeapErrorLast = EHeapError_Dealloc_OverwriteOverrun;
			return;
		}
		if ('e' == *(pMemory))
		{
			m_EHeapErrorLast = EHeapError_Dealloc_OverwriteOverrun;
			return;
		}
		pointerEnd = ('g' == *(pMemory));
	} while (!pointerEnd);

	//verify and reset the end guard bytes (all 4) to 'o'. If not, set error and return.
	check = ('e' == *(pMemory + 3) && 'e' == *(pMemory + 2) && 'g' == *(pMemory + 1) && 'g' == *(pMemory)); // Because the pointer has increased to show the first 'g' within while

	if (!check)
	{
		m_EHeapErrorLast = EHeapError_Dealloc_OverwriteOverrun;
		return;
	}

	*(pMemory + 3) = 'o';
	*(pMemory + 2) = 'o';
	*(pMemory + 1) = 'o';
	*(pMemory) = 'o';


	m_iAllocatedMem -= (sizeDeleted + 8);
	--m_iNumOfAllocs;
	m_EHeapErrorLast = EHeapError_Ok;
	//return
}


//////////////////////////////////////////////////////////////////////////
// this should return the current number of allocations
//////////////////////////////////////////////////////////////////////////
u32 CHeap::GetNumAllocs( void )
{
	return m_iNumOfAllocs;
}


//////////////////////////////////////////////////////////////////////////
// N.B. this should return ( size of heap - size of memory in all 
// allocated blocks including overheads ).
// This value will always be slightly greater than the maximum 
// allocatable memory.
//////////////////////////////////////////////////////////////////////////
u32 CHeap::GetFreeMemory( void )
{
	return m_iMemorySize - m_iAllocatedMem;
}


//////////////////////////////////////////////////////////////////////////
// YOU WILL NEED TO IMPLEMENT THIS FUNCTION AND A WAY OF STORING THE 
// ERRORS WHEN THEY OCCUR
//////////////////////////////////////////////////////////////////////////
CHeap::EHeapError CHeap::GetLastError( void )
{
	return m_EHeapErrorLast;
}


//////////////////////////////////////////////////////////////////////////
// returns 0 or a + ve value to add to the address of pPointerToAlign
// to align it to uAlignment.
// 
// Example of usage to get a 64 byte aligned address:
// 
// void*	pUnalignedAddress		= GetMemoryFromSomewhere();
// u32		uAlignmentDelta			= CHeap::CalculateAlignmentDelta( pUnaligned, 64 );
// u8*		p64ByteAlignedAddress	= ((u8*) pUnaligned ) + uAlignmentDelta );
//
// N.B. uAlignment MUST be a power of 2, this function does NOT check this
// you will have to check that in your code before calling this function
//////////////////////////////////////////////////////////////////////////
// static
u32 CHeap::CalculateAlignmentDelta( void* pPointerToAlign, u32 uAlignment )
{
	// Power of two numbers have lots of almost magical seeming properties in binary.
	//
	// I leave it to you to work out how this code aligns uAligned to uAlignment...
	//
	// Hint: write out the binary values long hand at each step in the calculation
	//
	// Hint 2: you can use very similar techniques to test whether a 
	// binary number is a power of 2 (which might come in handy in passing one of the other tests...)
	u32 uAlignAdd = uAlignment - 1;
	u32 uAlignMask = ~uAlignAdd;
	u32 uAddressToAlign = ((u32)pPointerToAlign);
	u32 uAligned = uAddressToAlign + uAlignAdd;
	uAligned &= uAlignMask;
	return(uAligned - uAddressToAlign);
}

bool CHeap::IsPowerOfTwo( u32 numToCheck )
{
	while (numToCheck >= 2)
	{
		numToCheck %= 2;
	}

	return numToCheck == 0;
}

u32 CHeap::CountFreeFromPointer( u8 * PointerToStart )
{
	u32 freespace = 0;

	while (*PointerToStart == 'o' || *PointerToStart == 'd')
	{
		++freespace;
		++PointerToStart;
		if (PointerToStart >= (m_pMemorySpace + (m_iMemorySize - 1)))
		{
			return freespace;
		}
	}

	return freespace;
}


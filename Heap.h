#ifndef _HEAP_H_
#define _HEAP_H_

#include "types.h"
#include "GCAssert.h"


//////////////////////////////////////////////////////////////////////////
// default alignment for windows platform is sizeof(u32)
//////////////////////////////////////////////////////////////////////////
#define CHEAP_PLATFORM_MIN_ALIGN	(sizeof(u32))


//////////////////////////////////////////////////////////////////////////
// CHeap class
//
// Please describe your implementation here. 
//
// Pay particular attention to architectural choices you made (and why you 
// made them).
//
// If aspects of functionality are split across groups of functions, this 
// might be a good place to mention the functions in each group and what 
// they do.
//
// Obviously feel free to comment the functions too :)
// 
// RULES:
// ^^^^^^
// Your heap implementation:
// 
// 1) MUST NOT do ANY dynamic memory allocations (you can't call new or delete)
// 
// 2) MUST NOT cause ANY dynamic memory allocations (you can't use library code that dynamically allocates)
//
// 3) MUST ONLY use member variables whose size is fixed at compile time
// 
// 4) the ONLY additional memory you have access to is the memory buffer passed to CHeap::Initialise 
// 
// 5) you are allowed to use 'placement' form of new - it allows you to call a constructor on a chunk of memory 
// 
// 
// implementation hints:
// ^^^^^^^^^^^^^^^^^^^^^
// * you need to be able to get back to the layout of a memory block
// from the pointer to the allocated memory you passed out from Allocate() 
// when it is returned to Deallocate(). 
// 
// * A good way to do this might be to define a struct (or structs?) that 
// specify the layout of free and allocated memory blocks
//
// * you may find ascii art diagrams of memory layout for free / allocated 
// blocks helpful (will certainly help me understand your code)
//
// * for pointer arithmetic to work how you expect (i.e. adding 1 to a pointer 
// adds 1 byte to its address), you will need to work with your pointers as u8* 
// (i.e. pointer to byte). 
// 
// * C or C++ style casts are fine, as long as your code is neat, clean, and easy to read.
//
// * I'm giving you one freebie: CalculateAlignmentDelta()
// 
// * when aligning a pointer P to an alignment of A bytes:
// 
//		* if P is already aligned to A bytes then Q == P
//		* if P not aligned to A bytes then Q < (P + A)
//
//////////////////////////////////////////////////////////////////////////
class CHeap
{
public:
	//////////////////////////////////////////////////////////////////////////
	// enum of possible error return values from CHeap::GetLastError()
	// These are grouped into errors relating to specific functionality
	//////////////////////////////////////////////////////////////////////////
	enum EHeapError
	{
		EHeapError_Ok = 0,						// no error

		EHeapError_NotInitialised,				// not yet initialised
		EHeapError_Init_BadAlign,				// memory passed Initialise wasn't aligned to CHEAP_PLATFORM_MIN_ALIGN 

		EHeapError_Alloc_WarnZeroSize,			// allocation of 0 bytes requested (valid, but might indicate bug)
		EHeapError_Alloc_BadAlign,				// alignment specified is < CHEAP_PLATFORM_MIN_ALIGN or *not a power of 2* 
		EHeapError_Alloc_NoLargeEnoughBlocks,	// can't find a large enough free block to allocate requested size

		EHeapError_Dealloc_WarnNULL,			// warn NULL passed for dealloc
		EHeapError_Dealloc_AlreadyDeallocated,	// tried to deallocate a block that's already deallocated
		EHeapError_Dealloc_OverwriteUnderrun,	// memory overwrite was detected before the allocated block 
		EHeapError_Dealloc_OverwriteOverrun,	// memory overwrite was detected after the allocated block 
	};


	// default constructor and destructor
	CHeap( void );
	~CHeap( void );

	// explicit construction and destruction
	void	Initialise( u8* pRawMemory, u32 uMemorySizeInBytes );
	void	Shutdown( void );

	// allocate deallocate
	void*	Allocate( u32 uNumBytes );
	void*	AllocateAligned( u32 uNumBytes, u32 uAlignment );
	void 	Deallocate( void* pMemory );

	// get info about the current Heap state
	u32		GetNumAllocs( void );

	// N.B. this should return ( size of heap - memory allocated including overheads )
	// Clearly this is slightly > maximum allocatable memory
	u32		GetFreeMemory( void );

	// if the last operation caused an error this should return a value != EHeapError_Ok
	EHeapError GetLastError( void );

	// returns a POSITIVE value between 0 and (uAlignment-1) to add to the address of 
	// pPointerToAlign in order to align it to uAlignment
	static u32 CalculateAlignmentDelta( void* pPointerToAlign, u32 uAlignment );

	//
	// add public functions
	//

private:

	// 
	// add private data / functions
	//


	EHeapError m_EHeapErrorLast;
	u32 m_iAllocatedMem;
	u32 m_iNumOfAllocs;
	u32 m_iMemorySize;
	u8* m_pMemorySpace;

	bool IsPowerOfTwo( u32 num );
	u32 CountFreeFromPointer( u8* pPointer );
};
#endif // #ifndef _HEAP_H_
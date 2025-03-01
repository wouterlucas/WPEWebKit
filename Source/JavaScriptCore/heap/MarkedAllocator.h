/*
 * Copyright (C) 2012, 2013, 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include "AllocatorAttributes.h"
#include "FreeList.h"
#include "MarkedBlock.h"
#include <wtf/FastBitVector.h>
#include <wtf/SentinelLinkedList.h>
#include <wtf/Vector.h>

namespace JSC {

class GCDeferralContext;
class Heap;
class MarkedSpace;
class LLIntOffsetsExtractor;

#define FOR_EACH_MARKED_ALLOCATOR_BIT(macro) \
    macro(live, Live) /* The set of block indices that have actual blocks. */\
    macro(empty, Empty) /* The set of all blocks that have no live objects and nothing to destroy. */ \
    macro(allocated, Allocated) /* The set of allblocks that are full of live objects. */\
    macro(canAllocateButNotEmpty, CanAllocateButNotEmpty) /* The set of all blocks are neither empty nor retired (i.e. are more than minMarkedBlockUtilization full). */ \
    macro(eden, Eden) /* The set of all blocks that have new objects since the last GC. */\
    macro(unswept, Unswept) /* The set of all blocks that could be swept by the incremental sweeper. */\
    \
    /* These are computed during marking. */\
    macro(markingNotEmpty, MarkingNotEmpty) /* The set of all blocks that are not empty. */ \
    macro(markingRetired, MarkingRetired) /* The set of all blocks that are retired. */

// FIXME: We defined canAllocateButNotEmpty and empty to be exclusive:
//
//     canAllocateButNotEmpty & empty == 0
//
// Instead of calling it canAllocate and making it inclusive:
//
//     canAllocate & empty == empty
//
// The latter is probably better. I'll leave it to a future bug to fix that, since breathing on
// this code leads to regressions for days, and it's not clear that making this change would
// improve perf since it would not change the collector's behavior, and either way the allocator
// has to look at both bitvectors.
// https://bugs.webkit.org/show_bug.cgi?id=162121

// Note that this collector supports overlapping allocator state with marking state, since in a
// concurrent collector you allow allocation while marking is running. So it's best to visualize a
// full mutable->eden collect->mutate->full collect cycle and see how the bits above get affected.
// The example below tries to be exhaustive about what happens to the bits, but omits a lot of
// things that happen to other state.
//
// Create allocator
// - all bits are empty
// Start allocating in some block
// - allocate the block and set the live bit.
// - the empty bit for the block flickers on and then gets immediately cleared by sweeping.
// - set the eden bit.
// Finish allocating in that block
// - set the allocated bit.
// Do that to a lot of blocks and then start an eden collection.
// - beginMarking() has nothing to do.
// - by default we have cleared markingNotEmpty/markingRetired bits.
// - marking builds up markingNotEmpty/markingRetired bits.
// We do endMarking()
// - clear all allocated bits.
// - for destructor blocks: fragmented = live & ~markingRetired
// - for non-destructor blocks:
//       empty = live & ~markingNotEmpty
//       fragmented = live & markingNotEmpty & ~markingRetired
// Snapshotting.
// - unswept |= eden
// Prepare for allocation.
// - clear eden
// Finish collection.
// Allocate in some block that had some free and some live objects.
// - clear the canAllocateButNotEmpty bit
// - clear the unswept bit
// - set the eden bit
// Finish allocating (set the allocated bit).
// Allocate in some block that was completely empty.
// - clear the empty bit
// - clear the unswept bit
// - set the eden bit.
// Finish allocating (set the allocated bit).
// Allocate in some block that was completely empty in another allocator.
// - clear the empty bit
// - clear all bits in that allocator
// - set the live bit in another allocator and the empty bit.
// - clear the empty, unswept bits.
// - set the eden bit.
// Finish allocating (set the allocated bit).
// Start a full collection.
// - beginMarking() clears markingNotEmpty, markingRetired
// - the heap version is incremented
// - marking rebuilds markingNotEmpty/markingretired bits.
// We do endMarking()
// - clear all allocated bits.
// - set canAllocateButNotEmpty/empty the same way as in eden collection.
// Snapshotting.
// - unswept = live
// prepare for allocation.
// - clear eden.
// Finish collection.
//
// Notice how in this scheme, the empty/canAllocateButNotEmpty state stays separate from the
// markingNotEmpty/markingRetired state. This is one step towards having separated allocation and
// marking state.

class MarkedAllocator {
    friend class LLIntOffsetsExtractor;

public:
    static ptrdiff_t offsetOfFreeList();
    static ptrdiff_t offsetOfCellSize();

    MarkedAllocator(Heap*, MarkedSpace*, size_t cellSize, const AllocatorAttributes&);
    void lastChanceToFinalize();
    void prepareForAllocation();
    void stopAllocating();
    void resumeAllocating();
    void beginMarkingForFullCollection();
    void endMarking();
    void snapshotUnsweptForEdenCollection();
    void snapshotUnsweptForFullCollection();
    void sweep();
    void shrink();
    void assertNoUnswept();
    size_t cellSize() const { return m_cellSize; }
    const AllocatorAttributes& attributes() const { return m_attributes; }
    bool needsDestruction() const { return m_attributes.destruction == NeedsDestruction; }
    DestructionMode destruction() const { return m_attributes.destruction; }
    HeapCell::Kind cellKind() const { return m_attributes.cellKind; }
    void* allocate(GCDeferralContext* = nullptr);
    void* tryAllocate(GCDeferralContext* = nullptr);
    Heap* heap() { return m_heap; }
    MarkedBlock::Handle* takeLastActiveBlock()
    {
        MarkedBlock::Handle* block = m_lastActiveBlock;
        m_lastActiveBlock = 0;
        return block;
    }
    
    template<typename Functor> void forEachBlock(const Functor&);
    
    void addBlock(MarkedBlock::Handle*);
    void removeBlock(MarkedBlock::Handle*);

    bool isPagedOut(double deadline);
    
    static size_t blockSizeForBytes(size_t);
    
#define MARKED_ALLOCATOR_BIT_ACCESSORS(lowerBitName, capitalBitName)     \
    bool is ## capitalBitName(size_t index) const { return m_ ## lowerBitName[index]; } \
    bool is ## capitalBitName(MarkedBlock::Handle* block) const { return is ## capitalBitName(block->index()); } \
    void setIs ## capitalBitName(size_t index, bool value) { m_ ## lowerBitName[index] = value; } \
    void setIs ## capitalBitName(MarkedBlock::Handle* block, bool value) { setIs ## capitalBitName(block->index(), value); } \
    bool atomicSetAndCheckIs ## capitalBitName(size_t index, bool value) { return m_ ## lowerBitName.atomicSetAndCheck(index, value); } \
    bool atomicSetAndCheckIs ## capitalBitName(MarkedBlock::Handle* block, bool value) { return m_ ## lowerBitName.atomicSetAndCheck(block->index(), value); }
    FOR_EACH_MARKED_ALLOCATOR_BIT(MARKED_ALLOCATOR_BIT_ACCESSORS)
#undef MARKED_ALLOCATOR_BIT_ACCESSORS

    template<typename Func>
    void forEachBitVector(const Func& func)
    {
#define MARKED_ALLOCATOR_BIT_CALLBACK(lowerBitName, capitalBitName) \
        func(m_ ## lowerBitName);
        FOR_EACH_MARKED_ALLOCATOR_BIT(MARKED_ALLOCATOR_BIT_CALLBACK);
#undef MARKED_ALLOCATOR_BIT_CALLBACK
    }
    
    template<typename Func>
    void forEachBitVectorWithName(const Func& func)
    {
#define MARKED_ALLOCATOR_BIT_CALLBACK(lowerBitName, capitalBitName) \
        func(m_ ## lowerBitName, #capitalBitName);
        FOR_EACH_MARKED_ALLOCATOR_BIT(MARKED_ALLOCATOR_BIT_CALLBACK);
#undef MARKED_ALLOCATOR_BIT_CALLBACK
    }
    
    MarkedAllocator* nextAllocator() const { return m_nextAllocator; }
    
    // MarkedSpace calls this during init.
    void setNextAllocator(MarkedAllocator* allocator) { m_nextAllocator = allocator; }
    
    MarkedBlock::Handle* findEmptyBlockToSteal();
    
    MarkedBlock::Handle* findBlockToSweep();
    
    void dump(PrintStream&) const;
    void dumpBits(PrintStream& = WTF::dataFile());
   
private:
    friend class MarkedBlock;
    
    bool shouldStealEmptyBlocksFromOtherAllocators() const;
    
    JS_EXPORT_PRIVATE void* allocateSlowCase(GCDeferralContext*);
    JS_EXPORT_PRIVATE void* tryAllocateSlowCase(GCDeferralContext*);
    void* allocateSlowCaseImpl(GCDeferralContext*, bool crashOnFailure);
    void didConsumeFreeList();
    void* tryAllocateWithoutCollecting();
    MarkedBlock::Handle* tryAllocateBlock();
    void* tryAllocateIn(MarkedBlock::Handle*);
    void* allocateIn(MarkedBlock::Handle*);
    ALWAYS_INLINE void doTestCollectionsIfNeeded(GCDeferralContext*);
    
    void setFreeList(const FreeList&);
    
    FreeList m_freeList;
    
    Vector<MarkedBlock::Handle*> m_blocks;
    Vector<unsigned> m_freeBlockIndices;
    
#define MARKED_ALLOCATOR_BIT_DECLARATION(lowerBitName, capitalBitName) \
    FastBitVector m_ ## lowerBitName;
    FOR_EACH_MARKED_ALLOCATOR_BIT(MARKED_ALLOCATOR_BIT_DECLARATION)
#undef MARKED_ALLOCATOR_BIT_DECLARATION
    
    // After you do something to a block based on one of these cursors, you clear the bit in the
    // corresponding bitvector and leave the cursor where it was.
    size_t m_allocationCursor { 0 }; // Points to the next block that is a candidate for allocation.
    size_t m_emptyCursor { 0 }; // Points to the next block that is a candidate for empty allocation (allocating in empty blocks).
    size_t m_unsweptCursor { 0 }; // Points to the next block that is a candidate for incremental sweeping.
    
    MarkedBlock::Handle* m_currentBlock;
    MarkedBlock::Handle* m_lastActiveBlock;

    Lock m_lock;
    unsigned m_cellSize;
    AllocatorAttributes m_attributes;
    Heap* m_heap;
    MarkedSpace* m_markedSpace;
    MarkedAllocator* m_nextAllocator;
};

inline ptrdiff_t MarkedAllocator::offsetOfFreeList()
{
    return OBJECT_OFFSETOF(MarkedAllocator, m_freeList);
}

inline ptrdiff_t MarkedAllocator::offsetOfCellSize()
{
    return OBJECT_OFFSETOF(MarkedAllocator, m_cellSize);
}

ALWAYS_INLINE void* MarkedAllocator::tryAllocate(GCDeferralContext* deferralContext)
{
    unsigned remaining = m_freeList.remaining;
    if (remaining) {
        unsigned cellSize = m_cellSize;
        remaining -= cellSize;
        m_freeList.remaining = remaining;
        return m_freeList.payloadEnd - remaining - cellSize;
    }
    
    FreeCell* head = m_freeList.head;
    if (UNLIKELY(!head))
        return tryAllocateSlowCase(deferralContext);
    
    m_freeList.head = head->next;
    return head;
}

ALWAYS_INLINE void* MarkedAllocator::allocate(GCDeferralContext* deferralContext)
{
    unsigned remaining = m_freeList.remaining;
    if (remaining) {
        unsigned cellSize = m_cellSize;
        remaining -= cellSize;
        m_freeList.remaining = remaining;
        return m_freeList.payloadEnd - remaining - cellSize;
    }
    
    FreeCell* head = m_freeList.head;
    if (UNLIKELY(!head))
        return allocateSlowCase(deferralContext);
    
    m_freeList.head = head->next;
    return head;
}

template <typename Functor> inline void MarkedAllocator::forEachBlock(const Functor& functor)
{
    m_live.forEachSetBit(
        [&] (size_t index) {
            functor(m_blocks[index]);
        });
}

} // namespace JSC

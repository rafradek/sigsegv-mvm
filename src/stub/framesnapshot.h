#ifndef _INCLUDE_SIGSEGV_STUB_FRAMESNAPSHOT_H_
#define _INCLUDE_SIGSEGV_STUB_FRAMESNAPSHOT_H_

#include <cstddef>

class CFrameSnapshotEntry;

// Engine-owned prefix only; never allocate an engine snapshot using this stub.
// Linux TF2 as of 2026-10-05 starts with the tick, not m_ListIndex. Leaving the
// obsolete leading integer on x86 reads the valid-entity count as a pointer.
// Other engines retain their existing layout until independently verified.
class CFrameSnapshot
{
    DECLARE_FIXEDSIZE_ALLOCATOR(CFrameSnapshot);
public:
    CFrameSnapshot();
    ~CFrameSnapshot();

#ifndef SE_IS_TF2
    CInterlockedInt m_ListIndex;
#endif
    int m_nTickCount;
    CFrameSnapshotEntry *m_pEntities;
    int m_nNumEntities;
    unsigned short *m_pValidEntities;
    int m_nValidEntities;
};

#ifdef SE_IS_TF2
static_assert(offsetof(CFrameSnapshot, m_nTickCount) == 0);
static_assert(offsetof(CFrameSnapshot, m_pEntities) == sizeof(void *));
static_assert(offsetof(CFrameSnapshot, m_nNumEntities) == 2 * sizeof(void *));
static_assert(offsetof(CFrameSnapshot, m_pValidEntities) == 3 * sizeof(void *));
static_assert(offsetof(CFrameSnapshot, m_nValidEntities) == 4 * sizeof(void *));
#endif

#endif

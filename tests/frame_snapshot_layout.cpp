#include <cstddef>
#include <cstdio>

// The allocator adds static members/operators only, not instance storage.
#define DECLARE_FIXEDSIZE_ALLOCATOR(type)
using CInterlockedInt = int;
#include "../src/stub/framesnapshot.h"

#ifdef SE_IS_TF2
static_assert(offsetof(CFrameSnapshot, m_nTickCount) == 0);
static_assert(offsetof(CFrameSnapshot, m_pEntities) == (sizeof(void *) == 4 ? 4 : 8));
static_assert(offsetof(CFrameSnapshot, m_nNumEntities) == (sizeof(void *) == 4 ? 8 : 16));
static_assert(offsetof(CFrameSnapshot, m_pValidEntities) == (sizeof(void *) == 4 ? 12 : 24));
static_assert(offsetof(CFrameSnapshot, m_nValidEntities) == (sizeof(void *) == 4 ? 16 : 32));
#else
static_assert(offsetof(CFrameSnapshot, m_ListIndex) == 0);
static_assert(offsetof(CFrameSnapshot, m_nTickCount) == 4);
static_assert(offsetof(CFrameSnapshot, m_pEntities) == 8);
static_assert(offsetof(CFrameSnapshot, m_nNumEntities) == (sizeof(void *) == 4 ? 12 : 16));
static_assert(offsetof(CFrameSnapshot, m_pValidEntities) == (sizeof(void *) == 4 ? 16 : 24));
static_assert(offsetof(CFrameSnapshot, m_nValidEntities) == (sizeof(void *) == 4 ? 20 : 32));
#endif

int main()
{
    std::printf("snapshot ABI passed: pointer=%zu tick=%zu entities=%zu count=%zu valid=%zu valid_count=%zu\n",
        sizeof(void *), offsetof(CFrameSnapshot, m_nTickCount),
        offsetof(CFrameSnapshot, m_pEntities), offsetof(CFrameSnapshot, m_nNumEntities),
        offsetof(CFrameSnapshot, m_pValidEntities), offsetof(CFrameSnapshot, m_nValidEntities));
}

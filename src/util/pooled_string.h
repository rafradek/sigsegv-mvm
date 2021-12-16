#ifndef _INCLUDE_SIGSEGV_POOLED_STRING_H_
#define _INCLUDE_SIGSEGV_POOLED_STRING_H_

#include "util/autolist.h"
#include "stub/misc.h"

class PooledString : public AutoList<PooledString>
{
public:
    [[gnu::noinline]]
    PooledString(const char *literal) {
        m_literal = literal;
        if (Link::LinkFinished())
            Reset();
    }
    operator const char*() const { return STRING(m_string); }
    operator string_t() const { return m_string; }

    auto operator==(const char *val) const { return (STRING(m_string) == val); }
    auto operator==(const string_t val) const { return (m_string == val); }

    const char *Get() const { return STRING(m_string); }
    void Reset() {m_string = AllocPooledString(m_literal); }
private:
    string_t m_string;
    const char *m_literal;
};

template<FixedString lit>
inline const char *PStr()
{
    static PooledString pooled(lit);
    return pooled.Get();
}

inline const char *operator ""_PStr(const char *str, size_t size)
{
    static PooledString pooled(str);
    return pooled.Get();
}

#endif
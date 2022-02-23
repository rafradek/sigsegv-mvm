#ifndef _INCLUDE_SIGSEGV_UTIL_EXPRESSION_EVAL_H_
#define _INCLUDE_SIGSEGV_UTIL_EXPRESSION_EVAL_H_

static int s_OpPriority[5] = {0,0,0,1,1};
class Evaluation
{ 
public:

    Evaluation(variant_t &result) : result(result) {};

    void Evaluate(const char *expression);

private:
    enum Op
    {
        INVALID,
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE
    };

    inline void ParseChar(char c);

    inline void ParseWhitespace();

    inline void ParseArrayBracket();
     
    inline void ParseQuote();

    inline void ParseParenthesisLeft();

    inline void ParseParenthesisRight();

    inline void ParseOp(Op op);

    void ParseToken(char *token, variant_t &value);

    void DoOp();


    bool m_bParsingVector = false;
    bool m_bParsingString = false;
    bool m_bWasString = false;
    bool m_bParseToken = true;
    Op m_LastOp = INVALID;
    char m_strToken[256];
    char *m_pCurrentChar = m_strToken;
    std::vector<Op> m_OpStack;
    std::vector<variant_t> m_ResultStack;
    variant_t m_curValue;
    variant_t &result;
};

#endif
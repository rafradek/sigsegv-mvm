#ifndef _INCLUDE_SIGSEGV_UTIL_EXPRESSION_EVAL_H_
#define _INCLUDE_SIGSEGV_UTIL_EXPRESSION_EVAL_H_

static int s_OpPriority[] = {0,0,1,1,2,2,2,3,3,3,3,3,3,4,4,5,5,5,6,6,6,7};

class Evaluation
{ 
public:
    using Params = std::deque<variant_t>;
    using ExpressionFunction = void (*)(const char *, Params &, int, variant_t&);
    Evaluation(variant_t &result) : m_Result(result) {};

    void Evaluate(const char *expression, CBaseEntity *self, CBaseEntity *activator, CBaseEntity *caller, variant_t &param);

    static std::vector<const char *> args_optional_empty;
    static void AddFunction(std::string name, ExpressionFunction function, std::vector<std::string> args, std::vector<const char *> args_optional = args_optional_empty);

    class EvaluationFuncDef
    {
    public:
        std::string name;
        ExpressionFunction function;
        const char *expr = nullptr;
        size_t paramCount = 0;
        std::vector<std::string> paramNames;
        std::vector<const char *> paramNamesOptional;
    };
    static const std::vector<Evaluation::EvaluationFuncDef> &GetFunctionList();
private:
    enum Op
    {
        INVALID,
        ASSIGN,
        AND,
        OR,
        AND_BITWISE,
        OR_BITWISE,
        XOR_BITWISE,
        LESS_THAN,
        LESS_THAN_OR_EQUAL,
        GREATER_THAN,
        GREATER_THAN_OR_EQUAL,
        EQUAL,
        NOT_EQUAL,
        RIGHT_SHIFT,
        LEFT_SHIFT,
        CONCAT,
        ADD,
        SUBTRACT,
        MULTIPLY,
        DIVIDE,
        REMAINDER,
        ACCESS,
    };
    enum DeclareMode
    {
        NORMAL,
        IF,
        IF_AFTER_TRUE,
        IF_AFTER_FALSE,
        ELSE_TRUE,
        ELSE_FALSE,
        RETURN
    };
    struct EvaluationFunc
    {
        EvaluationFuncDef *func;
        Params params;
    };
    struct EvaluationStack
    {
        Op op = INVALID;
        variant_t result;
        EvaluationFunc *func = nullptr;
        const char *returnPos = nullptr;
        std::deque<std::string> *varNames = nullptr;
        DeclareMode declMode = NORMAL;
        bool popStackOnEnd = false;
        bool isFunction = false;
    };

    inline void ParseChar(char c);

    inline void ParseWhitespace();

    inline void ParseArrayBracket();
     
    inline void ParseQuote();
    
    inline void ParseParam();

    inline void ParseParenthesisLeft();

    inline void ParseParenthesisRight();

    inline void ParseOp(Op op);

    inline void PushStack();
    inline void PopStack();

    inline void ParseBraceLeft();
    inline void ParseBraceRight();
    inline void ParseSemicolon();
    inline void SkipChars(char from, char to);
    void SkipNextCodeBlock();

    bool FindVariable(std::string &name, variant_t &value);
    bool SetVariable(std::string &name, variant_t &value);

    EvaluationFuncDef *FindFunction(std::string &name);
    void ExecuteFunction(EvaluationFunc *func);

    void ParseToken();

    void DoOp();

    void AddFunctionCustom(const char *name, const char *expr, std::vector<const char *> &args);

    bool m_bParsingVector = false;
    bool m_bParsingFloat = false;
    bool m_bParsingString = false;
    bool m_bWasString = false;
    bool m_bParseToken = true;
    bool m_bParseFunction = false;
    bool m_bParseFunctionParam = false;
    bool m_bTokenParsed = false;
    bool m_bDeclaringVariable = false;
    bool m_bReturning = false;
    std::string m_strToken;
    //std::string::iterator m_pCurrentChar = m_strToken.begin();
    std::deque<EvaluationStack> m_Stack;
    EvaluationStack m_CurStack;
    variant_t m_curValue;
    variant_t &m_Result;

    CBaseEntity *m_activator = nullptr;
    CBaseEntity *m_caller = nullptr;
    CBaseEntity *m_self = nullptr;
    const char *m_pExpression = nullptr;
    EvaluationFuncDef m_DefFunc;
    variant_t m_param;
    std::deque<std::pair<std::string, variant_t>> m_Variables;
    std::deque<EvaluationFuncDef> m_Functions;
    
};

#endif
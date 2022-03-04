#ifndef _INCLUDE_SIGSEGV_UTIL_EXPRESSION_EVAL_H_
#define _INCLUDE_SIGSEGV_UTIL_EXPRESSION_EVAL_H_

static int s_OpPriority[] = {0,0,0,1,1,1,2,2,2,2,2,2,3,3,4,4,4,5,5,5,6};

class Evaluation
{ 
public:
    using Params = std::deque<variant_t>;
    using ExpressionFunction = void (*)(const char *, Params &, int, variant_t&);
    Evaluation(variant_t &result) : result(result) {};

    void Evaluate(const char *expression, CBaseEntity *self, CBaseEntity *activator, CBaseEntity *caller, variant_t &param);

    static std::vector<const char *> args_optional_empty;
    static void AddFunction(const char *name, ExpressionFunction function, std::vector<const char *> args, std::vector<const char *> args_optional = args_optional_empty);

    struct EvaluationFuncDef
    {
        const char *name;
        ExpressionFunction function;
        size_t paramCount = 0;
        std::vector<const char *> paramNames;
        std::vector<const char *> paramNamesOptional;
    };
    struct EvaluationFuncDefCustom
    {
        std::string name;
        const char *expr;
        size_t paramCount = 0;
        std::vector<std::string> paramNames;
    };
    static const std::vector<Evaluation::EvaluationFuncDef> &GetFunctionList();
private:
    enum Op
    {
        INVALID,
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
        REMINDER,
        ACCESS,
    };
    struct EvaluationFunc
    {
        char func[256] = "";
        Params params;
    };
    struct EvaluationStack
    {
        Op op;
        variant_t result;
        EvaluationFunc *func;
        const char *returnPos = nullptr;
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

    bool FindVariable(const char *name, variant_t &value);


    void ExecuteFunction(EvaluationFunc *func);

    void ParseToken(char *token, variant_t &value);

    void DoOp();

    void AddFunctionCustom(const char *name, const char *expr, std::vector<const char *> &args);

    bool m_bParsingVector = false;
    bool m_bParsingFloat = false;
    bool m_bParsingString = false;
    bool m_bWasString = false;
    bool m_bParseToken = true;
    bool m_bParseFunction = false;
    Op m_LastOp = INVALID;
    char m_strToken[256];
    char *m_pCurrentChar = m_strToken;
    std::deque<EvaluationStack> m_Stack;
    EvaluationFunc *m_func = nullptr;
    variant_t m_curValue;
    variant_t &result;

    CBaseEntity *m_activator = nullptr;
    CBaseEntity *m_caller = nullptr;
    CBaseEntity *m_self = nullptr;
    const char *m_pExpression = nullptr;
    const char *m_pReturnPos = nullptr;
    EvaluationFuncDefCustom m_DefFunc;
    variant_t m_param;
    std::deque<std::pair<std::string, variant_t>> m_Variables;
    std::deque<EvaluationFuncDefCustom> m_Functions;
    
};

#endif
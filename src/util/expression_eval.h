#ifndef _INCLUDE_SIGSEGV_UTIL_EXPRESSION_EVAL_H_
#define _INCLUDE_SIGSEGV_UTIL_EXPRESSION_EVAL_H_

static int s_OpPriority[] = {0,0,1,1,2,2,2,3,3,3,3,3,3,4,4,5,5,5,6,6,6,7};

class Evaluation
{ 
public:
    using Params = std::deque<variant_t>;
    using ExpressionFunction = void (*)(const char *, Params &, int, variant_t&);
    
    void Evaluate(const char *expression, CBaseEntity *self, CBaseEntity *activator, CBaseEntity *caller, variant_t &param);

    static std::vector<std::string> args_optional_empty;
    static void AddFunction(std::string name, ExpressionFunction function, std::vector<std::string> args, std::string explanation, std::vector<std::string> args_optional = args_optional_empty);

    class EvaluationFuncDef
    {
    public:
        std::string name;
        ExpressionFunction function;
        const char *expr = nullptr;
        size_t paramCount = 0;
        std::string explanation;
        std::vector<std::string> paramNames;
        std::vector<std::string> paramNamesOptional;
    };

    class ScriptGlobals
    {
    public:
        std::deque<std::pair<std::string, variant_t>> m_Variables;
        std::deque<std::string> m_FunctionExpressions;
        std::vector<EvaluationFuncDef> m_Functions;

        void ClearGlobalState()
        {
            m_Variables.clear();
            m_FunctionExpressions.clear();
            m_Functions.clear();
        }
    };

    static ScriptGlobals s_scriptGlobals;

    Evaluation(variant_t &result) : m_Result(result) {};

    static const std::vector<Evaluation::EvaluationFuncDef> &GetFunctionList();
    static void GetFunctionInfoStrings(std::vector<std::string> &vec);
    static void GetVariableStrings(std::vector<std::string> &vec);

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
        WHILE_BEGIN,
        WHILE_CONDITION,
        WHILE_EXEC,
        DO,
        FOR_BEGIN,
        FOR_CONDITION,
        FOR_INCREMENT,
        FOR_EXEC,
        RETURN
    };
    struct EvaluationFunc
    {
        const EvaluationFuncDef *func;
        Params params;
    };
    struct EvaluationStack
    {
        Op op = INVALID;
        variant_t result;
        EvaluationFunc *func = nullptr;
        const char *returnPos = nullptr;
        DeclareMode declMode = NORMAL;
        std::deque<std::string> *varNames = nullptr;
        std::deque<std::pair<std::string, variant_t>> *variables = nullptr;
        bool popStackOnEnd = false;
        bool isFunction = false;
        bool exportMode = false;
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

    const EvaluationFuncDef *FindFunction(std::string &name);
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
    std::vector<EvaluationFuncDef> m_Functions;
    
    const char *m_pLoopGoBackPosition = nullptr;
};

#endif
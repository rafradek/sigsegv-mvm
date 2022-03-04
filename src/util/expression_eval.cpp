#include "stub/misc.h"
#include "util/expression_eval.h"
#include "util/misc.h"
#include "util/clientmsg.h"

namespace Mod::Etc::Mapentity_Additions
{
    enum GetInputType {
        ANY,
        VARIABLE,
        KEYVALUE,
        DATAMAP,
        SENDPROP
    };

    bool GetEntityVariable(CBaseEntity *entity, GetInputType type, const char *name, variant_t &variable);
}

std::vector<Evaluation::EvaluationFuncDef> expression_functions;

std::vector<const char *> Evaluation::args_optional_empty;

bool ParseNumberOrVectorFromStringStrict(const char *str, variant_t &value, bool asfloat)
{
    float val;
    int valint;
    if (str[0] == '[') {
        //Msg("Parse vector\n");
        Vector vec;
        sscanf(str, "[%f %f %f]", &vec.x, &vec.y, &vec.z);
        value.SetVector3D(vec);
        return true;
    }
    // Parse as int
    else if (!asfloat && StringToIntStrictAndSpend(str, valint)) {
       // Msg("Parse float\n");
        value.SetInt(valint);
        return true;
    }
    // Parse as float
    else if (StringToFloatStrict(str, val)) {
       // Msg("Parse float\n");
        value.SetFloat(val);
        return true;
    }
    return false;
}
const char *function_test = "a + b";
void Evaluation::AddFunction(const char *name, ExpressionFunction function, std::vector<const char *> args, std::vector<const char *> args_optional)
{
    expression_functions.push_back({name, function, args.size(), args, args_optional});
}

const std::vector<Evaluation::EvaluationFuncDef> &Evaluation::GetFunctionList()
{
    return expression_functions;
}

void Evaluation::Evaluate(const char *expression, CBaseEntity *self, CBaseEntity *activator, CBaseEntity *caller, variant_t &param)
{
    m_activator = activator;
    m_caller = caller;
    m_param = param;
    m_self = self;
    m_pExpression = expression;

    result.Convert(FIELD_VOID);
    m_pCurrentChar = m_strToken;
    *m_strToken = '\0';
    char c;
    char specialChar = '\0';
    while((c = (*m_pExpression++)) != '\0') {
        if (m_bParsingString) {
            if (c == '\'') {
                if  (*(m_pCurrentChar-1) != '\\') {
                    m_bParsingString = false;
                    continue;
                }
                else {
                    m_pCurrentChar--;
                }
            }
            // Placeholder comma character to omit , restriction in param value
            else if (c == '\2') {
                c = ',';
            }
            ParseChar(c);
        }
        else {

            // Parse double char operators
            if (specialChar != '\0') {
                if (specialChar == '&') {
                    specialChar = '\0';
                    if (c == '&') {
                        ParseOp(AND);
                        continue;
                    }
                    else {
                        ParseOp(AND_BITWISE);
                    }
                }
                else if (specialChar == '|') {
                    specialChar = '\0';
                    if (c == '|') {
                        ParseOp(OR);
                        continue;
                    }
                    else {
                        ParseOp(OR_BITWISE);
                    }
                }
                else if (specialChar == '>') {
                    specialChar = '\0';
                    if (c == '=') {
                        ParseOp(GREATER_THAN_OR_EQUAL);
                        continue;
                    }
                    else if (c == '>') {
                        ParseOp(RIGHT_SHIFT);
                        continue;
                    }
                    else {
                        ParseOp(GREATER_THAN);
                    }
                }
                else if (specialChar == '<') {
                    specialChar = '\0';
                    if (c == '=') {
                        ParseOp(LESS_THAN_OR_EQUAL);
                        continue;
                    }
                    else if (c == '<') {
                        ParseOp(LEFT_SHIFT);
                        continue;
                    }
                    else {
                        ParseOp(LESS_THAN);
                    }
                }
                else if (specialChar == '.') {
                    specialChar = '\0';
                    if (c == '.') {
                        ParseOp(CONCAT);
                        continue;
                    }
                    else if (m_pCurrentChar != m_strToken && *m_strToken != '+' && *m_strToken != '-' && !(*m_strToken >= '0' && *m_strToken <= '9')) {
                        ParseOp(ACCESS);
                    }
                    else {
                        ParseChar('.');
                        m_bParsingFloat = true;
                    }
                }
                else if (specialChar == '!') {
                    specialChar = '\0';
                    if (c == '=') {
                        ParseOp(NOT_EQUAL);
                        continue;
                    }
                    else {
                        ParseChar('!');
                    }
                }
                else if (specialChar == '=') {
                    specialChar = '\0';
                    if (c == '=') {
                        ParseOp(EQUAL);
                        continue;
                    }
                    else {
                        ParseChar('=');
                    }
                }
            }
            switch (c) {
                case ' ': ParseWhitespace(); break;
                case '+': ParseOp(ADD); break;
                case '-': ParseOp(SUBTRACT); break;
                case '*': ParseOp(MULTIPLY); break;
                case '/': ParseOp(DIVIDE); break;
                case '[': ParseArrayBracket(); break;
                case ']': m_bParsingVector = false; ParseChar(c); break;
                case '\'': ParseQuote(); break;
                case '(': ParseParenthesisLeft(); break;
                case ')': ParseParenthesisRight(); break;
                case '\2': ParseParam(); break;
                case ',': ParseParam(); break;
                case '>': specialChar = '>'; break;
                case '<': specialChar = '<'; break;
                case '&': specialChar = '&'; break;
                case '|': specialChar = '|'; break;
                case '.': specialChar = '.'; break;
                case '!': specialChar = '!'; break;
                case '=': specialChar = '='; break;
                case '{': ParseBraceLeft(); break;
                case '}': ParseBraceRight(); break;
                case ';': ParseSemicolon(); break;
                case '^': ParseOp(XOR_BITWISE); break;
                case '%': ParseOp(REMINDER); break;
                default : ParseChar(c);
            }
        }
    }
    ParseOp(INVALID);
}

void Evaluation::ParseParenthesisLeft()
{
    if (m_pCurrentChar == m_strToken) {
        PushStack();
    }
    else if (m_bParseFunction) {
        *m_pCurrentChar = '\0';
        m_DefFunc.name = m_strToken;
        m_pCurrentChar = m_strToken;
        *m_strToken = '\0';
        Msg("Parse function name %s\n", m_DefFunc.name.c_str());
    }
    else {
        *m_pCurrentChar = '\0';
        m_func = new EvaluationFunc();
        strcpy(m_func->func, m_strToken);
        m_pCurrentChar = m_strToken;
        *m_strToken = '\0';
        PushStack();
    }
}

void Evaluation::ParseParam()
{
    if (m_bParseFunction) {
        *m_pCurrentChar = '\0';
        m_DefFunc.paramNames.push_back(m_strToken);
        Msg("Parse param name %s\n", m_strToken);
        m_pCurrentChar = m_strToken;
        *m_strToken = '\0';
        return;
    }
    ParseOp(INVALID);
    PopStack();
    if (m_func != nullptr) {
        m_func->params.push_back(m_curValue);
    }
    m_curValue.Convert(FIELD_VOID);
    PushStack();
}

void Evaluation::ExecuteFunction(EvaluationFunc *func)
{
    for (auto &pair : expression_functions) {
        if (strcmp(pair.name,func->func) == 0) {
            if (pair.paramCount > func->params.size()) {
                ClientMsgAll("Script Error: Function %s called with %d parameters, need %d:", pair.name, func->params.size(), pair.paramCount);
                for (size_t i = 0; i < pair.paramCount; i++) {
                    ClientMsgAll(", %s", pair.paramNames[i]);
                }
                ClientMsgAll("\n");
                return;
            }
            (*pair.function)(func->func, func->params, func->params.size(), m_curValue);
            return;
        }
    }
    for (auto &pair : m_Functions) {
        if(pair.name == func->func) {
            PushStack();
            m_pReturnPos = m_pExpression;
            m_pExpression = pair.expr;
            for (size_t i = 0; i < pair.paramCount; i++) {
                m_Variables.push_back({pair.paramNames[i], func->params[i]});
            }
            Msg("Execute custom fuction %s go to %s\n", func->func, m_pExpression);
            return;
        }
    }
    ClientMsgAll("Script Error: Function %s not found\n", func->func);
}

bool Evaluation::FindVariable(const char *name, variant_t &value)
{
    for (auto &pair : m_Variables) {
        if (pair.first == name) {
            value = pair.second;
            return true;
        }
    }
    return false;
}

void Evaluation::ParseParenthesisRight()
{
    if (m_bParseFunction) {
        *m_pCurrentChar = '\0';
        m_DefFunc.paramNames.push_back(m_strToken);
        m_pCurrentChar = m_strToken;
        *m_strToken = '\0';
        m_DefFunc.paramCount = m_DefFunc.paramNames.size();
        return;
    }
    ParseOp(INVALID);
    PopStack();
    if (m_func != nullptr) {
       // Msg("Function %s\n", m_func->func);
        m_func->params.push_back(m_curValue);

        ExecuteFunction(m_func);
        delete m_func;
        m_func = nullptr;
    }
}

void Evaluation::ParseBraceLeft()
{
    if (m_bParseFunction) {
        Msg("Parse start fuction %s\n", m_pExpression);
        m_DefFunc.expr = m_pExpression;
        m_Functions.push_back(m_DefFunc);
        SkipChars('{', '}');
        m_bParseFunction = false;
        //Msg("Parse end fuction %s\n", m_pExpression);
    }
}

void Evaluation::ParseBraceRight()
{
    ParseOp(INVALID);
    if (m_pReturnPos != nullptr) {
        m_pExpression = m_pReturnPos;
        //Msg("Execution funcion end go to %s %s\n", m_pExpression, result.String());
    }
    PopStack();
}

void Evaluation::ParseSemicolon()
{
    ParseOp(INVALID);
    m_curValue.Convert(FIELD_VOID);
    result.Convert(FIELD_VOID);
}

// Skip chars inside from and to scope
void Evaluation::SkipChars(char from, char to)
{
    char c;
    int scope = 0;
    bool instring = false;
    while((c = (*m_pExpression++)) != '\0') {
        if (c == '\'') {
            if (instring && *(m_pExpression-2) != '\\') {
                instring = false;
            }
            else if (!instring) {
                instring = true;
            }
        }
        if (!instring) {
            if (c == from) {
                scope++;
            }
            else if (c == to) {
                scope--;
                if (scope < 0) {
                    return;
                }
            }
        }
    }
}

void Evaluation::PushStack()
{
    m_Stack.push_back({m_LastOp, result, m_func, m_pReturnPos});
    m_LastOp = INVALID;
    m_func = nullptr;
    m_pReturnPos = nullptr;
    result.Convert(FIELD_VOID);
    m_bParseToken = true;
}

void Evaluation::PopStack()
{
    if (m_Stack.empty()) {
        ClientMsgAll("Script Error: syntax error, ( expected, got ) or ;\n");
        return;
    }
    //Msg("Result inside %s\n", result.String());
    auto &stack = m_Stack.back();
    m_LastOp = stack.op;
    m_curValue = result;
    result = stack.result;
    if (m_func != nullptr) {
        delete m_func;
    }
    m_func = stack.func;
    m_pReturnPos = stack.returnPos;
    m_Stack.pop_back();
    //if (m_pReturnPos != nullptr)
        //Msg("Result restore %s\n", m_pReturnPos);
    m_bParseToken = false;
}

void Evaluation::ParseArrayBracket()
{
    if (m_pCurrentChar == m_strToken) {
        m_bParsingVector = true;
    }
    ParseChar('[');
}

void Evaluation::ParseQuote()
{
    if (m_pCurrentChar == m_strToken) {
        m_bParsingString = true;
        m_bWasString = true;
        return;
    }
    else {
        
    }
    ParseChar('\'');
}

void Evaluation::ParseChar(char c)
{   
    *m_pCurrentChar++ = c;
}

void Evaluation::ParseWhitespace() 
{
    if (m_bParsingVector || m_bParsingString) {
        ParseChar(' ');
    }
    // Parsing reserved keywords
    else if (m_pCurrentChar != m_strToken && strcmp(m_strToken, "function") == 0) {
        //Msg("Parse function \n");
        m_bParseFunction = true;
        m_pCurrentChar = m_strToken;
        *m_strToken = '\0';
    }
}
#define RAD2DEG2  ( (float)(180.f / M_PI_F) )
#define DEG2RAD2  ( (float)(M_PI_F / 180.f) )

void Evaluation::ParseToken(char *token, variant_t &value)
{
    m_bParseFunction = false;
    *m_pCurrentChar = '\0';
    bool asfloat = m_bParsingFloat;
    m_bParsingFloat = false;
    // Parse as vector
    if (!ParseNumberOrVectorFromStringStrict(token, value, asfloat)) {
        // Parse literal string
        if (m_bWasString) {
            //Msg("Parse literal string\n");
            value.SetString(AllocPooledString(token));
            m_bWasString = false;
            return;
        }

        if (strcmp(token, "pi") == 0) {
            value.SetFloat(M_PI_F);
            return;
        }
        else if (strcmp(token, "degtorad") == 0) {
            value.SetFloat(DEG2RAD2);
            return;
        }
        else if (strcmp(token, "radtodeg") == 0) {
            value.SetFloat(RAD2DEG2);
            return;
        }
        else if (strcmp(token, "true") == 0) {
            value.SetInt(1);
            return;
        }
        else if (strcmp(token, "false") == 0) {
            value.SetInt(0);
            return;
        }
        else if (strcmp(token, "!param") == 0) {
            value = m_param;
            return;
        }
        /*// Parse entity prop
        char *prop = strchr(token, '.');
        //Msg("Parse prop %d\n", prop);
        if (prop == nullptr) {
            value.Convert(FIELD_VOID);
            return;
        }
        // Separate entity and prop strings
        *(prop++) = '\0';
        //Msg("Parse target %s %s\n", token, prop);*/

        if (m_LastOp != ACCESS) {
            if (!FindVariable(token, value)) {
                CHandle<CBaseEntity> targetEnt = servertools->FindEntityGeneric(nullptr, token, m_self, m_activator, m_caller);
                if (targetEnt != nullptr) {
                    value.Set(FIELD_EHANDLE, &targetEnt);
                }
                else {
                    value.Set(FIELD_VOID, nullptr);
                }
            }
        }
        else {
            value.SetString(AllocPooledString(token));
        }
    }
    m_bWasString = false;
}

void Evaluation::DoOp()
{
    //result = m_curValue;
    
    if (m_bParseToken && (m_pCurrentChar != m_strToken || m_bWasString)) {
        //TIME_SCOPE2(ParseToken)
        ParseToken(m_strToken, m_curValue);
    }
    m_bParseToken = true;

    //TIME_SCOPE2(DoOp)
    if (result.FieldType() == FIELD_VOID) {
        result = m_curValue;
    }

    m_pCurrentChar = m_strToken;
    *m_strToken = '\0';
    if (m_LastOp == INVALID) return;

    variant_t& left = result;
    variant_t& right = m_curValue;

    if (m_LastOp == CONCAT) {
        char buf[512];
        strncpy(buf, left.String(), sizeof(buf));
        V_strncat(buf, right.String(), sizeof(buf));
        result.SetString(AllocPooledString(buf));
    }
    else if (m_LastOp == ACCESS) {
        if (left.FieldType() != FIELD_EHANDLE) {
            left.Convert(FIELD_EHANDLE);
        }
        CBaseEntity *entity = left.Entity();
        if (entity == nullptr || !GetEntityVariable(entity, Mod::Etc::Mapentity_Additions::ANY, right.String(), result)) {
            result.Convert(FIELD_VOID);
        }
        if (result.FieldType() == FIELD_BOOLEAN) {
            result.Convert(FIELD_INTEGER);
        }
        if (result.FieldType() == FIELD_CLASSPTR) {
            CBaseEntity *entity = nullptr;
            result.SetOther(entity);
            CHandle<CBaseEntity> ehandle = entity;
            result.Set(FIELD_EHANDLE, &ehandle);
        }
    }

    if (m_LastOp != CONCAT && m_LastOp != ACCESS) {
        if (left.FieldType() == FIELD_STRING) {
            ParseNumberOrVectorFromString(left.String(), left);
        }
        if (right.FieldType() == FIELD_STRING) {
            ParseNumberOrVectorFromString(right.String(), right);
        }
        Vector vec1;
        Vector vec2;
        left.Vector3D(vec1);
        right.Vector3D(vec2);
        if (left.FieldType() == FIELD_INTEGER && (right.FieldType() == FIELD_FLOAT || right.FieldType() == FIELD_VECTOR)) {
            left.Convert(FIELD_FLOAT);
        }
        else if ((left.FieldType() == FIELD_FLOAT || left.FieldType() == FIELD_VECTOR) && right.FieldType() == FIELD_INTEGER) {
            right.Convert(FIELD_FLOAT);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_VECTOR) {
            vec1.Init(left.Float(),left.Float(),left.Float());
            left.SetVector3D(vec1);
        }
        else if (right.FieldType() == FIELD_FLOAT && left.FieldType() == FIELD_VECTOR) {
            vec2.Init(right.Float(),right.Float(),right.Float());
            right.SetVector3D(vec2);
        }
        if (m_LastOp == ADD) {
            if (left.FieldType() == FIELD_VECTOR) {
                result.SetVector3D(vec1+vec2);
            }
            else if (left.FieldType() == FIELD_FLOAT) {
                result.SetFloat(left.Float() + right.Float());
            }
            else if (left.FieldType() == FIELD_INTEGER) {
                result.SetInt(left.Int() + right.Int());
            }
        }
        else if (m_LastOp == SUBTRACT) {
            if (left.FieldType() == FIELD_VECTOR) {
                result.SetVector3D(vec1-vec2);
            }
            else if (left.FieldType() == FIELD_FLOAT) {
                result.SetFloat(left.Float() - right.Float());
            }
            else if (left.FieldType() == FIELD_INTEGER) {
                result.SetInt(left.Int() - right.Int());
            }
        }
        else if (m_LastOp == MULTIPLY) {
            if (left.FieldType() == FIELD_VECTOR) {
                result.SetVector3D(vec1 * vec2);
            }
            else if (left.FieldType() == FIELD_FLOAT) {
                result.SetFloat(left.Float() * right.Float());
            }
            else if (left.FieldType() == FIELD_INTEGER) {
                result.SetInt(left.Int() * right.Int());
            }
        }
        else if (m_LastOp == DIVIDE) {
            if (left.FieldType() == FIELD_VECTOR) {
                result.SetVector3D(vec1 / vec2);
            }
            else if (left.FieldType() == FIELD_FLOAT) {
                result.SetFloat(left.Float() / right.Float());
            }
            else if (left.FieldType() == FIELD_INTEGER) {
                result.SetInt(left.Int() / right.Int());
            }
        }
        else if (m_LastOp == REMINDER) {
            left.Convert(FIELD_INTEGER);
            right.Convert(FIELD_INTEGER);
            result.SetInt(left.Int() % right.Int());
        }
        else if (m_LastOp == LESS_THAN) {
            result.SetInt(left.FieldType() == FIELD_FLOAT ? left.Float() < right.Float() : left.Int() < right.Int());
        }
        else if (m_LastOp == LESS_THAN_OR_EQUAL) {
            result.SetInt(left.FieldType() == FIELD_FLOAT ? left.Float() <= right.Float() : left.Int() <= right.Int());
        }
        else if (m_LastOp == GREATER_THAN) {
            result.SetInt(left.FieldType() == FIELD_FLOAT ? left.Float() > right.Float() : left.Int() > right.Int());
        }
        else if (m_LastOp == GREATER_THAN_OR_EQUAL) {
            result.SetInt(left.FieldType() == FIELD_FLOAT ? left.Float() >= right.Float() : left.Int() >= right.Int());
        }
        else if (m_LastOp == EQUAL) {
            if (left.FieldType() == FIELD_INTEGER) {
                result.SetInt(left.Float() == right.Float());
            }
            else if (left.FieldType() == FIELD_FLOAT) {
                result.SetInt(left.Float() == right.Float());
            }
            else if (left.FieldType() == FIELD_STRING) {
                result.SetInt(strcmp(left.String(), right.String()) == 0);
            }
            else if (left.FieldType() == FIELD_VECTOR) {
                result.SetInt(vec1 == vec2);
            }
            else if (left.FieldType() == FIELD_EHANDLE) {
                result.SetInt(left.Entity() == right.Entity());
            }
        }
        else if (m_LastOp == NOT_EQUAL) {
            if (left.FieldType() == FIELD_INTEGER) {
                result.SetInt(left.Int() == right.Int());
            }
            else if (left.FieldType() == FIELD_FLOAT) {
                result.SetInt(left.Float() != right.Float());
            }
            else if (left.FieldType() == FIELD_STRING) {
                result.SetInt(strcmp(left.String(), right.String()) != 0);
            }
            else if (left.FieldType() == FIELD_VECTOR) {
                result.SetInt(vec1 != vec2);
            }
            else if (left.FieldType() == FIELD_EHANDLE) {
                result.SetInt(left.Entity() == right.Entity());
            }
        }
        // Logical and bitwise operators below, others above this line
        else {
            if (left.FieldType() != FIELD_INTEGER) {
                left.Convert(FIELD_INTEGER);
            }
            if (right.FieldType() != FIELD_INTEGER) {
                right.Convert(FIELD_INTEGER);
            }
            if (m_LastOp == AND) {
                result.SetInt(left.Int() && right.Int());
            }
            else if (m_LastOp == OR) {
                result.SetInt(left.Int() || right.Int());
            }
            else if (m_LastOp == AND_BITWISE) {
                result.SetInt(left.Int() & right.Int());
            }
            else if (m_LastOp == OR_BITWISE) {
                result.SetInt(left.Int() | right.Int());
            }
            else if (m_LastOp == XOR_BITWISE) {
                result.SetInt(left.Int() ^ right.Int());
            }
            else if (m_LastOp == LEFT_SHIFT) {
                result.SetInt(left.Int() << right.Int());
            }
            else if (m_LastOp == RIGHT_SHIFT) {
                result.SetInt(left.Int() >> right.Int());
            }
        }
    }
    //Msg("Valuestep: %d %s\n", m_LastOp, result.String());
}

void Evaluation::ParseOp(Op op) 
{
    // If minus/plus sign and empty token, it is a negative/positive value sign
    if (m_pCurrentChar == m_strToken && m_bParseToken) {
        if (op == ADD) {
            ParseChar('+');
            return;
        }
        else if (op == SUBTRACT) {
            ParseChar('-');
            return;
        }
    }

    int priorityPrev = s_OpPriority[(int)m_LastOp];
    int priorityNew = s_OpPriority[(int)op];
    if (priorityNew > priorityPrev) {
        for (int i = 0; i < priorityNew - priorityPrev; i++) {
            PushStack();
        }
    }
    else if (priorityNew < priorityPrev) {
        for (int i = 0; i < priorityPrev - priorityNew; i++) {
            DoOp();
            PopStack();
        }
    }

    DoOp();
    m_LastOp = op;
}

CON_COMMAND_F(sig_expression_test, "Test of expressions", FCVAR_NONE)
{
	variant_t value;
    variant_t value2;
	Evaluation expr(value);
    {
        TIME_SCOPE2(EvalTime)
	    expr.Evaluate(args[1], nullptr, nullptr, nullptr, value2);
    }
	Msg("Value: %s\n ", value.String());
}

CON_COMMAND_F(sig_expression_functions, "List of expression functions", FCVAR_NONE)
{
	for (auto &func : expression_functions) {
        Msg("%s(", func.name);
        bool first = true;
	    for (auto &param : func.paramNames) {
            if (!first) {
                Msg(";");
            }
            first = false;
            Msg(" %s", param);
        }
        Msg(" )\n");
    }
}
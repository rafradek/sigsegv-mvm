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
    bool SetEntityVariable(CBaseEntity *entity, GetInputType type, const char *name, variant_t &variable);
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
        UTIL_StringToVectorAlt(vec, str+1);
        //sscanf(str, "[%f %f %f]", &vec.x, &vec.y, &vec.z);
        value.SetVector3D(vec);
        return true;
    }
    // Parse as int
    else if (!asfloat && *ParseToInt(str, valint) == '\0') {
       // Msg("Parse float\n");
        value.SetInt(valint);
        return true;
    }
    // Parse as float
    else if (ParseToFloat(str, val) != str) {
       // Msg("Parse float\n");
        value.SetFloat(val);
        return true;
    }
    return false;
}

void Evaluation::AddFunction(std::string name, ExpressionFunction function, std::vector<std::string> args, std::vector<const char *> args_optional)
{
    Evaluation::EvaluationFuncDef func;
    func.name = name;
    func.function = function;
    func.paramCount = args.size();
    func.paramNames = args;
    func.paramNamesOptional = args_optional;
    expression_functions.push_back(func);
}

const std::vector<Evaluation::EvaluationFuncDef> &Evaluation::GetFunctionList()
{
    return expression_functions;
}

void Evaluation::Evaluate(const char *expression, CBaseEntity *self, CBaseEntity *activator, CBaseEntity *caller, variant_t &param)
{   
    //m_strToken.reserve(256);
    m_activator = activator;
    m_caller = caller;
    m_param = param;
    m_self = self;
    m_pExpression = expression;
    m_CurStack.result = variant_t();
    char c;
    char specialChar = '\0';

    while((c = (*m_pExpression++)) != '\0') {
        if (m_bParsingString) {
            if (c == '\'') {
                if  (*(m_strToken.end()-1) != '\\') {
                    m_bParsingString = false;
                    continue;
                }
                else {
                    m_strToken.pop_back();
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
                    else if (!m_strToken.empty() && !m_bParsingVector && m_strToken.front() != '+' && m_strToken.front() != '-' && !(m_strToken.front() >= '0' && m_strToken.front() <= '9')) {
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
                        ParseOp(ASSIGN);
                    }
                }
            }
            switch (c) {
                case ' ': case '\t': case '\n': case '\r': ParseWhitespace(); break;
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
                case '>': case '<': case '&': case '|': case '.': case '!': case '=': specialChar = c; break;
                case '{': ParseBraceLeft(); break;
                case '}': ParseBraceRight(); break;
                case ';': ParseSemicolon(); break;
                case '^': ParseOp(XOR_BITWISE); break;
                case '%': ParseOp(REMAINDER); break;
                default : ParseChar(c);
            }
        }
    }
    ParseOp(INVALID);
    m_Result = m_CurStack.result;
}

void Evaluation::ParseParenthesisLeft()
{
    if (!m_strToken.empty()) {
        ParseToken();
    }
    if (!m_bParseFunction) {
        PushStack();
    }
    else {
        m_bParseFunctionParam = true;
    }
}

void Evaluation::ParseParam()
{
    if (m_bParseFunction) {
        if (!m_strToken.empty()) {
            ParseToken();
        }
        return;
    }
    ParseOp(INVALID);
    PopStack();
    if (m_CurStack.func != nullptr) {
        m_CurStack.func->params.push_back(m_curValue);
    }
    m_curValue = variant_t();
    PushStack();
}

Evaluation::EvaluationFuncDef *Evaluation::FindFunction(std::string &name)
{
        for (auto &pair : expression_functions) {
            if (pair.name == name) {
                return &pair;
            }
        }
        for (auto &pair : m_Functions) {
            if(pair.name == name) {
                return &pair;
            }
        }
    //ClientMsgAll("Script Error: Function %s not found\n", func->func);
    return nullptr;
}

void Evaluation::ExecuteFunction(EvaluationFunc *func)
{
    auto def = func->func;
    if (def->paramCount > func->params.size()) {
        ClientMsgAll("Script Error: Function %s called with %d parameters, need %d:", def->name, func->params.size(), def->paramCount);
        for (size_t i = 0; i < def->paramCount; i++) {
            ClientMsgAll(", %s", def->paramNames[i]);
        }
        ClientMsgAll("\n");
        return;
    }
    if (def->expr == nullptr) {
        (*def->function)(def->name.c_str(), func->params, func->params.size(), m_curValue);
    }
    else {
        PushStack();
        m_CurStack.returnPos = m_pExpression;
        m_pExpression = def->expr;
        for (size_t i = 0; i < def->paramCount; i++) {
            m_Variables.push_back({def->paramNames[i], func->params[i]});
        }
    }
}

bool Evaluation::FindVariable(std::string &name, variant_t &value)
{
    for (auto &pair : m_Variables) {
        if (pair.first == name) {
            value = pair.second;
            return true;
        }
    }
    return false;
}

bool Evaluation::SetVariable(std::string &name, variant_t &value)
{
    for (auto &pair : m_Variables) {
        if (pair.first == name) {
            pair.second = value;
            return true;
        }
    }
    m_Variables.push_back({name, value});
    return false;
}

void Evaluation::ParseParenthesisRight()
{
    if (m_bParseFunction) {
        if (!m_strToken.empty()) {
            ParseToken();
        }
        return;
    }
    ParseOp(INVALID);
    PopStack();
    if (m_CurStack.func != nullptr) {
       // Msg("Function %s\n", m_func->func);
        m_CurStack.func->params.push_back(m_curValue);

        ExecuteFunction(m_CurStack.func);
        delete m_CurStack.func;
        m_CurStack.func = nullptr;
    }
    if (m_CurStack.declMode == IF) {
        if (m_curValue.Int()) {
            m_CurStack.declMode = IF_AFTER_TRUE;
            PushStack();
            m_CurStack.popStackOnEnd = true;
        }
        else {
            m_CurStack.declMode = IF_AFTER_FALSE;
            SkipNextCodeBlock();
        }
    }
    m_bParseToken = false;
}

void Evaluation::ParseBraceLeft()
{
    if (m_CurStack.declMode == IF_AFTER_TRUE) {

    }
    else if (m_CurStack.declMode == ELSE_FALSE) {
        m_CurStack.declMode = NORMAL;
    }
    if (m_bParseFunction) {
        m_DefFunc.expr = m_pExpression;
        m_Functions.push_back(m_DefFunc);
        m_DefFunc = EvaluationFuncDef();
        SkipChars('{', '}');
        m_bParseFunction = false;
        m_bParseFunctionParam = false;
        //Msg("Parse end fuction %s\n", m_pExpression);
    }
    else {
        PushStack();
    }
}

void Evaluation::ParseBraceRight()
{
    ParseOp(INVALID);
    if (m_CurStack.returnPos != nullptr) {
        m_pExpression = m_CurStack.returnPos;
        m_bParseToken = false;
        //Msg("Execution funcion end go to %s %s\n", m_pExpression, result.String());
    }
    PopStack();
}

void Evaluation::ParseSemicolon()
{
    ParseOp(INVALID);
    if (!m_bReturning) {
        m_curValue = variant_t();
        m_CurStack.result = variant_t();
    }
    m_bReturning = false;
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
// Skip chars inside from and to scope
void Evaluation::SkipNextCodeBlock()
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
            if (c == '{') {
                scope++;
            }
            else if (c == '}') {
                scope--;
                if (scope <= 0) {
                    return;
                }
            }
            else if (c == ';' && scope == 0) {
                return;
            }
        }
    }
}

void Evaluation::PushStack()
{
    //Msg("Push Stack\n");
    m_Stack.push_back(m_CurStack);
    m_CurStack = EvaluationStack();
    //if (!m_strToken.empty()) {
    //    m_strToken.clear();
    //}
    //m_bParseToken = true;
}

void Evaluation::PopStack()
{
    //Msg("Pop Stack\n");
    if (m_Stack.empty()) {
        ClientMsgAll("Script Error: syntax error, ( expected, got ) or ;\n");
        return;
    }
    //Msg("Result inside %s\n", result.String());
    if (m_CurStack.func != nullptr) {
        delete m_CurStack.func;
    }
    if (m_CurStack.varNames != nullptr) {
        delete m_CurStack.varNames;
    }
    m_curValue = m_CurStack.result;
    m_CurStack = m_Stack.back();
    m_Stack.pop_back();
    //if (!m_strToken.empty()) {
    //    m_strToken.clear();
    //}
    //if (m_pReturnPos != nullptr)
        //Msg("Result restore %s\n", m_pReturnPos);
    //m_bParseToken = false;
}

void Evaluation::ParseArrayBracket()
{
    if (m_strToken.empty()) {
        m_bParsingVector = true;
    }
    ParseChar('[');
}

void Evaluation::ParseQuote()
{
    if (m_strToken.empty()) {
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
    m_strToken.push_back(c);
}

void Evaluation::ParseWhitespace() 
{
    if (m_bParsingVector) {
        ParseChar(' ');
    }
    // Parse token
    else if (!m_strToken.empty()) {
        ParseToken();
    }
}
#define RAD2DEG2  ( (float)(180.f / M_PI_F) )
#define DEG2RAD2  ( (float)(M_PI_F / 180.f) )

void Evaluation::ParseToken()
{
    m_bTokenParsed = true;
    bool asfloat = m_bParsingFloat;
    m_bParsingFloat = false;
   // Msg("Token %s %d\n", m_strToken.c_str(), m_bParseToken);
    //if (m_bParseToken) {
        // Parse as vector
    if (m_bWasString) {
        m_curValue.SetString(AllocPooledString(m_strToken.c_str()));
        m_bWasString = false;
    }
    // Reserved tokens
    else if (m_strToken == "function"sv) {
        m_bParseFunction = true;
    }
    else if (m_strToken == "var"sv) {
        m_bDeclaringVariable = true;
    }
    else if (m_strToken == "if"sv) {
        m_CurStack.declMode = IF;
    }
    else if (m_strToken == "else"sv) {
        if (m_CurStack.declMode == IF_AFTER_TRUE) {
            SkipNextCodeBlock();
            m_CurStack.declMode = NORMAL;
        }
        else {
            m_CurStack.declMode = ELSE_FALSE;
            PushStack();
            m_CurStack.popStackOnEnd = true;
        }
    }
    else if (m_strToken == "return"sv) {
        m_CurStack.declMode = RETURN;
    }
    else if (ParseNumberOrVectorFromStringStrict(m_strToken.c_str(), m_curValue, asfloat)) {

    }
    else if (m_strToken == "pi"sv) {
        m_curValue.SetFloat(M_PI_F);
    }
    else if (m_strToken == "degtorad"sv) {
        m_curValue.SetFloat(DEG2RAD2);
    }
    else if (m_strToken == "radtodeg"sv) {
        m_curValue.SetFloat(RAD2DEG2);
    }
    else if (m_strToken == "true"sv) {
        m_curValue.SetInt(1);
    }
    else if (m_strToken == "false"sv) {
        m_curValue.SetInt(0);
    }
    else if (m_strToken == "null"sv) {
        m_curValue.SetEntity(nullptr);
    }
    else if (m_strToken == "!param"sv) {
        m_curValue = m_param;
    }
    else if (m_bParseFunction) {
        if (!m_bParseFunctionParam) {
            m_DefFunc.name = m_strToken;
        }
        else {
            m_DefFunc.paramNames.push_back(m_strToken);
            m_DefFunc.paramCount = m_DefFunc.paramNames.size();
        }
    }
    else {
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
        if (!m_bDeclaringVariable) {
            if (m_CurStack.op != ACCESS) {
                if (!FindVariable(m_strToken, m_curValue)) {
                    auto function = FindFunction(m_strToken);
                    if (function != nullptr) {
                        m_CurStack.func = new EvaluationFunc {function};
                    }
                    else {
                        CHandle<CBaseEntity> targetEnt = servertools->FindEntityGeneric(nullptr, m_strToken.c_str(), m_self, m_activator, m_caller);
                        if (targetEnt != nullptr) {
                            m_curValue.Set(FIELD_EHANDLE, &targetEnt);
                        }
                        else {
                            m_curValue = variant_t();
                        }
                        //Msg("Not found Variable\n");
                    }
                }
            }
            else {
                m_curValue.SetString(AllocPooledString(m_strToken.c_str()));
            }
            //Msg("Val %s\n", m_curValue.String());
        }
        m_bDeclaringVariable = false;
        if (m_CurStack.func == nullptr) {
            if (m_CurStack.varNames == nullptr || m_CurStack.op != ACCESS) {
                m_CurStack.varNames = new std::deque<std::string>();
            }
            m_CurStack.varNames->push_back(m_strToken);
        }
    }
    //}
    m_bWasString = false;
    m_strToken.clear();
    m_bParseToken = true;
}

void Evaluation::DoOp()
{
    //result = m_curValue;
    auto &result = m_CurStack.result;
    if ((!m_strToken.empty() || m_bWasString)) {
        //TIME_SCOPE2(ParseToken)
        ParseToken();
    }

    if (result.FieldType() == FIELD_VOID) {
        result = m_curValue;
    }

    m_bTokenParsed = false;

   // Msg("DoOp %d\n", m_bParseToken);
    m_bParseToken = true;
    if (m_CurStack.op == INVALID) return;
    int op = m_CurStack.op;

    variant_t& left = result;
    variant_t& right = m_curValue;

    if (op == ACCESS) {
        if (left.FieldType() != FIELD_EHANDLE) {
            left.Convert(FIELD_EHANDLE);
        }
        CBaseEntity *entity = left.Entity();
        if (entity == nullptr || !GetEntityVariable(entity, Mod::Etc::Mapentity_Additions::ANY, right.String(), result)) {
            result = variant_t();
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
        return;
    }
    else if (op == ASSIGN) {
        if (m_CurStack.varNames == nullptr) {
            return;
        }
        auto &names = *m_CurStack.varNames;
        int size = names.size();
        
        //Msg("Assign size %d\n", size);
        if (size == 1) {
            //Msg("Set variable %s\n", names[0].c_str());
            SetVariable(names[0],right);
        }
        else {
            variant_t variant;
            //Msg("Multi variable %s %s\n", names[0].c_str(), names[1].c_str());
            std::vector<CBaseEntity *> entitiesToApply;
            if (FindVariable(names[0], variant) && variant.FieldType() == FIELD_EHANDLE) {
                entitiesToApply.push_back(variant.Entity());
            }
            else {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, names[0].c_str(), m_self, m_activator, m_caller)) != nullptr ;) {
                    entitiesToApply.push_back(target);
                }
            }
            for (auto ent : entitiesToApply) {
                if (ent == nullptr) continue;
                for(int i = 1; i < size - 1; i++) {
                    if (GetEntityVariable(ent, Mod::Etc::Mapentity_Additions::ANY, names[i].c_str(), variant)) {
                        ent = variant.Entity();
                        if (ent == nullptr) break;
                    }
                }
                if (ent != nullptr) {
                    SetEntityVariable(ent, Mod::Etc::Mapentity_Additions::ANY, names[size-1].c_str(), right);
                }
            }
        }
        result = right;
        delete m_CurStack.varNames;
        m_CurStack.varNames = nullptr;
        return;
    }
    else if (m_CurStack.varNames != nullptr) {
        delete m_CurStack.varNames;
        m_CurStack.varNames = nullptr;
    }

    if (op == CONCAT) {
        char buf[512];
        strncpy(buf, left.String(), sizeof(buf));
        V_strncat(buf, right.String(), sizeof(buf));
        result.SetString(AllocPooledString(buf));
        return;
    }

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
    if (op == ADD) {
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
    else if (op == SUBTRACT) {
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
    else if (op == MULTIPLY) {
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
    else if (op == DIVIDE) {
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
    else if (op == REMAINDER) {
        left.Convert(FIELD_INTEGER);
        right.Convert(FIELD_INTEGER);
        result.SetInt(left.Int() % right.Int());
    }
    else if (op == LESS_THAN) {
        result.SetInt(left.FieldType() == FIELD_FLOAT ? left.Float() < right.Float() : left.Int() < right.Int());
    }
    else if (op == LESS_THAN_OR_EQUAL) {
        result.SetInt(left.FieldType() == FIELD_FLOAT ? left.Float() <= right.Float() : left.Int() <= right.Int());
    }
    else if (op == GREATER_THAN) {
        result.SetInt(left.FieldType() == FIELD_FLOAT ? left.Float() > right.Float() : left.Int() > right.Int());
    }
    else if (op == GREATER_THAN_OR_EQUAL) {
        result.SetInt(left.FieldType() == FIELD_FLOAT ? left.Float() >= right.Float() : left.Int() >= right.Int());
    }
    else if (op == EQUAL) {
        if (left.FieldType() == FIELD_INTEGER) {
            result.SetInt(left.Int() == right.Int());
        }
        else if (left.FieldType() == FIELD_FLOAT) {
            result.SetInt(left.Float() == right.Float());
        }
        else if (left.FieldType() == FIELD_STRING) {
            result.SetInt(left.String() == right.String());
        }
        else if (left.FieldType() == FIELD_VECTOR) {
            result.SetInt(vec1 == vec2);
        }
        else if (left.FieldType() == FIELD_EHANDLE) {
            result.SetInt(left.Entity() == right.Entity());
        }
    }
    else if (op == NOT_EQUAL) {
        if (left.FieldType() == FIELD_INTEGER) {
            result.SetInt(left.Int() == right.Int());
        }
        else if (left.FieldType() == FIELD_FLOAT) {
            result.SetInt(left.Float() != right.Float());
        }
        else if (left.FieldType() == FIELD_STRING) {
            result.SetInt(left.String() != right.String());
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
        if (op == AND) {
            result.SetInt(left.Int() && right.Int());
        }
        else if (op == OR) {
            result.SetInt(left.Int() || right.Int());
        }
        else if (op == AND_BITWISE) {
            result.SetInt(left.Int() & right.Int());
        }
        else if (op == OR_BITWISE) {
            result.SetInt(left.Int() | right.Int());
        }
        else if (op == XOR_BITWISE) {
            result.SetInt(left.Int() ^ right.Int());
        }
        else if (op == LEFT_SHIFT) {
            result.SetInt(left.Int() << right.Int());
        }
        else if (op == RIGHT_SHIFT) {
            result.SetInt(left.Int() >> right.Int());
        }
    }
    //Msg("Valuestep: %d %s\n", m_LastOp, result.String());
}

void Evaluation::ParseOp(Op op) 
{
    // If minus/plus sign and empty token, it is a negative/positive value sign
    if ((m_strToken.empty() && !m_bTokenParsed && m_bParseToken) || m_bParsingVector) {
        if (op == ADD) {
            ParseChar('+');
            return;
        }
        else if (op == SUBTRACT) {
            ParseChar('-');
            return;
        }
    }
    
    int priorityPrev = s_OpPriority[(int)m_CurStack.op];
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
    m_CurStack.op = op;
    if (op == ASSIGN) {
        PushStack();
        m_CurStack.popStackOnEnd = true;
    }
    if (op == INVALID) {
        if (m_CurStack.declMode == RETURN) {
            variant_t retValue = m_CurStack.result;
            while (!m_CurStack.isFunction && m_Stack.size() > 0) {
                PopStack();
            }
            m_bReturning = true;
            if (m_Stack.size() == 0) {
                while(*m_pExpression != '\0') {
                    m_pExpression++;
                }
                m_CurStack.result = retValue;
            }
            else {
                if (m_CurStack.returnPos != nullptr) {
                    m_pExpression = m_CurStack.returnPos;
                }
                PopStack();
                m_curValue = retValue;
            }
        }
        if (m_CurStack.popStackOnEnd) {
            PopStack();
            ParseOp(INVALID);
        }
    }
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
        Msg("%s(", func.name.c_str());
        bool first = true;
	    for (auto &param : func.paramNames) {
            if (!first) {
                Msg(",");
            }
            first = false;
            Msg(" %s", param.c_str());
        }
        Msg(" )\n");
    }
}
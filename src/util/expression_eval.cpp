#include "stub/misc.h"
#include "util/expression_eval.h"
#include "util/misc.h"

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

void Evaluation::Evaluate(const char *expression)
{
    TIME_SCOPE2(EvalTime)
    result.Convert(FIELD_VOID);
    m_pCurrentChar = m_strToken;
    *m_strToken = '\0';
    char c;
    while((c = (*expression++)) != '\0') {
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
            ParseChar(c);
        }
        else {
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
                default : ParseChar(c);
            }
        }
    }
    ParseOp(INVALID);
}

void Evaluation::ParseParenthesisLeft()
{
    if (m_pCurrentChar == m_strToken) {
        m_OpStack.push_back(m_LastOp);
        m_LastOp = INVALID;
        m_ResultStack.push_back(result);
        result.Convert(FIELD_VOID);
    }
}

void Evaluation::ParseParenthesisRight()
{
    ParseOp(INVALID);
    Msg("Result inside %s\n", result.String());
    m_LastOp = m_OpStack.back();
    m_OpStack.pop_back();
    m_curValue = result;
    result = m_ResultStack.back();
    m_ResultStack.pop_back();
    Msg("Result restore %d %s\n", m_LastOp, result.String());
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
}

void Evaluation::ParseToken(char *token, variant_t &value)
{
    float val;
    Msg("Token %s\n", token);
    // Parse as vector
    if (token[0] == '[') {
        Msg("Parse vector\n");
        Vector vec;
        sscanf(token, "[%f %f %f]", &vec.x, &vec.y, &vec.z);
        value.SetVector3D(vec);
    }
    // Parse as float
    else if (StringToFloatStrict(token, val)) {
        Msg("Parse float\n");
        value.SetFloat(val);
    }
    else {
        // Parse literal string
        if (m_bWasString) {
            Msg("Parse literal string\n");
            value.SetString(AllocPooledString(token));
            m_bWasString = false;
            return;
        }
        // Parse entity prop
        char *prop = strchr(token, '.');
        Msg("Parse prop %d\n", prop);
        if (prop == nullptr) {
            value.Convert(FIELD_VOID);
            return;
        }
        // Separate entity and prop strings
        *(prop++) = '\0';
        Msg("Parse target %s %s\n", token, prop);

        CBaseEntity *targetEnt = servertools->FindEntityGeneric(nullptr, token);

        if (targetEnt == nullptr || !GetEntityVariable(targetEnt, Mod::Etc::Mapentity_Additions::ANY, prop, value)) {
            value.Convert(FIELD_VOID);
            return;
        }
        if (value.FieldType() == FIELD_INTEGER) {
            value.SetFloat((float)value.Int());
        }
    }
    m_bWasString = false;
}

void Evaluation::DoOp()
{
    
    TIME_SCOPE2(EvalTime)
    //result = m_curValue;
    *m_pCurrentChar = '\0';
    if (m_bParseToken) {
        ParseToken(m_strToken, m_curValue);
    }
    else {
        m_bParseToken = true;
    }

    if (result.FieldType() == FIELD_VOID) {
        result = m_curValue;
    }

    variant_t& left = result;
    variant_t& right = m_curValue;
    if (m_LastOp == ADD) {
        if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_VECTOR) {
            Vector vec1;
            left.Vector3D(vec1);
            Vector vec2;
            right.Vector3D(vec2);
            result.SetVector3D(vec1+vec2);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_FLOAT) {
            result.SetFloat(left.Float() + right.Float());
        }
    }
    else if (m_LastOp == SUBTRACT) {
        if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_VECTOR) {
            Vector vec1;
            left.Vector3D(vec1);
            Vector vec2;
            right.Vector3D(vec2);
            result.SetVector3D(vec1-vec2);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_FLOAT) {
            result.SetFloat(left.Float() - right.Float());
        }
    }
    else if (m_LastOp == MULTIPLY) {
        if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_VECTOR) {
            Vector vec1;
            left.Vector3D(vec1);
            Vector vec2;
            right.Vector3D(vec2);
            result.SetVector3D(vec1 * vec2);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_FLOAT) {
            result.SetFloat(left.Float() * right.Float());
        }
    }
    else if (m_LastOp == DIVIDE) {
        if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_VECTOR) {
            Vector vec1;
            left.Vector3D(vec1);
            Vector vec2;
            right.Vector3D(vec2);
            result.SetVector3D(vec1 / vec2);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_FLOAT) {
            result.SetFloat(left.Float() / right.Float());
        }
    }
    Msg("Valuestep: %d %s\n", m_LastOp, result.String());
    m_pCurrentChar = m_strToken;
    *m_strToken = '\0';
}

void Evaluation::ParseOp(Op op) 
{
    int priorityPrev = s_OpPriority[(int)m_LastOp];
    int priorityNew = s_OpPriority[(int)op];
    if (priorityNew > priorityPrev) {
        m_OpStack.push_back(m_LastOp);
        m_LastOp = INVALID;
        m_ResultStack.push_back(result);
        result.Convert(FIELD_VOID);
    }
    else if (priorityNew < priorityPrev) {
        DoOp();
        m_LastOp = m_OpStack.back();
        m_OpStack.pop_back();
        m_curValue = result;
        result = m_ResultStack.back();
        m_ResultStack.pop_back();
        m_bParseToken = false;
    }

    DoOp();
    m_LastOp = op;
}
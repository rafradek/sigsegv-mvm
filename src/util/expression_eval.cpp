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
        PushStack();
    }
}

void Evaluation::ParseParenthesisRight()
{
    ParseOp(INVALID);
    PopStack();
}

void Evaluation::PushStack()
{

    m_OpStack.push_back(m_LastOp);
    m_LastOp = INVALID;
    m_ResultStack.push_back(result);
    result.Convert(FIELD_VOID);
}

void Evaluation::PopStack()
{
    //Msg("Result inside %s\n", result.String());
    m_LastOp = m_OpStack.back();
    m_OpStack.pop_back();
    m_curValue = result;
    result = m_ResultStack.back();
    m_ResultStack.pop_back();
    //Msg("Result restore %d %s\n", m_LastOp, result.String());
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
    //Msg("Token %s\n", token);
    // Parse as vector
    if (token[0] == '[') {
        //Msg("Parse vector\n");
        Vector vec;
        sscanf(token, "[%f %f %f]", &vec.x, &vec.y, &vec.z);
        value.SetVector3D(vec);
    }
    // Parse as float
    else if (StringToFloatStrict(token, val)) {
       // Msg("Parse float\n");
        value.SetFloat(val);
    }
    else {
        // Parse literal string
        if (m_bWasString) {
            //Msg("Parse literal string\n");
            value.SetString(AllocPooledString(token));
            m_bWasString = false;
            return;
        }
        // Parse entity prop
        char *prop = strchr(token, '.');
        //Msg("Parse prop %d\n", prop);
        if (prop == nullptr) {
            value.Convert(FIELD_VOID);
            return;
        }
        // Separate entity and prop strings
        *(prop++) = '\0';
        //Msg("Parse target %s %s\n", token, prop);

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
    //result = m_curValue;
    *m_pCurrentChar = '\0';
    if (m_bParseToken) {
        TIME_SCOPE2(ParseToken)
        ParseToken(m_strToken, m_curValue);
    }
    else {
        m_bParseToken = true;
    }

    TIME_SCOPE2(DoOp)
    if (result.FieldType() == FIELD_VOID) {
        result = m_curValue;
    }

    variant_t& left = result;
    variant_t& right = m_curValue;
    Vector vec1;
    Vector vec2;
    left.Vector3D(vec1);
    right.Vector3D(vec2);
    if (m_LastOp == ADD) {
        if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_VECTOR) {
            result.SetVector3D(vec1+vec2);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_FLOAT) {
            result.SetFloat(left.Float() + right.Float());
        }
        else if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_FLOAT) {
            vec1 += right.Float();
            result.SetVector3D(vec1);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_VECTOR) {
            vec2 += left.Float();
            result.SetVector3D(vec2);
        }
    }
    else if (m_LastOp == SUBTRACT) {
        if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_VECTOR) {
            result.SetVector3D(vec1-vec2);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_FLOAT) {
            result.SetFloat(left.Float() - right.Float());
        }
        else if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_FLOAT) {
            vec1 -= right.Float();
            result.SetVector3D(vec1);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_VECTOR) {
            vec2 -= left.Float();
            result.SetVector3D(vec2);
        }
    }
    else if (m_LastOp == MULTIPLY) {
        if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_VECTOR) {
            result.SetVector3D(vec1 * vec2);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_FLOAT) {
            result.SetFloat(left.Float() * right.Float());
        }
        else if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_FLOAT) {
            vec1 *= right.Float();
            result.SetVector3D(vec1);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_VECTOR) {
            vec2 *= left.Float();
            result.SetVector3D(vec2);
        }
    }
    else if (m_LastOp == DIVIDE) {
        if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_VECTOR) {
            result.SetVector3D(vec1 / vec2);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_FLOAT) {
            result.SetFloat(left.Float() / right.Float());
        }
        else if (left.FieldType() == FIELD_VECTOR && right.FieldType() == FIELD_FLOAT) {
            vec1 /= right.Float();
            result.SetVector3D(vec1);
        }
        else if (left.FieldType() == FIELD_FLOAT && right.FieldType() == FIELD_VECTOR) {
            vec2 /= left.Float();
            result.SetVector3D(vec2);
        }
    }
    //Msg("Valuestep: %d %s\n", m_LastOp, result.String());
    m_pCurrentChar = m_strToken;
    *m_strToken = '\0';
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
        PushStack();
    }
    else if (priorityNew < priorityPrev) {
        DoOp();
        PopStack();
    }

    DoOp();
    m_LastOp = op;
}

CON_COMMAND_F(sig_expression_test, "Test of expressions", FCVAR_NONE)
{
	variant_t value;
	Evaluation expr(value);
    {
        TIME_SCOPE2(EvalTime)
	    expr.Evaluate(args[1]);
    }
	Msg("Value: %s\n ", value.String());
}

#include "stub/misc.h"
#include "stub/econ.h"
#include "stub/tfweaponbase.h"
#include "mod/pop/common.h"
#include "util/iterate.h"
#include "util/expression_eval.h"
#include "util/pooled_string.h"
#include "util/misc.h"

void FunctionTest(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (param_count < 3) {
        result = variant_t();
        return;
    }
    if (params[0].FieldType() != FIELD_INTEGER) {
        params[0].Convert(FIELD_INTEGER);
    }

    result = params[params[0].Int() != 0 ? 1 : 2];
}

void FunctionTestExists(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    result.SetInt(params[0].FieldType() == FIELD_VOID || (params[0].FieldType() == FIELD_EHANDLE && params[0].Entity() == nullptr) ? 0 : 1);
}

void FunctionLength(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR && !params[0].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    Vector vec;
    params[0].Vector3D(vec);
    result.SetFloat(vec.Length());
}
void FunctionDistance(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR && !params[0].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    if (params[1].FieldType() != FIELD_VECTOR && !params[1].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    Vector vec1;
    params[0].Vector3D(vec1);
    Vector vec2;
    params[1].Vector3D(vec2);
    result.SetFloat(vec1.DistTo(vec2));
}
void FunctionMinMax(const char *function, Evaluation::Params &params, int param_count, variant_t& result, bool max)
{
    if (params[0].FieldType() == FIELD_STRING) {
        ParseNumberOrVectorFromString(params[0].String(),params[0]);
    }
    if (params[1].FieldType() == FIELD_STRING) {
        ParseNumberOrVectorFromString(params[1].String(),params[1]);
    }
    if (params[0].FieldType() == FIELD_FLOAT) {
        if (params[1].FieldType() != FIELD_FLOAT)
            params[1].Convert(FIELD_FLOAT);

        result = params[(params[0].Float() > params[1].Float()) != max ? 1 : 0];
    }
    else if (params[0].FieldType() == FIELD_INTEGER) {
        if (params[1].FieldType() != FIELD_INTEGER)
            params[1].Convert(FIELD_INTEGER);
        
        result = params[(params[0].Int() > params[1].Int()) != max ? 1 : 0];
    }
    else if (params[0].FieldType() == FIELD_VECTOR) {
        if (params[1].FieldType() != FIELD_VECTOR)
            params[1].Convert(FIELD_VECTOR);
        Vector left; 
        Vector right; 
        params[0].Vector3D(left);
        params[1].Vector3D(right);

        result = params[(left.LengthSqr() > right.LengthSqr()) != max ? 1 : 0];
    }
    else {
        result = params[0];
    }
}

void FunctionMin(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    FunctionMinMax(function, params, param_count, result, false);
}

void FunctionMax(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    FunctionMinMax(function, params, param_count, result, true);
}

void FunctionDot(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR && !params[0].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    if (params[1].FieldType() != FIELD_VECTOR && !params[1].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    Vector vec1;
    params[0].Vector3D(vec1);
    Vector vec2;
    params[1].Vector3D(vec2);
    result.SetFloat(DotProduct(vec1, vec2));
}

void FunctionCross(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR && !params[0].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    if (params[1].FieldType() != FIELD_VECTOR && !params[1].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    Vector vec1;
    params[0].Vector3D(vec1);
    Vector vec2;
    params[1].Vector3D(vec2);
    Vector out;
    CrossProduct(vec1, vec2, out);
    result.SetVector3D(out);
}

void FunctionRotate(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR && !params[0].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    if (params[1].FieldType() != FIELD_VECTOR && !params[1].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    
    Vector vec1;
    params[0].Vector3D(vec1);
    QAngle ang;
    params[1].Vector3D(*reinterpret_cast<Vector *>(&ang));
    Vector out;
    VectorRotate(vec1, ang, out);
    result.SetVector3D(out);
}

void FunctionToAngles(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR && !params[0].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    
    Vector vec1;
    params[0].Vector3D(vec1);
    QAngle out;
    VectorAngles(vec1, out);
    result.SetVector3D(*reinterpret_cast<Vector *>(&out));
}

void FunctionNormalize(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR && !params[0].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    
    Vector vec1;
    params[0].Vector3D(vec1);
    result.SetVector3D(vec1.Normalized());
}

void FunctionToForwardVector(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR && !params[0].Convert(FIELD_VECTOR)) {
        result = variant_t();
        return;
    }
    
    Vector vec1;
    params[0].Vector3D(vec1);
    Vector out;
    AngleVectors(*reinterpret_cast<QAngle *>(&vec1), &out);
    result.SetVector3D(out);
}

void FunctionClamp(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    for (int i = 0; i < 3; i++) {
        if (params[i].FieldType() != FIELD_FLOAT && !params[i].Convert(FIELD_FLOAT)) {
            result = variant_t();
            return;
        }
    }
    result.SetFloat(clamp(params[0].Float(), params[1].Float(), params[2].Float()));
}

void FunctionRemap(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    for (int i = 0; i < 5; i++) {
        if (params[i].FieldType() != FIELD_FLOAT && !params[i].Convert(FIELD_FLOAT)) {
            result = variant_t();
            return;
        }
    }
    result.SetFloat(RemapVal(params[0].Float(), params[1].Float(), params[2].Float(), params[3].Float(), params[4].Float()));
}

void FunctionRemapClamp(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    for (int i = 0; i < 5; i++) {
        if (params[i].FieldType() != FIELD_FLOAT && !params[i].Convert(FIELD_FLOAT)) {
            result = variant_t();
            return;
        }
    }
    result.SetFloat(RemapValClamped(params[0].Float(), params[1].Float(), params[2].Float(), params[3].Float(), params[4].Float()));
}

void FunctionToInt(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    params[0].Convert(FIELD_INTEGER);
    result.SetInt(params[0].Int());
}

void FunctionToFloat(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    params[0].Convert(FIELD_FLOAT);
    result.SetFloat(params[0].Float());
}

void FunctionToString(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    result.SetString(params[0].FieldType() == FIELD_STRING ? params[0].StringID() : AllocPooledString(params[0].String()));
}

void FunctionToStringPadding(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    char fmt[32];
    params[0].Convert(FIELD_FLOAT);
    params[1].Convert(FIELD_INTEGER);
    if (param_count > 2) {
        params[2].Convert(FIELD_INTEGER);
        snprintf(fmt, 32, CFmtStr("%%0%d.%df", params[1].Int(), params[2].Int()), params[0].Float());
    }
    else {
        snprintf(fmt, 32, CFmtStr("%%0%d.f", params[1].Int()), params[0].Float());
    }
    result.SetString(AllocPooledString(CFmtStr(fmt,params[0])));
}

void FunctionToVector(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].Convert(FIELD_VECTOR)) {
        result = params[0];
        return;
    }
    
    if (param_count < 3) { result = variant_t(); return; }
    params[0].Convert(FIELD_FLOAT);
    params[1].Convert(FIELD_FLOAT);
    params[2].Convert(FIELD_FLOAT);
    Vector vec(params[0].Float(), params[1].Float(), params[2].Float());
    result.SetVector3D(vec);
}

void FunctionGetX(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR)
        params[0].Convert(FIELD_VECTOR);
        
    Vector vec;
    params[0].Vector3D(vec);
    result.SetFloat(vec.x);
}

void FunctionGetY(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR)
        params[0].Convert(FIELD_VECTOR);

    Vector vec;
    params[0].Vector3D(vec);
    result.SetFloat(vec.y);
}

void FunctionGetZ(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_VECTOR)
        params[0].Convert(FIELD_VECTOR);

    Vector vec;
    params[0].Vector3D(vec);
    result.SetFloat(vec.z);
}

void FunctionNegate(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_INTEGER)
        params[0].Convert(FIELD_INTEGER);

    result.SetInt(!params[0].Int());
}

void FunctionReverse(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_INTEGER)
        params[0].Convert(FIELD_INTEGER);

    result.SetInt(~params[0].Int());
}

void FunctionSquareRoot(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    result.SetFloat(sqrt(params[0].Float()));
}

void FunctionPower(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    if (params[1].FieldType() != FIELD_FLOAT)
        params[1].Convert(FIELD_FLOAT);

    result.SetFloat(pow(params[0].Float(), params[1].Float()));
}

void FunctionFloor(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    result.SetInt((int)floor(params[0].Float()));
}

void FunctionCeil(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    result.SetInt((int)ceil(params[0].Float()));
}

void FunctionRandomInt(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_INTEGER)
        params[0].Convert(FIELD_INTEGER);

    if (params[1].FieldType() != FIELD_INTEGER)
        params[1].Convert(FIELD_INTEGER);

    int range = params[1].Int() - params[0].Int() + 1;
    if (range != 0)
        result.SetInt(params[0].Int() + rand() % (range));
    else 
        result.SetInt(params[0].Int());
}

void FunctionRandomFloat(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    if (params[1].FieldType() != FIELD_FLOAT)
        params[1].Convert(FIELD_FLOAT);

    result.SetFloat(params[0].Float() + ((float)rand() / RAND_MAX) * (params[1].Float() - params[0].Float()));
}

void FunctionCase(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    params[0].Convert(FIELD_INTEGER);
    int param = params[0].Int();
    if (param > 0 && param < param_count - 1) {
        result = params[param + 1];
    }
    else {
        result = params[1];
    }
}

void FunctionSin(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    result.SetFloat(sin(DEG2RAD(params[0].Float())));
}

void FunctionCos(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    result.SetFloat(cos(DEG2RAD(params[0].Float())));
}

void FunctionTan(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    result.SetFloat(tan(DEG2RAD(params[0].Float())));
}

void FunctionAtan(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    result.SetFloat(RAD2DEG(atan(params[0].Float())));
}

void FunctionAbs(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() == FIELD_STRING)
        params[0].Convert(FIELD_FLOAT);

    if (params[0].FieldType() == FIELD_INTEGER) {
        result.SetInt(abs(params[0].Int()));
    }
    else if (params[0].FieldType() == FIELD_FLOAT) {
        result.SetFloat(abs(params[0].Float()));
    }
    else {
        result = params[0];
    }
}

void FunctionAtan2(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_FLOAT)
        params[0].Convert(FIELD_FLOAT);

    if (params[1].FieldType() != FIELD_FLOAT)
        params[1].Convert(FIELD_FLOAT);

    result.SetFloat(RAD2DEG(atan2(params[0].Float(), params[1].Float())));
}

void FunctionGetPlayer(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_INTEGER)
        params[0].Convert(FIELD_INTEGER);


    CHandle<CBaseEntity> entity = UTIL_PlayerByIndex(params[0].Int());
    result.Set(FIELD_EHANDLE, &entity);
}

void FunctionGetEntityIndex(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_EHANDLE)
        params[0].Convert(FIELD_EHANDLE);

    CBaseEntity *entity = params[0].Entity();
    if (entity == nullptr) {
        result.SetInt(-1);
        return;
    }
    result.SetInt(ENTINDEX(entity));
}

void FunctionGetPlayerItemAtSlot(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_EHANDLE)
        params[0].Convert(FIELD_EHANDLE);

    if (params[1].FieldType() != FIELD_INTEGER)
        params[1].Convert(FIELD_INTEGER);

    CTFPlayer *player = ToTFPlayer(params[0].Entity());
    if (player == nullptr) {
        result = variant_t();
        return;
    }
    CHandle<CBaseEntity> entity = GetEconEntityAtLoadoutSlot(player, params[1].Int());
    result.Set(FIELD_EHANDLE, &entity);
}

void FunctionGetPlayerItemByName(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_EHANDLE)
        params[0].Convert(FIELD_EHANDLE);

    CTFPlayer *player = ToTFPlayer(params[0].Entity());
    if (player == nullptr) {
        result = variant_t();
        return;
    }
    CHandle<CBaseEntity> entityh;
    ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
        if (entity->GetItem() != nullptr && FStrEq(GetItemName(entity->GetItem()), params[1].String())) {
            entityh = entity;
        }
    });
    result.Set(FIELD_EHANDLE, &entityh);
}

void FunctionGetItemAttribute(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{   
    if (params[0].FieldType() != FIELD_EHANDLE)
        params[0].Convert(FIELD_EHANDLE);

    CBaseEntity *entity = params[0].Entity();
    if (entity == nullptr) {
        result = variant_t();
        return;
    }
    
    CAttributeList *list = nullptr; 
    if (entity->IsPlayer()) {
        list = ToTFPlayer(entity)->GetAttributeList();
    } 
    else {
        CEconEntity *econEntity = rtti_cast<CEconEntity *>(entity);
        if (econEntity != nullptr) {
            list = &econEntity->GetItem()->GetAttributeList();
        }
    }
    if (list == nullptr) {
        result = variant_t();
        return;
    }
    CEconItemAttribute * attr = list->GetAttributeByName(params[1].String());
    variant_t variable;
    bool found = false;
    if (attr != nullptr) {
        if (!attr->GetStaticData()->IsType<CSchemaAttributeType_String>()) {
            result.SetFloat(attr->GetValuePtr()->m_Float);
        }
        else {
            char buf[256];
            attr->GetStaticData()->ConvertValueToString(*attr->GetValuePtr(), buf, sizeof(buf));
            result.SetString(AllocPooledString(buf));
        }
    }
    else {
        result = variant_t();
        return;
    }
}

void FunctionGetType(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    const char *type;
    switch(params[0].FieldType()) {
        case FIELD_VOID: type = PStr<"void">(); break;
        case FIELD_INTEGER: type = PStr<"int">(); break;
        case FIELD_FLOAT: type = PStr<"float">(); break;
        case FIELD_STRING: type = PStr<"string">(); break;
        case FIELD_EHANDLE: type = PStr<"entity">(); break;
        case FIELD_CLASSPTR: type = PStr<"entityptr">(); break;
        case FIELD_COLOR32: type = PStr<"color">(); break;
        case FIELD_VECTOR: type = PStr<"vector">(); break;
        default: type = PStr<"unknown">(); break;
    }
    result.SetString(MAKE_STRING(type));
}

void FunctionCharAt(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    params[1].Convert(FIELD_INTEGER);
    const char *str = params[0].String();
    int len = strlen(str);
    if (params[1].Int() >= 0 && params[1].Int() < len) {
        char buf[3];
        buf[0] = str[params[1].Int()];
        buf[1] = '\0';
        result.SetString(AllocPooledString(buf));
    }
    else {
        result = variant_t();
    }
}

void FunctionSubstring(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    params[1].Convert(FIELD_INTEGER);
    params[2].Convert(FIELD_INTEGER);
    const char *str = params[0].String();
    int len = strlen(str);
    if (params[2].Int() >= 0 && params[1].Int() >= 0 && params[1].Int() + params[2].Int() <= len) {
        char *buf = alloca(params[2].Int() + 1);
        strncpy(buf, str + params[1].Int(), params[2].Int());
        buf[params[2].Int()] = '\0';
        result.SetString(AllocPooledString(buf));
    }
    else {
        result = variant_t();
    }
}

void FunctionStartsWith(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    const char *str = params[0].String();
    result.SetInt(StringStartsWith(str, params[1].String(), param_count == 2 ? false : (bool)params[2].Int()));
}

void FunctionEndsWith(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    const char *str = params[0].String();
    result.SetInt(StringEndsWith(str, params[1].String(), param_count == 2 ? false : (bool)params[2].Int()));
}

void FunctionFind(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    const char *str = params[0].String();
    const char *find = param_count == 3 && params[2].Int() ? FindCaseInsensitive(str, params[1].String()) : strstr(str, params[1].String());
    if (find != nullptr) {
        result.SetInt(find - str);
    }
    else {
        result.SetInt(-1);
    }
}

/*void FunctionReplace(const char *function, Evaluation::Params &params, int param_count, variant_t& result)
{
    const char *str = params[0].String();
    const char *find = nullptr;
    while ((find = param_count == 3 && params[2].Int() ? FindCaseInsensitive(str, params[1].String()) : strstr(str, params[1].String())) != nullptr) {
        
    }
}*/

class ExpressionFunctionInit
{
public:
    ExpressionFunctionInit() 
    {
        Evaluation::AddFunction("test", FunctionTest, {"test expression", "if true", "if false"});
        Evaluation::AddFunction("?", FunctionTest, {"test expression", "if true", "if false"});
        Evaluation::AddFunction("exists", FunctionTestExists, {"value"});
        Evaluation::AddFunction("min", FunctionMin, {"value 1", "value 2"});
        Evaluation::AddFunction("max", FunctionMax, {"value 1", "value 2"});
        Evaluation::AddFunction("length", FunctionLength, {"vector"});
        Evaluation::AddFunction("distance", FunctionDistance, {"vector 1", "vector 2"});
        Evaluation::AddFunction("dotproduct", FunctionDot, {"vector 1", "vector 2"});
        Evaluation::AddFunction("crossproduct", FunctionCross, {"vector 1", "vector 2"});
        Evaluation::AddFunction("rotate", FunctionRotate, {"input vector", "rotation angles"});
        Evaluation::AddFunction("normalize", FunctionNormalize, {"vector"});
        Evaluation::AddFunction("toangles", FunctionToAngles, {"vector"});
        Evaluation::AddFunction("toforwardvector", FunctionToForwardVector, {"vector"});
        Evaluation::AddFunction("clamp", FunctionClamp, {"input value", "minimum value", "maximum value"});
        Evaluation::AddFunction("remap", FunctionRemap, {"input value", "from min", "from max", "to min", "to max"});
        Evaluation::AddFunction("remapclamped", FunctionRemapClamp, {"input value", "from min", "from max", "to min", "to max"});
        Evaluation::AddFunction("int", FunctionToInt, {"value"});
        Evaluation::AddFunction("float", FunctionToFloat, {"value"});
        Evaluation::AddFunction("string", FunctionToString, {"value"});
        Evaluation::AddFunction("stringpad", FunctionToStringPadding, {"value", "min digits", "num digits after decimal point"});
        Evaluation::AddFunction("vector", FunctionToVector, {"string or X coordinate"}, {"[Y coordinate]', '[Z coordinate]"});
        Evaluation::AddFunction("x", FunctionGetX, {"vector"});
        Evaluation::AddFunction("y", FunctionGetY, {"vector"});
        Evaluation::AddFunction("z", FunctionGetZ, {"vector"});
        Evaluation::AddFunction("not", FunctionNegate, {"value"});
        Evaluation::AddFunction("!", FunctionNegate, {"value"});
        Evaluation::AddFunction("~", FunctionReverse, {"value"});
        Evaluation::AddFunction("sqrt", FunctionSquareRoot, {"value"});
        Evaluation::AddFunction("pow", FunctionPower, {"base", "exponent"});
        Evaluation::AddFunction("floor", FunctionFloor, {"value"});
        Evaluation::AddFunction("ceil", FunctionCeil, {"value"});
        Evaluation::AddFunction("randomint", FunctionRandomInt, {"minimum value", "maximum value"});
        Evaluation::AddFunction("randomfloat", FunctionRandomFloat, {"minimum value", "maximum value"});
        Evaluation::AddFunction("case", FunctionCase, {"test value", "default", "case1"}, {"[case2]..."});
        Evaluation::AddFunction("sin", FunctionSin, {"angle in degrees"});
        Evaluation::AddFunction("cos", FunctionCos, {"angle in degrees"});
        Evaluation::AddFunction("tan", FunctionTan, {"angle in degrees"});
        Evaluation::AddFunction("atan", FunctionAtan, {"value"});
        Evaluation::AddFunction("atan2", FunctionAtan2, {"x", "y"});
        Evaluation::AddFunction("abs", FunctionAbs, {"value"});
        Evaluation::AddFunction("playeratindex", FunctionGetPlayer, {"index"});
        Evaluation::AddFunction("entityindex", FunctionGetEntityIndex, {"entity"});
        Evaluation::AddFunction("playeritematslot", FunctionGetPlayerItemAtSlot, {"entity", "slot"});
        Evaluation::AddFunction("playeritembyname", FunctionGetPlayerItemByName, {"entity", "item name"});
        Evaluation::AddFunction("attribute", FunctionGetItemAttribute, {"player or item", "attribute name"});
        Evaluation::AddFunction("type", FunctionGetType, {"value"});
        Evaluation::AddFunction("charat", FunctionCharAt, {"string", "pos"});
        Evaluation::AddFunction("substr", FunctionSubstring, {"string", "pos", "length"});
        Evaluation::AddFunction("substring", FunctionSubstring, {"string", "pos", "length"});
        Evaluation::AddFunction("startswith", FunctionStartsWith, {"string", "prefix"});
        Evaluation::AddFunction("endswith", FunctionEndsWith, {"string", "suffix"});
        Evaluation::AddFunction("find", FunctionFind, {"haystack", "needle"}, {"case sensitive = true"});
        //Evaluation::AddFunction("replace", FunctionReplace, 3, "haystack", "needle", "replace");
    };
};
ExpressionFunctionInit exprfuninit;
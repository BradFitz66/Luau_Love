// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"
#include "lnumutils.h"

#include <math.h>
#include <cstring>

LUAU_FASTFLAGVARIABLE(LuauVectorLerp)

static int vector_pack(lua_State* L)
{
    int count = lua_gettop(L);

    double x = luaL_checknumber(L, 1);
    double y = luaL_optnumber(L, 2, x);
    double z = luaL_optnumber(L, 3, count == 1 ? x : 0.0);

#if LUA_VECTOR_SIZE == 4
    double w = luaL_optnumber(L, 4, count == 1 ? x : 0.0);

    lua_pushvector(L, float(x), float(y), float(z), float(w));
#else
    lua_pushvector(L, float(x), float(y), float(z));
#endif

    return 1;
}

static int vector_unpack(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

    lua_pushnumber(L, v[0]);
    lua_pushnumber(L, v[1]);
    lua_pushnumber(L, v[2]);
#if LUA_VECTOR_SIZE == 4
    lua_pushnumber(L, v[3]);
    return 4;
#else
    return 3;
#endif
}

static int vector_length(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushnumber(L, sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]));
#else
    lua_pushnumber(L, sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
#endif

    return 1;
}

static int vector_normalize(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    float length2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3];

    if (length2 > 0.0f)
    {
        float invSqrt = 1.0f / sqrtf(length2);
        lua_pushvector(L, v[0] * invSqrt, v[1] * invSqrt, v[2] * invSqrt, v[3] * invSqrt);
    }
    else
    {
        lua_pushvector(L, 0.0f, 0.0f, 0.0f, 0.0f);
    }
#else
    float length2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

    if (length2 > 0.0f)
    {
        float invSqrt = 1.0f / sqrtf(length2);
        lua_pushvector(L, v[0] * invSqrt, v[1] * invSqrt, v[2] * invSqrt);
    }
    else
    {
        lua_pushvector(L, 0.0f, 0.0f, 0.0f);
    }
#endif

    return 1;
}

static int vector_distance(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);

    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];

#if LUA_VECTOR_SIZE == 4
    float dw = a[3] - b[3];
    lua_pushnumber(L, sqrtf(dx * dx + dy * dy + dz * dz + dw * dw));
#else
    lua_pushnumber(L, sqrtf(dx * dx + dy * dy + dz * dz));
#endif

    return 1;
}

static int vector_cross(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0], 0.0f);
#else
    lua_pushvector(L, a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]);
#endif

    return 1;
}

static int vector_dot(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);

#if LUA_VECTOR_SIZE == 4
    lua_pushnumber(L, a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]);
#else
    lua_pushnumber(L, a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
#endif

    return 1;
}

static int vector_angle(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);
    const float* axis = luaL_optvector(L, 3, nullptr);

    // cross(a, b)
    float cross[] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};

    double sinA = sqrt(cross[0] * cross[0] + cross[1] * cross[1] + cross[2] * cross[2]);
    double cosA = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    double angle = atan2(sinA, cosA);

    if (axis)
    {
        if (cross[0] * axis[0] + cross[1] * axis[1] + cross[2] * axis[2] < 0.0f)
            angle = -angle;
    }

    lua_pushnumber(L, angle);
    return 1;
}

static int vector_floor(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, floorf(v[0]), floorf(v[1]), floorf(v[2]), floorf(v[3]));
#else
    lua_pushvector(L, floorf(v[0]), floorf(v[1]), floorf(v[2]));
#endif

    return 1;
}

static int vector_ceil(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, ceilf(v[0]), ceilf(v[1]), ceilf(v[2]), ceilf(v[3]));
#else
    lua_pushvector(L, ceilf(v[0]), ceilf(v[1]), ceilf(v[2]));
#endif

    return 1;
}

static int vector_abs(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, fabsf(v[0]), fabsf(v[1]), fabsf(v[2]), fabsf(v[3]));
#else
    lua_pushvector(L, fabsf(v[0]), fabsf(v[1]), fabsf(v[2]));
#endif

    return 1;
}

static int vector_sign(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, luaui_signf(v[0]), luaui_signf(v[1]), luaui_signf(v[2]), luaui_signf(v[3]));
#else
    lua_pushvector(L, luaui_signf(v[0]), luaui_signf(v[1]), luaui_signf(v[2]));
#endif

    return 1;
}

static int vector_clamp(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    const float* min = luaL_checkvector(L, 2);
    const float* max = luaL_checkvector(L, 3);

    luaL_argcheck(L, min[0] <= max[0], 3, "max.x must be greater than or equal to min.x");
    luaL_argcheck(L, min[1] <= max[1], 3, "max.y must be greater than or equal to min.y");
    luaL_argcheck(L, min[2] <= max[2], 3, "max.z must be greater than or equal to min.z");

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(
        L,
        luaui_clampf(v[0], min[0], max[0]),
        luaui_clampf(v[1], min[1], max[1]),
        luaui_clampf(v[2], min[2], max[2]),
        luaui_clampf(v[3], min[3], max[3])
    );
#else
    lua_pushvector(L, luaui_clampf(v[0], min[0], max[0]), luaui_clampf(v[1], min[1], max[1]), luaui_clampf(v[2], min[2], max[2]));
#endif

    return 1;
}

static int vector_min(lua_State* L)
{
    int n = lua_gettop(L);
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    float result[] = {v[0], v[1], v[2], v[3]};
#else
    float result[] = {v[0], v[1], v[2]};
#endif

    for (int i = 2; i <= n; i++)
    {
        const float* b = luaL_checkvector(L, i);

        if (b[0] < result[0])
            result[0] = b[0];
        if (b[1] < result[1])
            result[1] = b[1];
        if (b[2] < result[2])
            result[2] = b[2];
#if LUA_VECTOR_SIZE == 4
        if (b[3] < result[3])
            result[3] = b[3];
#endif
    }

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, result[0], result[1], result[2], result[3]);
#else
    lua_pushvector(L, result[0], result[1], result[2]);
#endif

    return 1;
}

static int vector_max(lua_State* L)
{
    int n = lua_gettop(L);
    const float* v = luaL_checkvector(L, 1);

#if LUA_VECTOR_SIZE == 4
    float result[] = {v[0], v[1], v[2], v[3]};
#else
    float result[] = {v[0], v[1], v[2]};
#endif

    for (int i = 2; i <= n; i++)
    {
        const float* b = luaL_checkvector(L, i);

        if (b[0] > result[0])
            result[0] = b[0];
        if (b[1] > result[1])
            result[1] = b[1];
        if (b[2] > result[2])
            result[2] = b[2];
#if LUA_VECTOR_SIZE == 4
        if (b[3] > result[3])
            result[3] = b[3];
#endif
    }

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, result[0], result[1], result[2], result[3]);
#else
    lua_pushvector(L, result[0], result[1], result[2]);
#endif

    return 1;
}

static int vector_lerp(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);
    float t = float(luaL_checknumber(L, 3));

    float x = a[0] + (b[0] - a[0]) * t;
    float y = a[1] + (b[1] - a[1]) * t;
    float z = a[2] + (b[2] - a[2]) * t;

#if LUA_VECTOR_SIZE == 4
    float w = a[3] + (b[3] - a[3]) * t;
    lua_pushvector(L, x, y, z, w);
#else
    lua_pushvector(L, x, y, z);
#endif

    return 1;
}

static int vector_rotate(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    const short* q = luaL_checkquaternion(L, 2);

    float qx = fmaxf(q[0] / 32767.f, -1.f);
    float qy = fmaxf(q[1] / 32767.f, -1.f);
    float qz = fmaxf(q[2] / 32767.f, -1.f);
    float qw = fmaxf(q[3] / 32767.f, -1.f);

    float u[3] = { qx, qy, qz };
    float c[3] = { qy * v[2] - qz * v[1], qz * v[0] - qx * v[2], qx * v[1] - qy * v[0] };

    float uu = u[0] * u[0] + u[1] * u[1] + u[2] * u[2];
    float uv = u[0] * v[0] + u[1] * v[1] + u[2] * v[2];
    float s = qw * qw - uu;

    float out[3];
    out[0] = v[0] * s + u[0] * 2.f * uv + c[0] * 2.f * qw;
    out[1] = v[1] * s + u[1] * 2.f * uv + c[1] * 2.f * qw;
    out[2] = v[2] * s + u[2] * 2.f * uv + c[2] * 2.f * qw;

#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, out[0], out[1], out[2], 0.f);
#else
    lua_pushvector(L, out[0], out[1], out[2]);
#endif

    return 1;
}

static int vector_index(lua_State* L)
{
    const float* v = luaL_checkvector(L, 1);
    size_t namelen = 0;
    const char* name = luaL_checklstring(L, 2, &namelen);

    // field access implementation mirrors the fast-path we have in the VM
    if (namelen == 1)
    {
        int ic = (name[0] | ' ') - 'x';

#if LUA_VECTOR_SIZE == 4
        // 'w' is before 'x' in ascii, so ic is -1 when indexing with 'w'
        if (ic == -1)
            ic = 3;
#endif

        if (unsigned(ic) < LUA_VECTOR_SIZE)
        {
            lua_pushnumber(L, v[ic]);
            return 1;
        }
    }

    // fall back to vector library functions stored in metatable
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, name);
    if (!lua_isnil(L, -1))
    {
        return 1;
    }

    luaL_error(L, "attempt to index vector with '%s'", name);
}

static int vector_namecall(lua_State* L)
{
    if (const char* str = lua_namecallatom(L, nullptr))
    {
        if (strcmp(str, "create") == 0)
            return vector_pack(L);
        if (strcmp(str, "pack") == 0)
            return vector_pack(L);
        if (strcmp(str, "unpack") == 0)
            return vector_unpack(L);
        if (strcmp(str, "length") == 0)
            return vector_length(L);
        if (strcmp(str, "magnitude") == 0)
            return vector_length(L);
        if (strcmp(str, "normalize") == 0)
            return vector_normalize(L);
        if (strcmp(str, "distance") == 0)
            return vector_distance(L);
        if (strcmp(str, "cross") == 0)
            return vector_cross(L);
        if (strcmp(str, "dot") == 0)
            return vector_dot(L);
        if (strcmp(str, "angle") == 0)
            return vector_angle(L);
        if (strcmp(str, "floor") == 0)
            return vector_floor(L);
        if (strcmp(str, "ceil") == 0)
            return vector_ceil(L);
        if (strcmp(str, "abs") == 0)
            return vector_abs(L);
        if (strcmp(str, "sign") == 0)
            return vector_sign(L);
        if (strcmp(str, "clamp") == 0)
            return vector_clamp(L);
        if (strcmp(str, "min") == 0)
            return vector_min(L);
        if (strcmp(str, "max") == 0)
            return vector_max(L);
        if (strcmp(str, "lerp") == 0)
            return vector_lerp(L);
        if (strcmp(str, "rotate") == 0)
            return vector_rotate(L);
    }

    luaL_error(L, "%s is not a valid method of vector", luaL_checkstring(L, 1));
}

static int vector_call(lua_State* L)
{
    lua_remove(L, 1);
    return vector_pack(L);
}

static const luaL_Reg vectorlib[] = {
    {"create", vector_pack},
    {"pack", vector_pack},
    {"unpack", vector_unpack},
    {"length", vector_length},
    {"magnitude", vector_length},
    {"normalize", vector_normalize},
    {"distance", vector_distance},
    {"cross", vector_cross},
    {"dot", vector_dot},
    {"angle", vector_angle},
    {"floor", vector_floor},
    {"ceil", vector_ceil},
    {"abs", vector_abs},
    {"sign", vector_sign},
    {"clamp", vector_clamp},
    {"min", vector_min},
    {"max", vector_max},
    {"lerp", vector_lerp},
    {"rotate", vector_rotate},
    {NULL, NULL},
};

int luaopen_vector(lua_State* L)
{
    // vector
    luaL_register(L, LUA_VECLIBNAME, vectorlib);

    // __call
    lua_newtable(L);
    lua_pushcfunction(L, vector_call);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);

    // constants
#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, 0.0f, 0.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, "zero");
    lua_pushvector(L, 1.0f, 1.0f, 1.0f, 1.0f);
    lua_setfield(L, -2, "one");
    lua_pushvector(L, -1.0f, 0.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, "left");
    lua_pushvector(L, 1.0f, 0.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, "right");
    lua_pushvector(L, 0.0f, 1.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, "up");
    lua_pushvector(L, 0.0f, -1.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, "down");
    lua_pushvector(L, 0.0f, 0.0f, -1.0f, 0.0f);
    lua_setfield(L, -2, "forward");
    lua_pushvector(L, 0.0f, 0.0f, 1.0f, 0.0f);
    lua_setfield(L, -2, "back");
    lua_pushvector(L, 0.0f, 0.0f, 1.0f, 0.0f);
    lua_setfield(L, -2, "backward");
#else
    lua_pushvector(L, 0.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, "zero");
    lua_pushvector(L, 1.0f, 1.0f, 1.0f);
    lua_setfield(L, -2, "one");
    lua_pushvector(L, -1.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, "left");
    lua_pushvector(L, 1.0f, 0.0f, 0.0f);
    lua_setfield(L, -2, "right");
    lua_pushvector(L, 0.0f, 1.0f, 0.0f);
    lua_setfield(L, -2, "up");
    lua_pushvector(L, 0.0f, -1.0f, 0.0f);
    lua_setfield(L, -2, "down");
    lua_pushvector(L, 0.0f, 0.0f, -1.0f);
    lua_setfield(L, -2, "forward");
    lua_pushvector(L, 0.0f, 0.0f, 1.0f);
    lua_setfield(L, -2, "back");
    lua_pushvector(L, 0.0f, 0.0f, 1.0f);
    lua_setfield(L, -2, "backward");
#endif

    // metatable
#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, 0.0f, 0.0f, 0.0f, 0.0f);
#else
    lua_pushvector(L, 0.0f, 0.0f, 0.0f);
#endif

    lua_newtable(L);
    luaL_register(L, NULL, vectorlib);

    lua_pushcfunction(L, vector_index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, vector_namecall);
    lua_setfield(L, -2, "__namecall");

    lua_setmetatable(L, -2); // set metatable
    lua_pop(L, 1); // pop dummy vector

    return 1;
}

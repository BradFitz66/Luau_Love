// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
#include "lualib.h"

#include "lcommon.h"
#include "lnumutils.h"

#include <math.h>
#include <cstring>

#undef PI
#define PI (3.14159265358979323846)

static int quaternion_pack(lua_State* L)
{
    if (lua_isnoneornil(L, 1))
    {
        lua_pushquaternion(L, 0.f, 0.f, 0.f, 1.f);
        return 1;
    }

    float x = float(luaL_checknumber(L, 1));
    float y = float(luaL_checknumber(L, 2));
    float z = float(luaL_checknumber(L, 3));
    float w = float(luaL_checknumber(L, 4));

    float length2 = x * x + y * y + z * z + w * w;

    if (length2 < 1e-10f)
    {
        lua_pushquaternion(L, 0.f, 0.f, 0.f, 1.f);
    }
    else
    {
        float length = sqrtf(length2);
        lua_pushquaternion(L, float(x / length), float(y / length), float(z / length), float(w / length));
    }

    return 1;
}

static int quaternion_unpack(lua_State* L)
{
    const short* q = luaL_checkquaternion(L, 1);
    float x = luaui_maxf(q[0] / 32767.f, -1.f);
    float y = luaui_maxf(q[1] / 32767.f, -1.f);
    float z = luaui_maxf(q[2] / 32767.f, -1.f);
    float w = luaui_maxf(q[3] / 32767.f, -1.f);
    lua_pushnumber(L, x);
    lua_pushnumber(L, y);
    lua_pushnumber(L, z);
    lua_pushnumber(L, w);
    return 4;
}

static int quaternion_conjugate(lua_State* L)
{
    const short* q = luaL_checkquaternion(L, 1);
    float x = luaui_maxf(q[0] / 32767.f, -1.f);
    float y = luaui_maxf(q[1] / 32767.f, -1.f);
    float z = luaui_maxf(q[2] / 32767.f, -1.f);
    float w = luaui_maxf(q[3] / 32767.f, -1.f);
    lua_pushquaternion(L, -x, -y, -z, w);
    return 1;
}

static int quaternion_angleaxis(lua_State* L)
{
    float angle = float(luaL_checknumber(L, 1));
    float ax = float(luaL_checknumber(L, 2));
    float ay = float(luaL_checknumber(L, 3));
    float az = float(luaL_checknumber(L, 4));

    float s = sinf(angle * .5f);
    float c = cosf(angle * .5f);
    float length = sqrtf(ax * ax + ay * ay + az * az);
    if (length > 0.f) s /= length;

    lua_pushquaternion(L, s * ax, s * ay, s * az, c);
    return 1;
}

static int quaternion_toangleaxis(lua_State* L)
{
    const short* q = luaL_checkquaternion(L, 1);
    float x = luaui_maxf(q[0] / 32767.f, -1.f);
    float y = luaui_maxf(q[1] / 32767.f, -1.f);
    float z = luaui_maxf(q[2] / 32767.f, -1.f);
    float w = luaui_maxf(q[3] / 32767.f, -1.f);
    float s = sqrtf(1.f - w * w);
    s = s < .0001f ? 1.f : 1.f / s;
    lua_pushnumber(L, 2.f * acosf(w));
    lua_pushnumber(L, x * s);
    lua_pushnumber(L, y * s);
    lua_pushnumber(L, z * s);
    return 4;
}

static int quaternion_euler(lua_State* L)
{
  float x = float(luaL_checknumber(L, 1));
  float y = float(luaL_checknumber(L, 2));
  float z = float(luaL_checknumber(L, 3));

  float cx = cosf(x * .5f);
  float sx = sinf(x * .5f);
  float cy = cosf(y * .5f);
  float sy = sinf(y * .5f);
  float cz = cosf(z * .5f);
  float sz = sinf(z * .5f);

  lua_pushquaternion(L,
    cy * sx * cz + sy * cx * sz,
    sy * cx * cz - cy * sx * sz,
    cy * cx * sz - sy * sx * cz,
    cy * cx * cz + sy * sx * sz
  );

  return 1;
}

static int quaternion_toeuler(lua_State* L)
{
  const short* q = luaL_checkquaternion(L, 1);

  float x = luaui_maxf(q[0] / 32767.f, -1.f);
  float y = luaui_maxf(q[1] / 32767.f, -1.f);
  float z = luaui_maxf(q[2] / 32767.f, -1.f);
  float w = luaui_maxf(q[3] / 32767.f, -1.f);

  float unit = x * x + y * y + z * z + w * w;
  float test = x * w - y * z;
  const float eps = 1e-7f;

  float ax, ay, az;

  if (test > (.5f - eps) * unit) {
    ax = (float) PI / 2.f;
    ay = 2.f * atan2f(y, x);
    az = 0.f;
  } else if (test < -(.5f - eps) * unit) {
    ax = (float) -PI / 2.f;
    ay = -2.f * atan2f(y, x);
    az = 0.f;
  } else {
    ax = asinf(2.f * (w * x - y * z));
    ay = atan2f(2.f * w * y + 2.f * z * x, 1.f - 2.f * (x * x + y * y));
    az = atan2f(2.f * w * z + 2.f * x * y, 1.f - 2.f * (z * z + x * x));
  }

  lua_pushnumber(L, ax);
  lua_pushnumber(L, ay);
  lua_pushnumber(L, az);
  return 3;
}

static int quaternion_between(lua_State* L)
{
    const float* a = luaL_checkvector(L, 1);
    const float* b = luaL_checkvector(L, 2);

    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

    if (dot > .99999f || dot < -.99999f) {
      lua_pushquaternion(L, 0.f, 0.f, 0.f, 1.f);
      return 1;
    }

    float x = a[1] * b[2] - a[2] * b[1];
    float y = a[2] * b[0] - a[0] * b[2];
    float z = a[0] * b[1] - a[1] * b[0];
    float w = 1.f + dot;

    float length = sqrtf(x * x + y * y + z * z + w * w);

    lua_pushquaternion(L, x / length, y / length, z / length, w / length);

    return 1;
}

static int quaternion_lookdir(lua_State* L)
{
    const float yup[3] = { 0.f, 1.f, 0.f };
    const float* dir = luaL_checkvector(L, 1);
    const float* up = luaL_optvector(L, 2, yup);

    float Z[3] = { -dir[0], -dir[1], -dir[2] };
    float length = sqrtf(Z[0] * Z[0] + Z[1] * Z[1] + Z[2] * Z[2]);

    if (length == 0.f) {
        lua_pushquaternion(L, 0.f, 0.f, 0.f, 1.f);
        return 1;
    }

    Z[0] /= length;
    Z[1] /= length;
    Z[2] /= length;

    float X[3] = { up[1] * Z[2] - up[2] * Z[1], up[2] * Z[0] - up[0] * Z[2], up[0] * Z[1] - up[1] * Z[0] };
    length = sqrtf(X[0] * X[0] + X[1] * X[1] + X[2] * X[2]);

    if (length == 0.f) {
      if (fabs(Z[0]) < .9f) {
        X[0] = 0.f;
        X[1] = Z[2];
        X[2] = -Z[1];
      } else {
        X[0] = Z[2];
        X[1] = 0.f;
        X[2] = -Z[0];
      }

      length = sqrtf(X[0] * X[0] + X[1] * X[1] + X[2] * X[2]);
    }

    X[0] /= length;
    X[1] /= length;
    X[2] /= length;

    float Y[3] = { Z[1] * X[2] - Z[2] * X[1], Z[2] * X[0] - Z[0] * X[2], Z[0] * X[1] - Z[1] * X[0] };

    float m00 = X[0], m01 = X[1], m02 = X[2];
    float m10 = Y[0], m11 = Y[1], m12 = Y[2];
    float m20 = Z[0], m21 = Z[1], m22 = Z[2];

    float x, y, z, w;

    if (m22 < 0.f) {
      if (m00 > m11) {
        float t = 1.f + m00 - m11 - m22;
        float s = .5f / sqrtf(t);
        x = t * s;
        y = (m01 + m10) * s;
        z = (m20 + m02) * s;
        w = (m12 - m21) * s;
      } else {
        float t = 1.f - m00 + m11 - m22;
        float s = .5f / sqrtf(t);
        x = (m01 + m10) * s;
        y = t * s;
        z = (m12 + m21) * s;
        w = (m20 - m02) * s;
      }
    } else {
      if (m00 < -m11) {
        float t = 1.f - m00 - m11 + m22;
        float s = .5f / sqrtf(t);
        x = (m20 + m02) * s;
        y = (m12 + m21) * s;
        z = t * s;
        w = (m01 - m10) * s;
      } else {
        float t = 1.f + m00 + m11 + m22;
        float s = .5f / sqrtf(t);
        x = (m12 - m21) * s;
        y = (m20 - m02) * s;
        z = (m01 - m10) * s;
        w = t * s;
      }
    }

    length = sqrtf(x * x + y * y + z * z + w * w);
    lua_pushquaternion(L, x / length, y / length, z / length, w / length);

    return 1;
}

static int quaternion_direction(lua_State* L)
{
    const short* q = luaL_checkquaternion(L, 1);
    float qx = luaui_maxf(q[0] / 32767.f, -1.f);
    float qy = luaui_maxf(q[1] / 32767.f, -1.f);
    float qz = luaui_maxf(q[2] / 32767.f, -1.f);
    float qw = luaui_maxf(q[3] / 32767.f, -1.f);
    float x = -2.f * qx * qz - 2.f * qw * qy;
    float y = -2.f * qy * qz + 2.f * qw * qx;
    float z = -1.f + 2.f * qx * qx + 2.f * qy * qy;
#if LUA_VECTOR_SIZE == 4
    lua_pushvector(L, x, y, z, 0.f);
#else
    lua_pushvector(L, x, y, z);
#endif
    return 1;
}

static int quaternion_slerp(lua_State* L)
{
    const short* q = luaL_checkquaternion(L, 1);
    const short* r = luaL_checkquaternion(L, 2);
    float t = float(luaL_checknumber(L, 3));

    float qx = luaui_maxf(q[0] / 32767.f, -1.f);
    float qy = luaui_maxf(q[1] / 32767.f, -1.f);
    float qz = luaui_maxf(q[2] / 32767.f, -1.f);
    float qw = luaui_maxf(q[3] / 32767.f, -1.f);

    float rx = luaui_maxf(r[0] / 32767.f, -1.f);
    float ry = luaui_maxf(r[1] / 32767.f, -1.f);
    float rz = luaui_maxf(r[2] / 32767.f, -1.f);
    float rw = luaui_maxf(r[3] / 32767.f, -1.f);

    float dot = qx * rx + qy * ry + qz * rz + qw * rw;

    if (fabsf(dot) >= 1.f) {
      lua_settop(L, 1);
      return 1;
    }

    if (dot < 0.f) {
      dot *= -1.f;
      qx *= -1.f;
      qy *= -1.f;
      qz *= -1.f;
      qw *= -1.f;
    }

    float halfTheta = acosf(dot);
    float sinHalfTheta = sqrtf(1.f - dot * dot);

    if (fabsf(sinHalfTheta) < .001f) {
      float x = qx * .5f + rx * .5f;
      float y = qy * .5f + ry * .5f;
      float z = qz * .5f + rz * .5f;
      float w = qw * .5f + rw * .5f;
      lua_pushquaternion(L, x, y, z, w);
      return 1;
    }

    float a = sinf((1.f - t) * halfTheta) / sinHalfTheta;
    float b = sinf(t * halfTheta) / sinHalfTheta;

    float x = qx * a + rx * b;
    float y = qy * a + ry * b;
    float z = qz * a + rz * b;
    float w = qw * a + rw * b;
    lua_pushquaternion(L, x, y, z, w);
    return 1;
}

static int quaternion_call(lua_State* L)
{
    if (lua_isnoneornil(L, 2))
    {
        lua_pushquaternion(L, 0.f, 0.f, 0.f, 1.f);
        return 1;
    }
    else
    {
        lua_remove(L, 1);
        return quaternion_angleaxis(L);
    }
}

static int quaternion_index(lua_State* L)
{
    const short* q = luaL_checkquaternion(L, 1);
    size_t namelen = 0;
    const char* name = luaL_checklstring(L, 2, &namelen);

    // field access implementation mirrors the fast-path we have in the VM
    if (namelen == 1)
    {
        int ic = (name[0] | ' ') - 'x';

        // 'w' is before 'x' in ascii, so ic is -1 when indexing with 'w'
        if (ic == -1)
            ic = 3;

        if (unsigned(ic) < 4)
        {
            lua_pushnumber(L, luaui_maxf(q[ic] / 32767.f, -1.f));
            return 1;
        }
    }

    // fall back to quaternion library functions stored in metatable
    lua_getmetatable(L, 1);
    lua_getfield(L, -1, name);
    if (!lua_isnil(L, -1))
    {
        return 1;
    }

    luaL_error(L, "attempt to index quaternion with '%s'", name);
}

static int quaternion_namecall(lua_State* L)
{
    if (const char* str = lua_namecallatom(L, nullptr))
    {
        if (strcmp(str, "pack") == 0)
            return quaternion_pack(L);
        if (strcmp(str, "unpack") == 0)
            return quaternion_unpack(L);
        if (strcmp(str, "conjugate") == 0)
            return quaternion_conjugate(L);
        if (strcmp(str, "angleaxis") == 0)
            return quaternion_angleaxis(L);
        if (strcmp(str, "toangleaxis") == 0)
            return quaternion_toangleaxis(L);
        if (strcmp(str, "euler") == 0)
            return quaternion_euler(L);
        if (strcmp(str, "toeuler") == 0)
            return quaternion_toeuler(L);
        if (strcmp(str, "between") == 0)
            return quaternion_between(L);
        if (strcmp(str, "lookdir") == 0)
            return quaternion_lookdir(L);
        if (strcmp(str, "direction") == 0)
            return quaternion_direction(L);
        if (strcmp(str, "slerp") == 0)
            return quaternion_slerp(L);
    }

    luaL_error(L, "%s is not a valid method of quaternion", luaL_checkstring(L, 1));
}

static const luaL_Reg quaternionlib[] = {
    {"pack", quaternion_pack},
    {"unpack", quaternion_unpack},
    {"conjugate", quaternion_conjugate},
    {"angleaxis", quaternion_angleaxis},
    {"toangleaxis", quaternion_toangleaxis},
    {"euler", quaternion_euler},
    {"toeuler", quaternion_toeuler},
    {"between", quaternion_between},
    {"lookdir", quaternion_lookdir},
    {"direction", quaternion_direction},
    {"slerp", quaternion_slerp},
    {NULL, NULL},
};

int luaopen_quaternion(lua_State* L)
{
    // quaternion
    luaL_register(L, LUA_QUATLIBNAME, quaternionlib);

    // __call
    lua_newtable(L);
    lua_pushcfunction(L, quaternion_call);
    lua_setfield(L, -2, "__call");
    lua_setmetatable(L, -2);

    // constants
    lua_pushquaternion(L, 0.0f, 0.0f, 0.0f, 1.0f);
    lua_setfield(L, -2, "identity");

    // metatable
    lua_pushquaternion(L, 0.0f, 0.0f, 0.0f, 1.0f);

    lua_newtable(L);
    luaL_register(L, NULL, quaternionlib);

    lua_pushcfunction(L, quaternion_index);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, quaternion_namecall);
    lua_setfield(L, -2, "__namecall");

    lua_setmetatable(L, -2); // set metatable
    lua_pop(L, 1); // pop dummy quaternion

    return 1;
}

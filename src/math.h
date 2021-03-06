#pragma once

#define PI 3.14159265358979323846f
#define TWOPI 6.2831853071795864769f
#define TORAD (PI/180.f)
#include <string.h>
#include "immintrin.h"

struct v2;
struct v3;
struct v4;

struct v2
{
	float x;
	float y;

	void operator +=(const v2& r);
	void operator -=(const v2& r);
	void operator *=(const v2& r);
	void operator /=(const v2& r);
	void operator +=(const float r);
	void operator -=(const float r);
	void operator *=(const float r);
	void operator /=(const float r);

};



struct v3
{
	float x;
	float y;
	float z;
	void operator +=(const v3& r);
	void operator -=(const v3& r);
	void operator *=(const v3& r);
	void operator /=(const v3& r);

	void operator +=(const float r);
	void operator -=(const float r);
	void operator *=(const float r);
	void operator /=(const float r);

	v2 tov2();
	v4 tov4();
	v4 tov4(float w);

	uint32_t tocolor();
};
struct v4
{
	float x;
	float y;
	float z;
	float w;

	void operator +=(const v4& r);
	void operator -=(const v4& r);
	void operator *=(const v4& r);
	void operator /=(const v4& r);

	void operator +=(const float r);
	void operator -=(const float r);
	void operator *=(const float r);
	void operator /=(const float r);



	v2 tov2();
	v3 tov3();
	uint32_t tocolor();
};
///
/// ROW-MAJOR
///
/// x y z w    x
/// x y z w  * y
/// x y z w    z
/// x y z w    w
#ifdef _WIN32
#define ALIGN16 __declspecl(align(16))
#else
#define ALIGN16 __attribute__((aligned(16)))
#endif

struct m
{
	//{
		v4 x;
		v4 y;
		v4 z;
		// union
		// {
			v4 trans;
// 			v4 w;
//		};
	//};
	// struct
	// {
	// 	v4 r0;
	// 	v4 r1;
	// 	v4 r2;
	// 	v4 r3;
	// };
};
inline
m mload(const float* pFloats)
{
	m temp;
	memcpy(&temp, pFloats, 16*sizeof(float));
	return temp;
}
inline
bool operator ==(const m& l, const m& r)
{
	return 0 == memcmp(&l, &r, sizeof(m));
}

v2 operator +(const v2 l, const v2 r);
v2 operator -(const v2 l, const v2 r);
v2 operator *(const v2 l, const v2 r);
v2 operator /(const v2 l, const v2 r);
bool operator <(const v2 l, const v2 r);

v2 operator +(const v2 l, float f);
v2 operator -(const v2 l, float f);
v2 operator *(const v2 v, float f);
v2 operator /(const v2 v, float f);
v2 operator +(float f, const v2 l);
v2 operator -(float f, const v2 l);
v2 operator *(float f, const v2 v);
v2 operator /(float f, const v2 v);

float v2length(v2 v);
v2 v2normalize(v2 v);
v2 v2hat(v2 v);
v2 v2reflect(v2 normal, v2 direction);
inline v2 v2neg(v2 v)
{ v.x = -v.x; v.y = -v.y; return v;}
float v2dot(v2 v);
float v2dot(v2 v0, v2 v1);
inline
float v2length2(v2 v){return v2dot(v,v);}
inline float v2distance2(v2 a, v2 b)
{
	return v2length2(a-b);
}
inline float v2distance(v2 a, v2 b)
{
	return v2length(a-b);
}

inline
v2 v2init(float x, float y){v2 r; r.x = x; r.y = y; return r;}
inline 
v2 v2init(float f){v2 r; r.x = f; r.y = f; return r;}

inline
v2 v2zero(){v2 z = {0,0};return z;}
inline
v2 v2max(v2 v0, v2 v1)
{
	v2 r; 
	r.x = v0.x < v1.x ? v1.x : v0.x;
	r.y = v0.y < v1.y ? v1.y : v0.y;
	return r; 
}
inline
v2 v2min(v2 v0, v2 v1)
{
	v2 r; 
	r.x = v0.x < v1.x ? v0.x : v1.x; 
	r.y = v0.y < v1.y ? v0.y : v1.y; 
	return r; 
}
v2 v2round(v2 v);
v2 v2sign(v2 v);
v2 v2abs(v2 v);
inline
void v2swap(v2& va, v2& vb) { v2 vtmp = vb; vb = va; va = vtmp; }
inline
v2 v2floor(v2 v){v2 r = v; r.x = floor(r.x); r.y = floor(r.y); return r;}
inline
v2 v2clamp(v2 value, v2 min_, v2 max_){ return v2min(max_, v2max(min_, value)); }
inline
v2 v2lerp(v2 from_, v2 to_, float fLerp) { return from_ + (to_-from_) * fLerp; }
inline
float v2ManhattanDistance(v2 from_, v2 to_) { return (fabs(from_.x-to_.x) + fabs(from_.y-to_.y)); }
inline
float fclamp(float value, float min_, float max_){ if(value<min_) return min_; if(value>max_) return max_; return value; }

float v2findpenetration(v2 pos, v2 dir, v2 size, v2 p0, v2 p1, v2 p2, v2 p3, v2& dirout, v2& pointout);
void v2createbounds(v2 vPosition, v2 vDirection, v2 vSize, v2& min_, v2& max_);
void v2createcorners(v2 vPosition, v2 vDirection, v2 vSize, v2& p0, v2& p1, v2& p2, v2& p3);
bool v2fucked(v2 v);
v2 v2fromangle(float fAngle);

inline bool v2iszero(v2 v){return v2length2(v)<1e-8f;}


v3 operator +(const v3 l, const v3 r);
v3 operator -(const v3 l, const v3 r);
v3 operator *(const v3 l, const v3 r);
v3 operator /(const v3 l, const v3 r);

v3 operator +(const v3 l, float f);
v3 operator -(const v3 l, float f);
v3 operator *(const v3 v, float f);
v3 operator /(const v3 v, float f);
v3 operator +(float f, const v3 l);
v3 operator -(float f, const v3 l);
v3 operator *(float f, const v3 v);
v3 operator /(float f, const v3 v);
v3 operator -(const v3 v);


inline 
v3 v3init(float f){v3 r; r.x = f; r.y = f; r.z = f; return r;}
inline 
v3 v3init(v2 v, float f){v3 r; r.x = v.x; r.y = v.y; r.z = f; return r;}

inline
v3 v3init(float x, float y, float z){v3 r; r.x = x; r.y = y;r.z = z; return r;}
inline 
v3 v3init(v4 v)
{ v3 r; r.x = v.x; r.y = v.y; r.z = v.z; return r;}

inline
v3 v3load(float* f)
{
	v3 r;
	r.x = f[0];
	r.y = f[1];
	r.z = f[2];
	return r;
}

inline
v3 v3lerp(v3 from_, v3 to_, float fLerp) { return from_ + (to_-from_) * fLerp; }


inline
float v3dot(v3 v0, v3 v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}
float v3distance(v3 p0, v3 p1);
v3 v3fromcolor(uint32_t nColor);

v3 v3cross(v3 v0, v3 v1);
float v3length(v3 v);
float v3lengthsq(v3 v);
v3 v3normalize(v3 v);
v3 v3min(v3 a, v3 b);
v3 v3min(v3 a, float f);
v3 v3max(v3 a, v3 b);
v3 v3max(v3 a, float f);

v3 v3abs(v3 a, v3 b);
v3 v3splatx(v3 v);
v3 v3splaty(v3 v);
v3 v3splatz(v3 v);
inline
v3 v3zero(){v3 z = {0,0,0};return z;}
inline 
v3 v3rep(float f){ v3 r = {f,f,f}; return r;}




inline 
v4 v4init(float f){v4 r; r.x = f; r.y = f; r.z = f; r.w = f; return r;}
inline
v4 v4zero(){v4 z = {0,0,0,0};return z;}

inline
v4 v4init(float x, float y, float z, float w) {v4 r; r.x = x; r.y = y; r.z = z; r.w = w; return r;}
inline
v4 v4init(v3 v, float w){ v4 r; r.x = v.x; r.y = v.y; r.z = v.z; r.w = w; return r; }
inline
v4 v4init(v4 v, float w){ v4 r; r.x = v.x; r.y = v.y; r.z = v.z; r.w = w; return r; }
v4 operator +(const v4 l, const v4 r);
v4 operator -(const v4 l, const v4 r);
v4 operator *(const v4 l, const v4 r);
v4 operator /(const v4 l, const v4 r);
v4 operator +(const v4 l, float f);
v4 operator -(const v4 l, float f);
v4 operator *(const v4 v, float f);
v4 operator /(const v4 v, float f);
v4 operator +(float f, const v4 l);
v4 operator -(float f, const v4 l);
v4 operator *(float f, const v4 v);
v4 operator /(float f, const v4 v);
v4 operator -(const v4 v);


v4 v4fromcolor(uint32_t nColor);
v4 v4neg(v4 v);
float v4dot(v4 v0, v4 v1);
float v4length(v4 v0);
float v4length2(v4 v0);

v4 v4makeplane(v3 p, v3 normal);



inline float v4length2(v4 v0){ return v0.x * v0.x + v0.y * v0.y + v0.z * v0.z + v0.w * v0.w;}
inline float v4distance(v4 v0, v4 v1){ return v4length(v0-v1);}
inline float v4distance2(v4 v0, v4 v1){ return v4length2(v0-v1);}

inline
v4 v4lerp(v4 from_, v4 to_, float fLerp) { return from_ + (to_-from_) * fLerp; }




m minit(v3 vx, v3 vy, v3 vz, v3 vtrans);
m mid();
m mcreate(v3 vDir, v3 vRight, v3 vPoint);
m mcreate(v3 vDir, v3 vRight, v3 vUp, v3 vPoint);
m mmult(m m0, m m1);
m mmult_sse(const m* m0, const m* m1);


m mtranspose(m mat);
m mrotatex(float fAngle);
m mrotatey(float fAngle);
m mrotatez(float fAngle);
m mscale(float fScale);
m mscale(float fScaleX, float fScaleY, float fScaleZ);
m mtranslate(v3 trans);
m mviewport(float x, float y, float w, float h);
m mperspective(float fFovY, float fAspect, float fNear, float fFar);
m mortho(float fXWidth, float fYWidth, float fZRange);
m morthogl(float left, float right, float top, float bottom, float near, float far);

v3 mtransform(m mat, v3 point);
v4 mtransform(m mat, v4 vector);
v3 mrotate(m mat, v3 vector);
m minverse(m);
void msetxaxis(m& mat, v3 axis);
void msetyaxis(m& mat, v3 axis);
void msetzaxis(m& mat, v3 axis);
inline v3 mgetxaxis(const m& mat){v3 r; r.x = mat.x.x; r.y = mat.x.y; r.z = mat.x.z; return r;}
inline v3 mgetyaxis(const m& mat){v3 r; r.x = mat.y.x; r.y = mat.y.y; r.z = mat.y.z; return r;}
inline v3 mgetzaxis(const m& mat){v3 r; r.x = mat.z.x; r.y = mat.z.y; r.z = mat.z.z; return r;}
m minverserotation(m mat);
m maffineinverse(m mat);
void ZASSERTAFFINE(m mat);


v3 obbtoaabb(m mrotation, v3 vHalfSize);
#define INTERSECT_FAIL (-FLT_MAX)
float rayplaneintersect(v3 r0, v3 rdir, v4 plane);
float rayplaneintersect(v3 r0, v3 rdir, v3 p0, v3 pnormal);
float rayboxintersect(v3 r0, v3 rdir, m boxtransform, v3 boxsize);


float frand();
int32_t randrange(int32_t nmin, int32_t nmax);
float frandrange(float fmin, float fmax);
v2 v2randir();
v2 v2randdisc();
uint32 randcolor();
uint32 randredcolor();

uint64_t rand64();
uint64_t rand64(uint64_t nPrev);

uint32_t rand32();
void randseed(uint32_t k, uint32_t j);



#define ZASSERTNORMALIZED2(v) ZASSERT(fabs(v2length(v)-1.f) < 1e-4f)
#define ZASSERTNORMALIZED3(v) ZASSERT(fabs(v3length(v)-1.f) < 1e-4f)
#define ZASSERTNORMALIZED4(v) ZASSERT(fabs(v4length(v)-1.f) < 1e-4f)

inline float signf(float f) { return f<0.0f ? -1.0f : 1.0f; }

#ifdef _WIN32
inline
float round(float f)
{
	return floor(f+0.5f);
}
#endif


template<typename T>
T Min(T a, T b)
{ return a < b ? a : b; }

template<typename T>
T Max(T a, T b)
{ return a > b ? a : b; }

template<typename T>
T Clamp(T v, T a, T b)
{ return Max(Min(v,b), a); }

template<typename T>
void Swap(T& a, T& b)
{
	T t = a;
	a = b;
	b = t;
}
//#ifndef _WINDOWS
//#define inline __inline __attribute__((always_inline))

//#endif

inline v2 v3::tov2()
{
	v2 r;
	r.x = x;
	r.y = y;
	return r;
}

inline v4 v3::tov4()
{
	v4 r;
	r.x = x;
	r.y = y;
	r.z = z;
	r.w = 0.f;
	return r;
}
inline uint32_t v3::tocolor()
{
	return 0xff000000 
		| ((uint32_t(z * 255.f) << 16)&0xff0000)
		| ((uint32_t(y * 255.f) << 8)&0xff00)
		| (uint32_t(z * 255.f));
}

inline v4 v3::tov4(float w)
{
	v4 r;
	r.x = x;
	r.y = y;
	r.z = z;
	r.w = w;
	return r;
}

inline v3 v3cross(v3 v0, v3 v1)
{
	v3 r;
	r.x = v0.y * v1.z - v0.z * v1.y;
	r.y = v0.z * v1.x - v0.x * v1.z;
	r.z = v0.x * v1.y - v0.y * v1.x;
	return r;
}


inline v3 v4::tov3()
{
	v3 r;
	r.x = x;
	r.y = y;
	r.z = z;
	return r;
}

inline v2 v4::tov2()
{
	v2 r;
	r.x = x;
	r.y = y;
	return r;
}
inline v4 v4neg(v4 v)
{
	v.x = -v.x;
	v.y = -v.y;
	v.z = -v.z;
	v.w = -v.w;
	return v;
}

inline float v4dot(v4 v0, v4 v1)
{
	return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z + v0.w * v1.w;
}



inline __m128 rsqrt(__m128 x)
{
	__m128 v = _mm_rsqrt_ps(x);
	__m128 halfx = _mm_mul_ps(x, _mm_set1_ps(-0.5f));
	__m128 x2 = _mm_mul_ps(v, v);
	__m128 foo = _mm_mul_ps(v, _mm_add_ps(_mm_set1_ps(1.5f), _mm_mul_ps(x2, halfx)));
	return foo;
}

inline v3 v3normalize(v3 v_)
{
#if 1
	__m128 v;
	__m128 x = _mm_load_ss(&v_.x);
	__m128 y = _mm_load_ss(&v_.y);
	__m128 z = _mm_load_ss(&v_.z);
	__m128 xy = _mm_movelh_ps(x, y);
	v = _mm_shuffle_ps(xy, z, _MM_SHUFFLE(2, 0, 2, 0));
	__m128 r0 = _mm_mul_ps(v, v);
	__m128 r1 = _mm_hadd_ps(r0, r0);
	__m128 r2 = _mm_hadd_ps(r1, r1);
	__m128 result = _mm_mul_ps(rsqrt(r2), v);
	return *(v3*)&result;
#else
	return v_ / v3length(v_);
#endif

}


inline
m mmult_sse(const m* m0_, const m* m1_)
{
	__m128* p0 = (__m128*)m0_;
	__m128* p1 = (__m128*)m1_;
	__m128 m0_x = _mm_loadu_ps((float*)&p0[0]);
	__m128 m0_y = _mm_loadu_ps((float*)&p0[1]);
	__m128 m0_z = _mm_loadu_ps((float*)&p0[2]);
	__m128 m0_w = _mm_loadu_ps((float*)&p0[3]);
	__m128 m1_x = _mm_loadu_ps((float*)&p1[0]);
	__m128 m1_y = _mm_loadu_ps((float*)&p1[1]);
	__m128 m1_z = _mm_loadu_ps((float*)&p1[2]);
	__m128 m1_w = _mm_loadu_ps((float*)&p1[3]);

	__m128 rx = _mm_mul_ps(m0_x, _mm_shuffle_ps(m1_x, m1_x, 0));
	rx = _mm_add_ps(rx, _mm_mul_ps(m0_y, _mm_shuffle_ps(m1_x,m1_x, 0x55)));
	rx = _mm_add_ps(rx, _mm_mul_ps(m0_z, _mm_shuffle_ps(m1_x, m1_x, 0xaa)));
	rx = _mm_add_ps(rx, _mm_mul_ps(m0_w, _mm_shuffle_ps(m1_x, m1_x, 0xff)));
	__m128 ry = _mm_mul_ps(m0_x, _mm_shuffle_ps(m1_y, m1_y, 0));
	ry = _mm_add_ps(ry, _mm_mul_ps(m0_y, _mm_shuffle_ps(m1_y,m1_y, 0x55)));
	ry = _mm_add_ps(ry, _mm_mul_ps(m0_z, _mm_shuffle_ps(m1_y, m1_y, 0xaa)));
	ry = _mm_add_ps(ry, _mm_mul_ps(m0_w, _mm_shuffle_ps(m1_y, m1_y, 0xff)));
	__m128 rz = _mm_mul_ps(m0_x, _mm_shuffle_ps(m1_z, m1_z, 0));
	rz = _mm_add_ps(rz, _mm_mul_ps(m0_y, _mm_shuffle_ps(m1_z,m1_z, 0x55)));
	rz = _mm_add_ps(rz, _mm_mul_ps(m0_z, _mm_shuffle_ps(m1_z, m1_z, 0xaa)));
	rz = _mm_add_ps(rz, _mm_mul_ps(m0_w, _mm_shuffle_ps(m1_z, m1_z, 0xff)));
	__m128 rw = _mm_mul_ps(m0_x, _mm_shuffle_ps(m1_w, m1_w, 0));
	rw = _mm_add_ps(rw, _mm_mul_ps(m0_y, _mm_shuffle_ps(m1_w,m1_w, 0x55)));
	rw = _mm_add_ps(rw, _mm_mul_ps(m0_z, _mm_shuffle_ps(m1_w, m1_w, 0xaa)));
	rw = _mm_add_ps(rw, _mm_mul_ps(m0_w, _mm_shuffle_ps(m1_w, m1_w, 0xff)));

	m r1;
	_mm_storeu_ps((float*)&((__m128*)&r1)[0], rx);
	_mm_storeu_ps((float*)&((__m128*)&r1)[1], ry);
	_mm_storeu_ps((float*)&((__m128*)&r1)[2], rz);
	_mm_storeu_ps((float*)&((__m128*)&r1)[3], rw);

	return r1;
}



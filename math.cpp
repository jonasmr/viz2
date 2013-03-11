#include <math.h>
#include <float.h>

#include "base.h"
#include "math.h"


void v2::operator +=(const v2& r)
{
	x += r.x;
	y += r.y;
}
void v2::operator -=(const v2& r)
{
	x -= r.x;
	y -= r.y;
}
void v2::operator *=(const v2& r)
{
	x *= r.x;
	y *= r.y;
}
void v2::operator /=(const v2& r)
{
	x /= r.x;
	y /= r.y;
}
void v2::operator +=(const float r)
{
	x += r;
	y += r;
}
void v2::operator -=(const float r)
{
	x -= r;
	y -= r;
}
void v2::operator *=(const float r)
{
	x *= r;
	y *= r;
}
void v2::operator /=(const float r)
{
	x /= r;
	y /= r;
}

bool operator <(const v2 l, const v2 r)
{
	return (l.x<r.x && l.y<r.y);
}

v2 operator +(const v2 l, const v2 r)
{
	v2 res;
	res.x = l.x+r.x;
	res.y = l.y+r.y;
	return res;
}

v2 operator +(const v2 l, float f)
{
	v2 res;
	res.x = l.x+f;
	res.y = l.y+f;
	return res;
}
v2 operator -(const v2 l, const v2 r)
{
	v2 res;
	res.x = l.x-r.x;
	res.y = l.y-r.y;
	return res;
}

v2 operator -(const v2 l, float f)
{
	v2 res;
	res.x = l.x-f;
	res.y = l.y-f;
	return res;
}

v2 operator *(const v2 v, const v2 r)
{
	v2 res;
	res.x = v.x*r.x;
	res.y = v.y*r.y;
	return res;
}


v2 operator *(const v2 v, float f)
{
	v2 res;
	res.x = v.x*f;
	res.y = v.y*f;
	return res;
}

v2 operator /(const v2 v, const v2 r)
{
	v2 res;
	res.x = v.x/r.x;
	res.y = v.y/r.y;
	return res;
}


v2 operator /(const v2 v, float f)
{
	v2 res;
	res.x = v.x/f;
	res.y = v.y/f;
	return res;
}

float v2length(v2 v)
{
	return sqrt(v.x * v.x + v.y * v.y);
}

float v2dot(v2 v)
{
	return v.x * v.x + v.y * v.y;
}

float v2dot(v2 v0, v2 v1)
{
	return v0.x * v1.x + v0.y * v1.y;
}

v2 v2normalize(v2 v)
{
	if(v2iszero(v)) return v2init(0.0f,0.0f);
	return v / v2length(v);
}

v2 v2hat(v2 v)
{
	v2 r;
	r.x = v.y;
	r.y = -v.x;
	return r;
}

v2 v2reflect(v2 normal, v2 direction)
{
	return direction - normal * 2.f* v2dot(direction, normal);
}

float sign(float foo)
{
	return foo > 0 ? 1.f : -1.f;
}
v2 v2sign(v2 v)
{
	v2 r;
	r.x = sign(v.x);
	r.y = sign(v.y);
	return r;
}
v2 v2abs(v2 v)
{
	v2 r;
	r.x = fabs(v.x);
	r.y = fabs(v.y);
	return r;
}
v2 v2round(v2 v)
{
	return v2init(round(v.x), round(v.y));
}
float v2findpenetration(v2 pos, v2 dir, v2 size, v2 p0, v2 p1, v2 p2, v2 p3, v2& dirout, v2& pointout)
{
	ZASSERT(abs(v2length(dir)-1.f) < 1e-4f);
	v2 dirhat = v2hat(dir);
	p0 = p0 - pos;
	p1 = p1 - pos;
	p2 = p2 - pos;
	p3 = p3 - pos;
	float fPenetration = 0;
	float fSign = 1;
	v2 dirout_ = v2zero();
	v2 pointout_ = v2zero();

	float dp0 = v2dot(p0,dir);
	float dp1 = v2dot(p1,dir);
	float dp2 = v2dot(p2,dir);
	float dp3 = v2dot(p3,dir);
	float dp0hat = v2dot(p0,dirhat);
	float dp1hat = v2dot(p1,dirhat);
	float dp2hat = v2dot(p2,dirhat);
	float dp3hat = v2dot(p3,dirhat);
	float d0 = fabs(dp0);
	float d1 = fabs(dp1);
	float d2 = fabs(dp2);
	float d3 = fabs(dp3);
	float d0hat = fabs(dp0hat);
	float d1hat = fabs(dp1hat);
	float d2hat = fabs(dp2hat);
	float d3hat = fabs(dp3hat);

 	if(d0 < size.x && d0hat < size.y)
 	{
 		float fDist = size.x-d0;
 		float fDistHat = size.y-d0hat;
 		float fMinDist = Min(fDist, fDistHat);
 		if(fMinDist > fPenetration)
 		{
 			pointout_ = p0;
 			fPenetration = fDist;
 			if(fDist < fDistHat)
 				dirout_ = dir * sign(dp0)*(size.x - d0);
 			else
 				dirout_ = dirhat * sign(dp0hat) * (size.y - d0hat);
 		}
	}
 	if(d1 < size.x && d1hat < size.y)
 	{
 		float fDist = size.x-d1;
 		float fDistHat = size.y-d1hat;
 		float fMinDist = Min(fDist, fDistHat);
 		if(fMinDist > fPenetration)
 		{
 			pointout_ = p1;
 			fPenetration = fDist;
 			if(fDist < fDistHat)
 				dirout_ = dir * sign(dp1)*(size.x - d1);
 			else
 				dirout_ = dirhat * sign(dp1hat) * (size.y - d1hat);
 		}
 	}
 	if(d2 < size.x && d2hat < size.y)
 	{
 		float fDist = size.x-d2;
 		float fDistHat = size.y-d2hat;
  		float fMinDist = Min(fDist, fDistHat);
 		if(fMinDist > fPenetration)
 		{
 			pointout_ = p2;
 			fPenetration = fDist;
 			if(fDist < fDistHat)
 				dirout_ = dir * sign(dp2)*(size.x - d2);
 			else
 				dirout_ = dirhat * sign(dp2hat) * (size.y - d2hat);
 		}
 	}
 	if(d3 < size.x && d3hat < size.y)
 	{
 		float fDist = size.x-d3;
 		float fDistHat = size.y-d3hat;
  		float fMinDist = Min(fDist, fDistHat);
 		if(fMinDist > fPenetration)
 		{
 			pointout_ = p3;
 			fPenetration = fDist;
 			if(fDist < fDistHat)
 				dirout_ = dir * sign(dp3)*(size.x - d3);
 			else
 				dirout_ = dirhat * sign(dp3hat) * (size.y - d3hat);
 		}
 	}
 	dirout = dirout_;
 	pointout = pointout_ + pos;
 	return fPenetration;
}


void v2createcorners(v2 vPosition, v2 vDirection, v2 vSize, v2& p0, v2& p1, v2& p2, v2& p3)
{
	v2 vDirScale = vDirection * vSize.x;
	v2 vDirScaleHat = v2hat(vDirection) * vSize.y;
	p0 = vPosition + vDirScale + vDirScaleHat;
	p1 = vPosition - vDirScale + vDirScaleHat;
	p2 = vPosition - vDirScale - vDirScaleHat;
	p3 = vPosition + vDirScale - vDirScaleHat;
}

void v2createbounds(v2 vPosition, v2 vDirection, v2 vSize, v2& min_, v2& max_)
{
	v2 p0,p1,p2,p3;
	v2createcorners(vPosition, vDirection, vSize, p0, p1, p2, p3);
	min_ = v2min(v2min(p2,p3), v2min(p0,p1));
	max_ = v2max(v2max(p2,p3), v2max(p0,p1));
}


#define FLOAT_IS_OK(f) (f <= FLT_MAX && f >= -FLT_MAX)

bool v2fucked(v2 v)
{
	return (!(FLOAT_IS_OK(v.x) && FLOAT_IS_OK(v.y)));
}


v2 v2fromangle(float angle)
{
	v2 r;
	r.x = cosf(angle);
	r.y = sinf(angle);
	return r;
}	

void v3::operator +=(const v3& r)
{
	x += r.x;
	y += r.y;
	z += r.z;
}
void v3::operator -=(const v3& r)
{
	x -= r.x;
	y -= r.y;
	z -= r.z;
}
void v3::operator *=(const v3& r)
{
	x *= r.x;
	y *= r.y;
	z *= r.z;
}
void v3::operator /=(const v3& r)
{
	x /= r.x;
	y /= r.y;
	z /= r.z;
}

void v3::operator +=(const float r)
{
	x += r;
	y += r;
	z += r;
}
void v3::operator -=(const float r)
{
	x -= r;
	y -= r;
	z -= r;
}
void v3::operator *=(const float r)
{
	x *= r;
	y *= r;
	z *= r;
}
void v3::operator /=(const float r)
{
	x /= r;
	y /= r;
	z /= r;
}

v3 operator +(const v3 l, const v3 r)
{
	v3 res;
	res.x = l.x+r.x;
	res.y = l.y+r.y;
	res.z = l.z+r.z;	
	return res;
}

v3 operator +(const v3 l, float f)
{
	v3 res;
	res.x = l.x+f;
	res.y = l.y+f;
	res.z = l.z+f;	
	return res;
}
v3 operator -(const v3 l, const v3 r)
{
	v3 res;
	res.x = l.x-r.x;
	res.y = l.y-r.y;
	res.z = l.z-r.z;	
	return res;
}

v3 operator -(const v3 l, float f)
{
	v3 res;
	res.x = l.x-f;
	res.y = l.y-f;
	res.z = l.z-f;	
	return res;
}

v3 operator *(const v3 v, const v3 r)
{
	v3 res;
	res.x = v.x*r.x;
	res.y = v.y*r.y;
	res.z = v.z*r.z;	
	return res;
}


v3 operator *(const v3 v, float f)
{
	v3 res;
	res.x = v.x*f;
	res.y = v.y*f;
	res.z = v.z*f;	
	return res;
}

v3 operator /(const v3 v, const v3 r)
{
	v3 res;
	res.x = v.x/r.x;
	res.y = v.y/r.y;
	res.z = v.z/r.z;	
	return res;
}


v3 operator /(const v3 v, float f)
{
	v3 res;
	res.x = v.x/f;
	res.y = v.y/f;
	res.z = v.z/f;	
	return res;
}

v3 operator -(const v3 v)
{
	v3 r;
	r.x = -v.x;
	r.y = -v.y;
	r.z = -v.z;
	return r;
}

v3 v3cross(v3 v0, v3 v1)
{
	v3 r;
	r.x = v0.y * v1.z - v0.z * v1.y;
	r.y = v0.z * v1.x - v0.x * v1.z;
	r.z = v0.x * v1.y - v0.y * v1.x;
	return r;
}
float v3lengthsq(v3 v)
{	
	return v3dot(v, v);
}

float v3length(v3 v)
{	
	return sqrt(v3dot(v,v));
}
v3 v3normalize(v3 v)
{
	return v / v3length(v);

}




m minit(v3 vx, v3 vy, v3 vz, v3 vtrans)
{
	m r;
	r.x = vx;
	r.y = vy;
	r.z = vz;
	r.trans = v4init(vtrans, 1);
	return r;
}
m minit(v4 vx, v4 vy, v4 vz, v4 vtrans)
{
	m r;
	r.x = vx;
	r.y = vy;
	r.z = vz;
	r.trans = vtrans;
	return r;
}

m mid()
{
	m r;
	r.x = v4init(1,0,0,0);
	r.y = v4init(0,1,0,0);
	r.z = v4init(0,0,1,0);
	r.trans = v4init(0,0,0,1);
	return r;
}
m mcreate(v3 vDir, v3 vRight, v3 vPoint)
{
	v3 vUp = v3normalize(v3cross(vRight, vDir));
	m r0 = mid();
	r0.x = v4init(vRight,1.f);
	r0.y = v4init(vUp, 1.f);
	r0.z = v4init(-vDir, 1.f);
	m rt = mtranslate(-vPoint);
	return mmult(r0, rt);
}
m mmult(m m0, m m1)
{
	m r;
	r.x.x = m0.x.x * m1.x.x + m0.x.y * m1.y.x + m0.x.z * m1.z.x + m0.x.w * m1.w.x; 
	r.x.y = m0.x.x * m1.x.y + m0.x.y * m1.y.y + m0.x.z * m1.z.y + m0.x.w * m1.w.y; 
	r.x.z = m0.x.x * m1.x.z + m0.x.y * m1.y.z + m0.x.z * m1.z.z + m0.x.w * m1.w.z; 
	r.x.w = m0.x.x * m1.x.w + m0.x.y * m1.y.w + m0.x.z * m1.z.w + m0.x.w * m1.w.w; 

	r.y.x = m0.y.x * m1.x.x + m0.y.y * m1.y.x + m0.y.z * m1.z.x + m0.y.w * m1.w.x; 
	r.y.y = m0.y.x * m1.x.y + m0.y.y * m1.y.y + m0.y.z * m1.z.y + m0.y.w * m1.w.y; 
	r.y.z = m0.y.x * m1.x.z + m0.y.y * m1.y.z + m0.y.z * m1.z.z + m0.y.w * m1.w.z; 
	r.y.w = m0.y.x * m1.x.w + m0.y.y * m1.y.w + m0.y.z * m1.z.w + m0.y.w * m1.w.w; 

	r.z.x = m0.z.x * m1.x.x + m0.z.y * m1.y.x + m0.z.z * m1.z.x + m0.z.w * m1.w.x; 
	r.z.y = m0.z.x * m1.x.y + m0.z.y * m1.y.y + m0.z.z * m1.z.y + m0.z.w * m1.w.y; 
	r.z.z = m0.z.x * m1.x.z + m0.z.y * m1.y.z + m0.z.z * m1.z.z + m0.z.w * m1.w.z; 
	r.z.w = m0.z.x * m1.x.w + m0.z.y * m1.y.w + m0.z.z * m1.z.w + m0.z.w * m1.w.w; 

	r.y.x = m0.w.x * m1.x.x + m0.w.y * m1.y.x + m0.w.z * m1.z.x + m0.w.w * m1.w.x; 
	r.y.y = m0.w.x * m1.x.y + m0.w.y * m1.y.y + m0.w.z * m1.z.y + m0.w.w * m1.w.y; 
	r.y.z = m0.w.x * m1.x.z + m0.w.y * m1.y.z + m0.w.z * m1.z.z + m0.w.w * m1.w.z; 
	r.y.w = m0.w.x * m1.x.w + m0.w.y * m1.y.w + m0.w.z * m1.z.w + m0.w.w * m1.w.w; 

	return r;
}
m mrotatex(m m0, float fAngle)
{
	m r = mid();
	float ca = cos(fAngle);
	float sa = sin(fAngle);
	r.y.y = ca;
	r.y.z = -sa;
	r.z.y = sa;
	r.z.z = ca;
	return r;
}
m mrotatey(m m0, float fAngle)
{
	m r = mid();
	float ca = cos(fAngle);
	float sa = sin(fAngle);
	r.x.x = ca;
	r.x.z = sa;
	r.z.x = -sa;
	r.z.z = ca;
	return r;	
}
m mrotatez(m m0, float fAngle)
{
	m r = mid();
	float ca = cos(fAngle);
	float sa = sin(fAngle);
	r.x.x = ca;
	r.x.y = -sa;
	r.y.x = sa;
	r.y.z = ca;
	return r;	

}
m mtranslate(v3 trans)
{
	m r = mid();
	r.trans = trans;
	return r;
}

v3 mtransform(m mat, v3 point)
{
	v3 r;
	r.x = v3dot(v3init(mat.x), point);
	r.y = v3dot(v3init(mat.y), point);
	r.z = v3dot(v3init(mat.z), point);
	r += point;
	return r;
}



v2 v2randir()
{
	v2 v;
	do
	{
		v.x = randrange(-1000,1000);
		v.y = randrange(-1000,1000);
	}while(v2length2(v) < 1e-3f);
	return v2normalize(v);
}

v2 v2randdisc()
{
	return v2init(frand()*2.0f-1.0f, frand()*2.0f-1.0f);
}

int32_t randrange(int32_t nmin, int32_t nmax)
{
	if(nmin == nmax) return nmin;
	int r = rand();
	return nmin + (r % (nmax-nmin));
}

float frand()
{
	return (float)rand() / RAND_MAX;
}

float frandrange(float fmin, float fmax)
{
	return frand() * (fmax-fmin) + fmin;	
}

uint64_t rand64()
{
	//todo add better rand
	return rand();
}
uint64_t rand64(uint64_t)
{
	//todo: borken
	return rand();
}


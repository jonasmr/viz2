#include <stdio.h>
#include <stdarg.h>
#include <SDL.h>
#include <string>
#include "base.h"
#include "input.h"
#include "glinc.h"
#include "text.h"
#include "math.h"
#include "program.h"
#include "bsp.h"
#include "debug.h"
#include "manipulator.h"
#include "mesh.h"
#include "shader.h"
#include "physics.h"
#include "microprofile.h"
#include "test.h"
#include "imgui/imgui.h"
#include <functional>
#include <xmmintrin.h>
//#include <fenv.h>

uint32_t g_nDump = 0;
extern uint32_t g_Width;
extern uint32_t g_Height;




void WorldDrawObjects(bool* bCulled);
uint32_t CullObjects(uint32 nNumObjects, bool* bCulled);
uint32_t CullObjectsFast(uint32 nNumObjects, bool* bCulled, SWorldGrid& Grid);
void CompareCullResult(uint32_t nNumObjects, bool* bCulled, bool* bCulledFast);
void SetupCameraState(SCameraState& Camera, v3 vPosition, v3 vDir, v3 vRight);
void WriteCamera(const SCameraState& Camera);
void DoBuildBsp(SOccluderBspViewDesc& ViewDesc, int nDebugIndex = -1);




// void CullObjectsVerify(uint32 nNumObjects, bool* bCulled);
// void CullObjectsVerify2(uint32 nNumObjects);


bool g_DebugMask[10<<10];
uint32 g_nUseOrtho = 0;
float g_fOrthoScale = 10;
SOccluderBsp* g_Bsp = 0;
uint32 g_nDebugCullDraw = 0;
int g_nDebugDumpIndex = -1;

uint32_t g_nFailedObject = 0;


// UI
enum ECameraSource
{
	ESOURCE_FREECAM,
	ESOURCE_LOCKED,
	ESOURCE_TEST,
};
const char* g_CameraSourceStrings[] = 
{
	"FreeCam",
	"Locked",
	"Test",
};
int g_CameraSource = ESOURCE_FREECAM;
int g_CameraSourceBsp = ESOURCE_FREECAM;
bool g_bCameraVerify = true;
int g_nTestFrame = 0;
bool g_nTestAdvance = false;
bool g_nDebugMode = false;

int32 g_nBspNodeCap = 2048;
extern int g_UIEnabled;

SCameraState g_LockedCamera;
SCameraState g_FreeCamera;
SCameraState g_TestCamera;
SCameraState& GetCamera(int iSource)
{
	switch(iSource)
	{
		case ESOURCE_LOCKED:
			return g_LockedCamera;
		case ESOURCE_FREECAM:
			return g_FreeCamera;
		case ESOURCE_TEST:
			return g_TestCamera;
		default:
			return g_FreeCamera;
	};
}


// UI



uint32 g_nDrawGrid = 1;


// enum {
// 	LOCK_CAM_OFF=0,
// 	LOCK_CAM_BSP=1,
// 	LOCK_CAM_BOTH=2,
// };

// int g_LockCamera = LOCK_CAM_OFF;



#define TILE_SIZE 8
#define MAX_WIDTH 1920
#define MAX_HEIGHT 1080

#define SHADOWMAP_SIZE 2048

#define MAX_LIGHT_INDEX (32<<10)
#define LIGHT_INDEX_SIZE 1024
//#define TILE_HEIGHT 8
#define MAX_NUM_LIGHTS 1024
GLuint g_LightTexture;
GLuint g_LightTileTexture;
GLuint g_LightIndexTexture;
uint32_t g_LightTileWidth;
uint32_t g_LightTileHeight;

struct LightDesc
{
	float Pos[4];
	float Color[4];
};

struct LightTileIndex
{
	int16_t nBase;
	int16_t nCount;
};
struct LightIndex
{
	int16_t nIndex;
	int16_t nDummy;
};

LightIndex g_LightIndex[MAX_LIGHT_INDEX];
LightTileIndex g_LightTileInfo[(MAX_HEIGHT/TILE_SIZE)*(MAX_WIDTH/TILE_SIZE)];
LightDesc g_LightBuffer[MAX_NUM_LIGHTS];

SWorldState g_WorldState;
SEditorState g_EditorState;

void DebugRender(m mprj)
{
	if(g_EditorState.pSelected)
	{
		ZDEBUG_DRAWBOUNDS(g_EditorState.pSelected->mObjectToWorld, g_EditorState.pSelected->vSize, g_EditorState.bLockSelection ? (0xffffff00) : -1);
		uplotfnxt("SELECTION %p", g_EditorState.pSelected);
	}

	DebugDrawFlush(mprj);
}

void EditorStateInit()
{
	memset(&g_EditorState, 0, sizeof(g_EditorState));
	g_EditorState.Manipulators[0] = new ManipulatorTranslate();
	g_EditorState.Manipulators[1] = new ManipulatorRotate();
}

struct ShadowMap
{
	GLuint FrameBufferId;
	GLuint TextureId;
	m mprjview;
	m mprjviewtex;
};

ShadowMap g_SM;

ShadowMap AllocateShadowMap()
{
	CheckGLError();
	ShadowMap r;
	glGenTextures(1, &r.TextureId);
	glBindTexture(GL_TEXTURE_2D, r.TextureId);
	CheckGLError();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	CheckGLError();

	CheckGLError();
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	CheckGLError();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	
	CheckGLError();
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWMAP_SIZE, SHADOWMAP_SIZE, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &r.FrameBufferId);

	glBindFramebuffer(GL_FRAMEBUFFER, r.FrameBufferId);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	CheckGLError();

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, r.TextureId, 0);



	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
	{
		uprintf("STATUS IS %d\n", status);


		ZBREAK();
	}


	CheckGLError();
	return r;
}


void RenderShadowMap(ShadowMap& SM, v3 vEye, v3 vDir, v3* vCorners)
{
	MICROPROFILE_SCOPEGPUI("GPU", "Shadowmap", 0xff0099);
	v3 vDirection = v3normalize(v3init(0.59188753, -0.74972421, 0.29594377));
	v3 vRight = v3normalize(v3cross(v3init(0,1,0), vDirection));
	v3 vUp = v3normalize(v3cross(vRight, vDirection));
	m mview = mcreate(vDirection, vRight, v3zero());
	m mviewinverse = maffineinverse(mview);

	{
		v3 vDirLight = mrotate(mview, vDir);
		v3 vPosLight = mtransform(mview, vEye);		
		v3 vCornerLocal[4];
		v3 vDirMin = vPosLight;
		v3 vDirMax = vPosLight;
		for(int i = 0; i < 4; ++i)
		{			
			vCornerLocal[i] = mtransform(mview, vCorners[i]);
			vDirMin = v3min(vDirMin, vCornerLocal[i]);
			vDirMax = v3max(vDirMax, vCornerLocal[i]);
		}
		v3 vCenter = 0.5f * (vDirMax + vDirMin);
		m mtrans = mid();
		mtrans.trans = v4init(-vCenter, 1.f);
		//mtrans = mtransform(mviewinverse(
		mview = mmult(mtrans, mview);
		v3 AABBLocal[8];
		AABBLocal[0] = v3init(vDirMax.x, vDirMax.y, vDirMax.z);
		AABBLocal[1] = v3init(vDirMax.x, vDirMin.y, vDirMax.z);
		AABBLocal[2] = v3init(vDirMin.x, vDirMin.y, vDirMax.z);
		AABBLocal[3] = v3init(vDirMin.x, vDirMax.y, vDirMax.z);
		AABBLocal[4] = v3init(vDirMax.x, vDirMax.y, vDirMin.z);
		AABBLocal[5] = v3init(vDirMax.x, vDirMin.y, vDirMin.z);
		AABBLocal[6] = v3init(vDirMin.x, vDirMin.y, vDirMin.z);
		AABBLocal[7] = v3init(vDirMin.x, vDirMax.y, vDirMin.z);
		for(int i = 0; i < 8; ++i)
		{
			AABBLocal[i] = mtransform(mviewinverse, AABBLocal[i]);
		}
		// ZDEBUG_DRAWLINE(AABBLocal[0], AABBLocal[1], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[1], AABBLocal[2], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[2], AABBLocal[3], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[3], AABBLocal[0], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[4], AABBLocal[5], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[5], AABBLocal[6], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[6], AABBLocal[7], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[7], AABBLocal[4], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[0], AABBLocal[4], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[1], AABBLocal[5], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[2], AABBLocal[6], -1, 0);
		// ZDEBUG_DRAWLINE(AABBLocal[3], AABBLocal[7], -1, 0);

	}
	const int size = 120;
	m mprj = morthogl(-size, size, -size, size, -1500.f, 1500.f);
	m moffset = mid();
	moffset.x = v4init(0.5, 0.0, 0.0, 0.0f);
	moffset.y = v4init(0.0, 0.5, 0.0, 0.0f);
	moffset.z = v4init(0.0, 0.0, 0.5, 0.0f);

	moffset.trans.x = 0.5f;
	moffset.trans.y = 0.5f;
	moffset.trans.z = 0.5f;
	SM.mprjview = mmult(mprj, mview);
	SM.mprjviewtex = mmult(moffset, mmult(mprj, mview));


	CheckGLError();
	uplotfnxt("render shadowmap");
	glBindFramebuffer(GL_FRAMEBUFFER, SM.FrameBufferId);
	glViewport(0, 0, SHADOWMAP_SIZE, SHADOWMAP_SIZE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
	glColorMask(0,0,0,0);
	glClear(GL_DEPTH_BUFFER_BIT);


	ShaderUse(VS_SHADOWMAP, PS_SHADOWMAP);
	SHADER_SET("ProjectionMatrix", &SM.mprjview);

	WorldDrawObjects(nullptr);

	CheckGLError();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glColorMask(1,1,1,1);
	glDepthMask(1);
	glViewport(0, 0, g_Width, g_Height);
	CheckGLError();
}
void DoBuildBsp(SOccluderBspViewDesc& ViewDesc, int nDebugIndex)
{
	MICROPROFILE_SCOPEI("CullTest", "Build", 0xff00ff00);
	uint32 nNumPlaneOccluders = 0, nNumBoxOccluders = 0;
	for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	{
		if(g_WorldState.WorldObjects[i].nFlags & SObject::OCCLUDER_BOX)
		{
			nNumBoxOccluders ++;
		}
	}
	for(uint32 i = 0; i < g_WorldState.nNumOccluders; ++i)
	{
		if(g_WorldState.Occluders[i].nFlags & SObject::OCCLUDER_BOX)
		{
			nNumPlaneOccluders ++;
		}
		else
		{
			ZBREAK();
		}
	}

	SOccluderDesc** pPlaneOccluders = (SOccluderDesc**)alloca(sizeof(SOccluderDesc*)*nNumPlaneOccluders);
	SOccluderDesc** pBoxOccluders = (SOccluderDesc**)alloca(sizeof(SOccluderDesc*)*nNumBoxOccluders);
	int PlaneIdx = 0, BoxIdx = 0;
	for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	{
		if(g_WorldState.WorldObjects[i].nFlags & SObject::OCCLUDER_BOX)
		{
			pBoxOccluders[BoxIdx++] = (SOccluderDesc*)&g_WorldState.WorldObjects[i].mObjectToWorld;
		}
	}

	for(uint32 i = 0; i < g_WorldState.nNumOccluders; ++i)
	{
		if(g_WorldState.Occluders[i].nFlags & SObject::OCCLUDER_BOX)
		{
			pPlaneOccluders[PlaneIdx++] = (SOccluderDesc*)&g_WorldState.Occluders[i].mObjectToWorld;
		}
		else
		{
			ZBREAK();
		}
	}

	if(nDebugIndex >= 0)
	{
		uprintf("BUILDING DEBUG MASK\n");
		memset(g_DebugMask, 0, sizeof(g_DebugMask));
		BspBuildDebugSearch(g_Bsp, pPlaneOccluders, nNumPlaneOccluders, pBoxOccluders, nNumBoxOccluders, ViewDesc,
			(SOccluderDesc*)&g_WorldState.WorldObjects[nDebugIndex],
			&g_DebugMask[0],
			sizeof(g_DebugMask));

		for(int i = 0; i < sizeof(g_DebugMask); ++i)
		{
			if(g_DebugMask[i])
			{
				uprintf("** DEBUG MASK %d\n", i);
			}
		}

	}
	else
	{
		bool* pDebugMask = g_nDebugMode ? g_DebugMask : 0;
		BspBuild(g_Bsp, 
			pPlaneOccluders, nNumPlaneOccluders,
			pBoxOccluders, nNumBoxOccluders,
			ViewDesc,
			pDebugMask );
	}
}
void RunPlainTest()
{
	bool* bCulled = (bool*)alloca(g_WorldState.nNumWorldObjects);
	uprintf("\nbase test\n");
	StartTest();
	while(g_nRunTest)
	{
		_mm_setcsr( _mm_getcsr() | 0x8040 );
		v3 vPosition, vDir, vRight;
		RunTest(vPosition, vDir, vRight);
		SCameraState Camera;
		SetupCameraState(Camera, vPosition, vDir, vRight);
		SOccluderBspViewDesc ViewDesc;
		ViewDesc.vOrigin = Camera.vPosition;
		ViewDesc.vDirection = Camera.vDir;
		ViewDesc.vRight = Camera.vRight;
		ViewDesc.fFovY = Camera.fFovY;
		ViewDesc.fAspect = (float)g_Height / (float)g_Width;
		ViewDesc.fZNear = Camera.fNear;
		ViewDesc.nNodeCap = g_nBspNodeCap;
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		{
			DoBuildBsp(ViewDesc);
		}

		memset(bCulled, 0, g_WorldState.nNumWorldObjects);
		{
			MICROPROFILE_SCOPEI("CullTest", "Cull", 0xff00ff00);
			CullObjects(g_WorldState.nNumWorldObjects, bCulled);
		}
		MicroProfileFlip();
	}
}
void RunGridTest(const char* pName, SWorldGrid& Grid)
{
	bool* bCulled = (bool*)alloca(g_WorldState.nNumWorldObjects);

	uprintf("\n%s test\n", pName);
	StartTest();
	while(g_nRunTest)
	{
		_mm_setcsr( _mm_getcsr() | 0x8040 );
		v3 vPosition, vDir, vRight;
		RunTest(vPosition, vDir, vRight);
		SCameraState Camera;
		SetupCameraState(Camera, vPosition, vDir, vRight);
		SOccluderBspViewDesc ViewDesc;
		ViewDesc.vOrigin = Camera.vPosition;
		ViewDesc.vDirection = Camera.vDir;
		ViewDesc.vRight = Camera.vRight;
		ViewDesc.fFovY = Camera.fFovY;
		ViewDesc.fAspect = (float)g_Height / (float)g_Width;
		ViewDesc.fZNear = Camera.fNear;
		ViewDesc.nNodeCap = g_nBspNodeCap;
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		{
			DoBuildBsp(ViewDesc);
		}

		memset(bCulled, 0, g_WorldState.nNumWorldObjects);
		{
			MICROPROFILE_SCOPEI("CullTest", "Cull", 0xff00ff00);
			CullObjectsFast(g_WorldState.nNumWorldObjects, bCulled, Grid);
		}
		MicroProfileFlip();
	}	
}
void RunTestOnly()
{
	#if 0 == QUICK_PERF
	return;
	#endif
	WorldInitOcclusionTest();



	RunPlainTest();
	RunGridTest("Grid3", g_Grid3);
	RunGridTest("Grid5", g_Grid5);
	RunGridTest("Grid8", g_Grid8);
	RunGridTest("Grid10", g_Grid10);
	RunGridTest("Grid12", g_Grid12);
	RunGridTest("Grid15", g_Grid15);
	RunGridTest("Grid20", g_Grid20);



}



void WorldInit()
{
	g_WorldState.Occluders[0].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[0].mObjectToWorld.trans = v4init(3.f,0.f,0.1f, 1.f);
	g_WorldState.Occluders[0].vSize = v3init(0.5f, 0.5f, 0.f);

	g_WorldState.Occluders[1].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[1].mObjectToWorld.trans = v4init(2.5f,0.f,-0.5f, 1.f);
	//g_WorldState.Occluders[1].mObjectToWorld.trans = v4init(1.5f,0.f,-0.5f, 1.f);
	g_WorldState.Occluders[1].vSize = v3init(0.75f, 0.75f, 0);

	g_WorldState.Occluders[2].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[2].mObjectToWorld.trans = v4init(2.6f,0.f,-0.5f, 1.f);
	g_WorldState.Occluders[2].vSize = v3init(0.25f, 0.25f, 0);
	g_WorldState.nNumOccluders = 3;
	g_WorldState.nNumOccluders = 0;

	if(1)
	{
		g_WorldState.nNumWorldObjects = 0;
		WorldInitOcclusionTest();
	}
	else
	{
		g_WorldState.WorldObjects[0].mObjectToWorld = mrotatey(90*TORAD);
		g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f,0.f,-0.5f, 1.f);
		g_WorldState.WorldObjects[0].vSize = v3init(0.25f, 0.2f, 0.2f); 
		g_WorldState.WorldObjects[0].nFlags = 0;
		g_WorldState.WorldObjects[0].nFlags |= SObject::OCCLUDER_BOX; 

		g_WorldState.WorldObjects[1].mObjectToWorld = mrotatey(90*TORAD);
		g_WorldState.WorldObjects[1].mObjectToWorld.trans = v4init(3.f,0.f,-1.5f, 1.f);
		g_WorldState.WorldObjects[1].vSize = v3init(0.25f, 0.2f, 0.2f)* 0.5f; 
		g_WorldState.WorldObjects[1].nFlags = 0; 
		g_WorldState.WorldObjects[1].nFlags |= SObject::OCCLUSION_TEST; 
		g_WorldState.nNumWorldObjects = 2;
	//	g_WorldState.nNumWorldObjects = 0;
	}
	if(0)
	{
		int idx = 0;
		#define GRID_SIZE 10
		for(uint32 i = 0; i < GRID_SIZE; i++)
		{
		for(uint32 j = 0; j < GRID_SIZE; j++)
		{
		for(uint32 k = 0; k < GRID_SIZE; k++)
		{
			v3 vPos = v3init(i,j,k) / (GRID_SIZE-1.f);
			vPos.z += j * 0.01f;
			vPos -= 0.5f;
			vPos *= 8.f;
			vPos += 0.03f;
			vPos.x += 2.0f;
			g_WorldState.WorldObjects[idx].mObjectToWorld = mid();
			g_WorldState.WorldObjects[idx].mObjectToWorld.trans = v4init(vPos,1.f);
			g_WorldState.WorldObjects[idx].vSize = v3init(0.25f, 0.2f, 0.2); 

			idx++;
		}}}
		g_WorldState.nNumWorldObjects = idx;
		for(int i = 0; i < g_WorldState.nNumWorldObjects; ++i)
		{
			PhysicsAddObjectBox(&g_WorldState.WorldObjects[i]);
		}
	}


#if 0
	g_WorldState.nNumLights = 1;
	for(int i = 0; i < g_WorldState.nNumLights; ++i)
	{
		m mat = mid();
		mat.trans.y = -3.f;
		// mat.trans.x = frandrange(-5.f, 5.f);
		// mat.trans.y = frandrange(-2.7f, -3.f);
		// mat.trans.z = frandrange(-5.f, 5.f);
//		uprintf("LIGHT AT %f %f %f\n", mat.trans.x, mat.trans.y, mat.trans.z);
		g_WorldState.Lights[i].mObjectToWorld = mat;
		g_WorldState.Lights[i].nColor = randcolor() | 0xff000000;
		g_WorldState.Lights[i].fRadius = 2.f;
	}
#else
	g_WorldState.nNumLights = 15;
	for(int i = 0; i < g_WorldState.nNumLights; ++i)
	{
		ZDEBUG_DRAWSPHERE(g_WorldState.Lights[i].mObjectToWorld.trans.tov3(), 2.f, -1);
		m mat = mid();
		mat.trans.x = frandrange(-5.f, 5.f);
		mat.trans.y = frandrange(-2.7f, -3.f);
		mat.trans.z = frandrange(-5.f, 5.f);
//		uprintf("LIGHT AT %f %f %f\n", mat.trans.x, mat.trans.y, mat.trans.z);
		g_WorldState.Lights[i].mObjectToWorld = mat;
		g_WorldState.Lights[i].nColor = randcolor() | 0xff000000;
		g_WorldState.Lights[i].fRadius = 2.f;
	}
#endif


	g_EditorState.pSelected = 0;


	CheckGLError();
	glGenTextures(1, &g_LightTexture);
	glGenTextures(1, &g_LightTileTexture);
	glGenTextures(1, &g_LightIndexTexture);
	memset(&g_LightIndex, 0, sizeof(g_LightIndex));
	memset(&g_LightBuffer, 0, sizeof(g_LightBuffer));
	glBindTexture(GL_TEXTURE_2D, g_LightTexture);
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, MAX_NUM_LIGHTS*2, 1, 0, GL_RGBA, GL_FLOAT, &g_LightBuffer);


	glBindTexture(GL_TEXTURE_2D, g_LightTileTexture);
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    g_LightTileWidth = g_Width / TILE_SIZE;
    g_LightTileHeight = g_Height / TILE_SIZE;
    ZASSERT(g_LightTileWidth * TILE_SIZE == g_Width);
    ZASSERT(g_LightTileHeight * TILE_SIZE == g_Height);
    CheckGLError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, g_LightTileWidth, g_LightTileHeight, 0, GL_RGBA, GL_SHORT, 0);
    CheckGLError();
    

	glBindTexture(GL_TEXTURE_2D, g_LightIndexTexture);
	{
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);     
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16, LIGHT_INDEX_SIZE, LIGHT_INDEX_SIZE, 0, GL_RGBA, GL_SHORT, 0);
    CheckGLError();

    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_LightTileWidth, g_LightTileHeight, 0, GL_RGBA, GL_SHORT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    CheckGLError();

}


void WorldDrawCulledObjects(bool* bCulled)
{
	MICROPROFILE_SCOPEI("lala", "WorldDrawCulledObjects", 0xff00ffff);
	ZASSERT(bCulled);
	int nLocSize = ShaderGetLocation("Size");
	int nLocModelView = ShaderGetLocation("ModelViewMatrix");
	int nLocColor = ShaderGetLocation("ConstColor");
	for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	{
		if(bCulled)
		{
			if(bCulled[i])
			{
				ZDEBUG_DRAWBOX(g_WorldState.WorldObjects[i].mObjectToWorld, g_WorldState.WorldObjects[i].mObjectToWorld.trans.tov3(), g_WorldState.WorldObjects[i].vSize, 0xffff0000, 0);
			}
		}
	}
}

void WorldDrawObjects(bool* bCulled)
{
	MICROPROFILE_SCOPEI("lala", "WorldDrawObjects", 0xff00ffff);
	int nLocSize = ShaderGetLocation("Size");
	int nLocModelView = ShaderGetLocation("ModelViewMatrix");
	int nLocColor = ShaderGetLocation("ConstColor");
	for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	{
		if(bCulled)
		{
			if(bCulled[i])
			{
				// ZDEBUG_DRAWBOX(g_WorldState.WorldObjects[i].mObjectToWorld, g_WorldState.WorldObjects[i].mObjectToWorld.trans.tov3(), g_WorldState.WorldObjects[i].vSize, 0xffff0000, 0);
			}
			else
			{
				ShaderSetUniform(nLocSize, g_WorldState.WorldObjects[i].vSize);
				ShaderSetUniform(nLocModelView, &g_WorldState.WorldObjects[i].mObjectToWorld);
				ShaderSetUniform(nLocColor, v3fromcolor(g_WorldState.WorldObjects[i].nColor));
				MeshDraw(GetBaseMesh(MESH_BOX_FLAT));
			}
		}
		else
		{
			ShaderSetUniform(nLocSize, g_WorldState.WorldObjects[i].vSize);
			ShaderSetUniform(nLocModelView, &g_WorldState.WorldObjects[i].mObjectToWorld);
			MeshDraw(GetBaseMesh(MESH_BOX_FLAT));
		}
	}
}


int g_nSimulate = 0;
void WorldRender()
{
	MICROPROFILE_SCOPEI("MAIN", "WorldRender", 0xff44dddd);
	if(g_KeyboardState.keys[SDL_SCANCODE_F1] & BUTTON_RELEASED)
	{
		g_nSimulate = !g_nSimulate;
	}

	if(g_KeyboardState.keys[SDL_SCANCODE_P] & BUTTON_RELEASED)
	{
		g_lShowDebug = !g_lShowDebug;
	}

	static int incfoo = 1;
	if(g_nSimulate)
	{
		ZBREAK();
		g_WorldState.WorldObjects[0].mObjectToWorld = mmult(g_WorldState.WorldObjects[0].mObjectToWorld, mrotatey(0.5f*TORAD));
		g_WorldState.Occluders[0].mObjectToWorld = mmult(g_WorldState.Occluders[0].mObjectToWorld, mrotatez(0.3f*TORAD));
		static float xx = 0;
		if(incfoo)
			xx += 0.005f;
		g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f, 0.f, sin(xx)*2, 1.f);
	}

	uint32 nNumOccluders = g_WorldState.nNumOccluders;
	static float foo = 0;
	
	if(g_KeyboardState.keys[SDL_SCANCODE_Y] & BUTTON_RELEASED)
	{
		incfoo = !incfoo;
	}

	const int nCanVerify = g_CameraSourceBsp == g_CameraSource && g_nTestFail == 0;
	const SCameraState& CameraBsp = GetCamera(g_CameraSourceBsp);
	const SCameraState& Camera = GetCamera(g_CameraSource);

	bool bShift = 0 != ((g_KeyboardState.keys[SDL_SCANCODE_RSHIFT]|g_KeyboardState.keys[SDL_SCANCODE_LSHIFT]) & BUTTON_DOWN);
	if(g_KeyboardState.keys[SDL_SCANCODE_L] & BUTTON_RELEASED)
	{
		// if(bShift)
		// {
			// g_WorldState.
		// 	g_WorldState.Camera.vPosition = vLockedCamPos;
		// 	g_WorldState.Camera.vDir = vLockedCamDir;
		// 	g_WorldState.Camera.vRight = vLockedCamRight;
		// 	uprintf("g_WorldState.Camera.vPosition = v3init(%f,%f,%f);\n", vLockedCamPos.x, vLockedCamPos.y, vLockedCamPos.z);
		// 	uprintf("g_WorldState.Camera.vDir = v3init(%f,%f,%f);\n", vLockedCamDir.x, vLockedCamDir.y, vLockedCamDir.z);
		// 	uprintf("g_WorldState.Camera.vRight = v3init(%f,%f,%f)\n;", vLockedCamRight.x, vLockedCamRight.y, vLockedCamRight.z);
		// }
		// else
		// 	g_WorldState.Camera.vPosition = v3init(0,sin(foo), 0);;
	}


	// v3 vPos = vLockedCamPos;
	// v3 vDir = vLockedCamDir;
	// v3 vRight = vLockedCamRight;
	v3 vPos = CameraBsp.vPosition;
	v3 vDir = CameraBsp.vDir;
	v3 vRight = CameraBsp.vRight;

	// if(g_nUseDebugCameraPos)
	// {
	// 	vPos = v3init(0,sin(foo), 0);
	// 	vDir = v3normalize(v3init(3,0,0) - vPos);
	// 	vRight = v3init(0,-1,0);
	// 	{
	// 		v3 vUp = v3normalize(v3cross(vRight, vDir));
	// 		vRight = v3normalize(v3cross(vDir, vUp));
	// 	}
	// }
	if(g_KeyboardState.keys[SDL_SCANCODE_Q]&BUTTON_RELEASED)
	{
		g_nBspNodeCap <<= 1;
		if(g_nBspNodeCap > 1024)
			g_nBspNodeCap = 2;
	}

		
	
	ZDEBUG_DRAWBOX(mid(), vPos, v3rep(0.02f), -1,1);
	// ZDEBUG_DRAWBOX(mid(), vPos, v3rep(0.04f), -1,1);
	// ZDEBUG_DRAWBOX(mid(), vPos, v3rep(0.06f), -1,1);
	SOccluderBspViewDesc ViewDesc;
	ViewDesc.vOrigin = vPos;
	ViewDesc.vDirection = vDir;
	ViewDesc.vRight = vRight;
	ViewDesc.fFovY = CameraBsp.fFovY;
	ViewDesc.fAspect = (float)g_Height / (float)g_Width;
	ViewDesc.fZNear = CameraBsp.fNear;
	ViewDesc.nNodeCap = g_nBspNodeCap;
	uplotfnxt("bsp pos is %f %f %f", ViewDesc.vOrigin.x, ViewDesc.vOrigin.y, ViewDesc.vOrigin.z);
	uplotfnxt("DEBUG POS %f %f %f", vPos.x, vPos.y, vPos.z);

	//fesetenv(FE_DFL_DISABLE_SSE_DENORMS_ENV)

	int nDumpFrame = 0;


	{{
		if(g_KeyboardState.keys[SDL_SCANCODE_F3] & BUTTON_RELEASED)
		{
			BspDebugNextDrawMode(g_Bsp);
		}

		if(g_KeyboardState.keys[SDL_SCANCODE_F4] & BUTTON_RELEASED)
		{
			BspDebugNextDrawClipResult(g_Bsp);
		}

		if(g_KeyboardState.keys[SDL_SCANCODE_V] & BUTTON_RELEASED)
		{
			BspDebugNextPoly(g_Bsp);
		}
		if(g_KeyboardState.keys[SDL_SCANCODE_C] & BUTTON_RELEASED)
		{
			BspDebugPreviousPoly(g_Bsp);
		}
		if(g_KeyboardState.keys[SDL_SCANCODE_F8] & BUTTON_RELEASED)
			BspDebugShowClipLevelNext(g_Bsp);
		if(g_KeyboardState.keys[SDL_SCANCODE_F7] & BUTTON_RELEASED)
			BspDebugShowClipLevelPrevious(g_Bsp);
		static int debugplane = -1;
		if(g_KeyboardState.keys[SDL_SCANCODE_F12] & BUTTON_RELEASED)
		{
			debugplane++;
		}
		if(g_KeyboardState.keys[SDL_SCANCODE_F11] & BUTTON_RELEASED)
		{
			debugplane--;
		}
		uplotfnxt("debug plane %d", debugplane);
		BspDebugPlane(g_Bsp, debugplane);
	// void BspDebugPlane(SOccluderBsp* pBsp, int nPlane)
		if(g_KeyboardState.keys[SDL_SCANCODE_F5] & BUTTON_RELEASED)
		{
			g_nDebugCullDraw = (g_nDebugCullDraw+1) % 4;
		}



		if(g_KeyboardState.keys[SDL_SCANCODE_END] & BUTTON_RELEASED)
			BspDebugToggleInsideOutside(g_Bsp);

		// f(g_KeyboardState.keys[SDL_SCANCODE_F5] & BUTTON_RELEASED)
		// 	g_nShowClipLevel++;
		// if(g_KeyboardState.keys[SDL_SCANCODE_F6] & BUTTON_RELEASED)
		// 	g_nShowClipLevel--;


//		if((int)g_nShowClipLevel >= 0)
		{

			bool bShift = 0 != ((g_KeyboardState.keys[SDL_SCANCODE_RSHIFT]|g_KeyboardState.keys[SDL_SCANCODE_LSHIFT]) & BUTTON_DOWN);

			if(!bShift && (g_KeyboardState.keys[SDL_SCANCODE_F4] & BUTTON_RELEASED))
				BspDebugClipLevelSubNext(g_Bsp);
			if(bShift && 0 != (g_KeyboardState.keys[SDL_SCANCODE_F4] & BUTTON_RELEASED))
				BspDebugClipLevelSubPrev(g_Bsp);


		// uint32 g_nShowClipLevelSubCounter = 0;
		// uint32 g_nShowClipLevelSub = 0
		// uint32 g_nShowClipLevelMax = 0;
		}

		if(g_KeyboardState.keys[SDL_SCANCODE_F10] & BUTTON_RELEASED)
		{
			nDumpFrame = 1;
		}

	}}

	
	_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);

	{
		MICROPROFILE_SCOPEI("CullTest", "Build", 0xff00ff00);
		DoBuildBsp(ViewDesc);
	}
	if(nDumpFrame)
	{
		BspDebugDumpFrame(g_Bsp, 2);
	}

	uint32 nNumObjects = g_WorldState.nNumWorldObjects;
	bool* bCulled = (bool*)alloca(nNumObjects);
	bool* bCulledFast = (bool*)alloca(nNumObjects);
	bool* bCulledRef = (bool*)alloca(nNumObjects);
	int* nCulledRef = (int*)alloca(nNumObjects*4);
	bCulled[0] = false;
	memset(bCulled, 0, nNumObjects);
	memset(bCulledFast, 0, nNumObjects);
	memset(bCulledRef, 0, nNumObjects);
	memset(nCulledRef, 0, nNumObjects*4);
	uint32_t nCulled = 0, nCulledFast = 0;

	SOccluderBspStats Stats, StatsFast;

	int nNoRef = 0;
	{
		{
			MICROPROFILE_SCOPEI("CullTest", "CullFast", 0xff00ff00);
			BspClearCullStats(g_Bsp);
			nCulledFast = CullObjectsFast(nNumObjects, bCulledFast, g_Grid10);
			BspGetStats(g_Bsp, &StatsFast);

		}
		{
			MICROPROFILE_SCOPEI("CullTest", "Cull", 0xff00ff00);
			BspClearCullStats(g_Bsp);
			nCulled = CullObjects(nNumObjects, bCulled);
			BspGetStats(g_Bsp, &Stats);
		}
		CompareCullResult(nNumObjects, bCulled, bCulledFast);
	}
	uplotfnxt("********************BSP STATS********************");
	uplotfnxt("** TESTED %5d VISIBLE %5d SUBTEST %d CHILDBSP %d CHILDBSPVIS %d", Stats.nNumObjectsTested, Stats.nNumObjectsTestedVisible, Stats.nNunSubBspTests, Stats.nNumChildBspsCreated, Stats.nNumChildBspsVisible);
	uplotfnxt("********************BSP FAST STATS********************");
	uplotfnxt("** TESTED %5d VISIBLE %5d SUBTEST %d CHILDBSP %d CHILDBSPVIS %d", StatsFast.nNumObjectsTested, StatsFast.nNumObjectsTestedVisible, StatsFast.nNunSubBspTests, StatsFast.nNumChildBspsCreated, StatsFast.nNumChildBspsVisible);

	static v3 LightPos = v3init(0,-2.7,0);
	static v3 LightColor = v3init(0.9, 0.5, 0.9);
	static v3 LightPos0 = v3init(0,-2.7,0);
	static v3 LightColor0 = v3init(0.3, 1.0, 0.3);

	LightPos.z = (sin(foo*10)) * 5;
	LightPos0.x = (sin(foo*10)) * 5;

#if 1
	g_WorldState.Lights[0].mObjectToWorld.trans.z = (sin(foo*5.5)) * 5;
	g_WorldState.Lights[1].mObjectToWorld.trans.x = (cos((foo+2)*3)) * 5;
#endif

	#define BUFFER_SIZE (512*512)
	static v3 PointBuffer[BUFFER_SIZE];
	static v3 DumpPlaneNormal0, DumpPlaneNormal1, DumpPlaneNormal2, DumpPlaneNormal3;
	static v3 DumpPlanePos0, DumpPlanePos1, DumpPlanePos2, DumpPlanePos3;
	static int nNumPoints = 0;
	bool bDumpPoints = false;
	if(g_KeyboardState.keys[SDL_SCANCODE_B]&BUTTON_RELEASED)
	{
		bDumpPoints = true;
		nNumPoints = 0;
	}

	m mprj = (Camera.mprj);
	m mprjinv = minverse(Camera.mprj);



	int nLightIndex = 0;

	m mcliptoworld = mmult(Camera.mviewinv, Camera.mprjinv);
	v3 veye = Camera.vPosition;
	int nMaxTileLights = 0;
	if(0)
	for(int i = 0; i < g_LightTileWidth; ++i)
	{
			MICROPROFILE_SCOPEI("MAIN", "Tile Stuff", 0xff44dddd);
			//g_LightTileWidth

		float x0 = 2*(float)i / (g_LightTileWidth)-1;
		float x1 = 2*(float)(i+1) / (g_LightTileWidth)-1;
		for(int j = 0; j < g_LightTileHeight; ++j)
		{
			float y0 = 2*(float)j / (g_LightTileHeight)-1;
			float y1 = 2*(float)(j+1) / (g_LightTileHeight)-1;
			v4 p0 = mtransform(mcliptoworld, v4init(x0,y0,-1.f,1));
			v4 p1 = mtransform(mcliptoworld, v4init(x1,y0,-1.f,1));
			v4 p2 = mtransform(mcliptoworld, v4init(x1,y1,-1.f,1));
			v4 p3 = mtransform(mcliptoworld, v4init(x0,y1,-1.f,1));
			v3 _p0 = p0.tov3() / p0.w;
			v3 _p1 = p1.tov3() / p1.w;
			v3 _p2 = p2.tov3() / p2.w;
			v3 _p3 = p3.tov3() / p3.w;

			LightTileIndex& Index = g_LightTileInfo[i+j*g_LightTileWidth];
			Index.nBase = nLightIndex;
			Index.nCount = 0;
			v3 vNormal0 = -v3normalize(v3cross(_p0-_p1, _p1 - veye));
			v3 vNormal1 = -v3normalize(v3cross(_p1-_p2, _p2 - veye));
			v3 vNormal2 = -v3normalize(v3cross(_p2-_p3, _p3 - veye));
			v3 vNormal3 = -v3normalize(v3cross(_p3-_p0, _p0 - veye));
			v4 plane0 = v4makeplane(_p0, vNormal0);
			v4 plane1 = v4makeplane(_p1, vNormal1);
			v4 plane2 = v4makeplane(_p2, vNormal2);
			v4 plane3 = v4makeplane(_p3, vNormal3);

			for(int k = 0; k < g_WorldState.nNumLights && nLightIndex < MAX_LIGHT_INDEX; ++k)
			{
				v4 center = g_WorldState.Lights[k].mObjectToWorld.trans;
				float radius = g_WorldState.Lights[k].fRadius;
				center.w = 1.f;
				//ZASSERT(center.w == 1.f);
				float fDot0 = v4dot(plane0, center);
				float fDot1 = v4dot(plane1, center);
				float fDot2 = v4dot(plane2, center);
				float fDot3 = v4dot(plane3, center);

				
				if( fDot0 + radius >= 0 &&
					fDot1 + radius >= 0 &&
					fDot2 + radius >= 0 &&
					fDot3 + radius >= 0)
				{
				//	ZBREAK();
					int idx = nLightIndex++;

					g_LightIndex[idx].nIndex = k;
					g_LightIndex[idx].nDummy = k;
					Index.nCount++;
					///ZASSERT(Index.nCount == (nLightIndex - Index.nBase));
					//ZASSERT(nLightIndex < MAX_LIGHT_INDEX);
				}
			}
			// if(Index.nCount)
			// 	uprintf("INDEX AT %d %d has %d\n", i, j, Index.nCount);
			nMaxTileLights = Max((int)Index.nCount, nMaxTileLights);
			//Index.nCount = j % 1;
//			Index.nBase = i % 10;

			if(bDumpPoints)
			{
//				uprintf("DUMP %f %f %f\n", p0.x, p0.y, p0.z);
				if(i == 50 && j == 50)
				{

					DumpPlaneNormal0 = vNormal0;
					DumpPlanePos0 = _p0;
					DumpPlaneNormal1 = vNormal1;
					DumpPlanePos1 = _p1;
					DumpPlaneNormal2 = vNormal2;
					DumpPlanePos2 = _p2;
					DumpPlaneNormal3 = vNormal3;
					DumpPlanePos3 = _p3;
					//ZBREAK();

				}
				PointBuffer[nNumPoints++] = p0.tov3();
				PointBuffer[nNumPoints++] = p1.tov3();
				PointBuffer[nNumPoints++] = p2.tov3();
				PointBuffer[nNumPoints++] = p3.tov3();
				ZASSERT(nNumPoints < BUFFER_SIZE);
			}
		}
	}
	if(nNumPoints)
	{
		ZASSERT(nNumPoints == 4 * g_LightTileWidth * g_LightTileHeight);
		for(int i = 0; i < nNumPoints;i+=4)
		{
			ZDEBUG_DRAWLINE(PointBuffer[i], PointBuffer[i+1], -1, 0);
			ZDEBUG_DRAWLINE(PointBuffer[i+3], PointBuffer[i], -1, 0);
		}

		ZDEBUG_DRAWPLANE(DumpPlaneNormal0, DumpPlanePos0, -1);
		ZDEBUG_DRAWPLANE(DumpPlaneNormal1, DumpPlanePos1, -1);
		ZDEBUG_DRAWPLANE(DumpPlaneNormal2, DumpPlanePos2, -1);
		ZDEBUG_DRAWPLANE(DumpPlaneNormal3, DumpPlanePos3, -1);
	}
	for(int i = 0; i < g_WorldState.nNumLights; ++ i)
	{
		g_LightBuffer[i].Pos[0] = g_WorldState.Lights[i].mObjectToWorld.trans.x;
		g_LightBuffer[i].Pos[1] = g_WorldState.Lights[i].mObjectToWorld.trans.y;
		g_LightBuffer[i].Pos[2] = g_WorldState.Lights[i].mObjectToWorld.trans.z;
		v3 col = v3fromcolor(g_WorldState.Lights[i].nColor);
		g_LightBuffer[i].Color[0] = col.x;
		g_LightBuffer[i].Color[1] = col.y;
		g_LightBuffer[i].Color[2] = col.z;
		g_LightBuffer[i].Color[3] = g_WorldState.Lights[i].fRadius;
	}



	{
		MICROPROFILE_SCOPEI("MAIN", "RenderShadowMap", 0xff44dddd);

		v3 vFrustumCorners[4];
		SOccluderBspViewDesc Desc = ViewDesc;
		float fAngle = (Desc.fFovY * PI / 180.f) / 2.f;
		float fCA = cosf(fAngle);
		float fSA = sinf(fAngle);
		float fY = fSA / fCA;
		float fX = fY / Desc.fAspect;
		const float fDist = 200;
		const v3 vOrigin = Desc.vOrigin;
		const v3 vDirection = Desc.vDirection;
		const v3 vRight = Desc.vRight;
		const v3 vUp = v3normalize(v3cross(vRight, vDirection));

		vFrustumCorners[0] = fDist * (vDirection + fY * vUp + fX * vRight) + vOrigin;
		vFrustumCorners[1] = fDist * (vDirection - fY * vUp + fX * vRight) + vOrigin;
		vFrustumCorners[2] = fDist * (vDirection - fY * vUp - fX * vRight) + vOrigin;
		vFrustumCorners[3] = fDist * (vDirection + fY * vUp - fX * vRight) + vOrigin;



		ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[0], 0, true);
		ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[1], 0, true);
		ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[2], 0, true);
		ZDEBUG_DRAWLINE(vOrigin, vFrustumCorners[3], 0, true);
		ZDEBUG_DRAWLINE(vFrustumCorners[0], vFrustumCorners[1], 0, true);
		ZDEBUG_DRAWLINE(vFrustumCorners[1], vFrustumCorners[2], 0, true);
		ZDEBUG_DRAWLINE(vFrustumCorners[2], vFrustumCorners[3], 0, true);
		ZDEBUG_DRAWLINE(vFrustumCorners[3], vFrustumCorners[0], 0, true);


		RenderShadowMap(g_SM, vOrigin, vDirection, vFrustumCorners);

	}









	int nNumLights = g_WorldState.nNumLights;
	{
		MICROPROFILE_SCOPEI("MAIN", "UpdateTexture", 0xff44dd44);
		CheckGLError();
		glBindTexture(GL_TEXTURE_2D, g_LightTexture);
	    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, nNumLights*2, 1, GL_RGBA, GL_FLOAT, &g_LightBuffer);
	    CheckGLError();

	    glBindTexture(GL_TEXTURE_2D, g_LightTileTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_LightTileWidth, g_LightTileHeight, GL_RG, GL_SHORT, &g_LightTileInfo);
	    CheckGLError();

	    glBindTexture(GL_TEXTURE_2D, g_LightIndexTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, LIGHT_INDEX_SIZE, MAX_LIGHT_INDEX/LIGHT_INDEX_SIZE, GL_RG, GL_SHORT, &g_LightIndex);
	    CheckGLError();

	    glBindTexture(GL_TEXTURE_2D, 0);
	    CheckGLError();

	}




	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_LightTexture);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, g_LightTileTexture);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, g_LightIndexTexture);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, g_SM.TextureId);
	glActiveTexture(GL_TEXTURE0);
	static int g_Mode = 0;
	uplotfnxt("mode is %d", g_Mode);
	if(g_KeyboardState.keys[SDL_SCANCODE_T] & BUTTON_RELEASED)
	{
		g_Mode = (g_Mode+1)%3;
	}



	ShaderUse(VS_LIGHTING, PS_LIGHTING);
	SHADER_SET("texLight", 1);
	SHADER_SET("texLightTile", 2);
	SHADER_SET("texLightIndex", 3);
	SHADER_SET("texDepth", 4);
	SHADER_SET("NumLights", nNumLights);
	//g_WorldState.nNumLights 
	SHADER_SET("lightDelta", 1.f / (2*MAX_NUM_LIGHTS));
	SHADER_SET("MaxTileLights", nMaxTileLights);
	SHADER_SET("ScreenSize", v2init(g_Width, g_Height));
	v3 vTileSize = v3init(g_LightTileWidth, g_LightTileHeight, TILE_SIZE);
	SHADER_SET("TileSize", vTileSize);
	SHADER_SET("MaxLightTileIndex", nLightIndex);
	SHADER_SET("vWorldEye", Camera.vPosition);
	m mprjview = mmult(Camera.mprj, Camera.mview);
	SHADER_SET("ProjectionMatrix", &mprjview);
	SHADER_SET("ShadowMatrix", &g_SM.mprjviewtex);
	SHADER_SET("mode", g_Mode);
	#define MAX_FAIL 32
	static SWorldObject* pFailObjects[MAX_FAIL];
	static int nNumFail = 0;
	if(nCanVerify)
	{
		MICROPROFILE_SCOPEI("MAIN", "Stencil Reference Test", 0xff44dddd);
		glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(1);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		int nLocSize = ShaderGetLocation("Size");
		int nLocModelView = ShaderGetLocation("ModelViewMatrix");
		{
			MICROPROFILE_SCOPEI("lala", "Fill Buffer", 0xff00ffff);
			for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
			{
				SWorldObject* pObject = g_WorldState.WorldObjects + i;
				if(pObject->nFlags & SObject::OCCLUDER_BOX)
				{
					v3 vSizeScaled = pObject->vSize;
					vSizeScaled *= BSP_BOX_SCALE;
					ShaderSetUniform(nLocSize, vSizeScaled);
					ShaderSetUniform(nLocModelView, &pObject->mObjectToWorld);
					MeshDraw(GetBaseMesh(MESH_BOX_FLAT));
				}
			}
		}
		glDepthMask(0);


		enum{
			MAX_QUERIES=10<<10,
			QUERY_FRAME_DELAY=1,
			PIXEL_THRESHOLD=5,//note:to get this to zero, the test should not generate overlapping meshes.
		};
		static int nQueryFrame = 0;
		struct QueryFrameState
		{
			int nReady;
			int nNumObjects;
			GLuint Queries[MAX_QUERIES];
			bool bCulled[MAX_QUERIES];
			SCameraState CameraState;
		};
		static QueryFrameState QS[QUERY_FRAME_DELAY] = {0};
		// static GLuint Queries[MAX_QUERIES] = {0};
		ZASSERT(10<<10 > g_WorldState.nNumWorldObjects);

		{{
			{
				QueryFrameState& Cur = QS[nQueryFrame];
				// uprintf("issue queries %d\n", nQueryFrame);
				if(!Cur.Queries[0])
				{
					glGenQueries(MAX_QUERIES, &Cur.Queries[0]);
				}
				{
					MICROPROFILE_SCOPEI("lala", "IssueQueries", 0xff00ffff);
					for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
					{
						SWorldObject* pObject = g_WorldState.WorldObjects + i;
						if(pObject->nFlags & SObject::OCCLUSION_TEST)
						{
							ShaderSetUniform(nLocSize, pObject->vSize);
							ShaderSetUniform(nLocModelView, &pObject->mObjectToWorld);
							glBeginQuery(GL_SAMPLES_PASSED, Cur.Queries[i]);
							MeshDraw(GetBaseMesh(MESH_BOX_FLAT));
							glEndQuery(GL_SAMPLES_PASSED);
						}
					}
				}
				Cur.nNumObjects = g_WorldState.nNumWorldObjects;
				memcpy(&Cur.bCulled[0], &bCulled[0], sizeof(Cur.bCulled[0]) * g_WorldState.nNumWorldObjects);
				Cur.CameraState = Camera;
				// Cur.CameraState = g_LockedCamera;
				

			}
			nQueryFrame = (nQueryFrame+1) % QUERY_FRAME_DELAY;
			nNumFail = 0;
			uint32 nAgree = 0, nFailCull = 0, nFalsePositives = 0;
	
			QueryFrameState& Prev = QS[nQueryFrame];
			if(Prev.nNumObjects && 0 == nNoRef)
			{
				// uprintf("fetch queries %d\n", nQueryFrame);
				{
					MICROPROFILE_SCOPEI("lala", "GetResult", 0xff00ffff);
					for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
					{
						SWorldObject* pObject = g_WorldState.WorldObjects + i;
						if(pObject->nFlags & SObject::OCCLUSION_TEST)
						{
							int result, result0;
							glGetQueryObjectiv(Prev.Queries[i], GL_QUERY_RESULT, &result0);
							glGetQueryObjectiv(Prev.Queries[i], GL_QUERY_RESULT, &result);
							ZASSERT(result == result0);
							bCulledRef[i] = PIXEL_THRESHOLD > result;
							nCulledRef[i] = result;
						}
					}
				}
				for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
				{
					SWorldObject* pObject = g_WorldState.WorldObjects + i;
					if(pObject->nFlags & SObject::OCCLUSION_TEST)
					{
						if(Prev.bCulled[i] && !bCulledRef[i])
						{
							++nFailCull;
							++g_nTestFail;
							g_nFailedObject = i;
							uprintf("FAIL CULL %d .. count %d\n", i, nCulledRef[i]);
							if(nNumFail < MAX_FAIL)
								pFailObjects[nNumFail++] = pObject;

						}
						else if(bCulledRef[i] && !Prev.bCulled[i])
						{
							++nFalsePositives;
							g_nTestFalsePositives++;
							//uprintf("FALSE CULL %d\n", i);
						}
						else
						{
							++nAgree;
						}
					}
				}
			}
			// if(g_nDebugDumpIndex>=0)
			// {
			// 	bool b1 = Prev.bCulled[g_nDebugDumpIndex];
			// 	bool b2 = bCulledRef[g_nDebugDumpIndex];
			// 	fprintf(g_TestFailOut, "DEBUG %s : %d %d  ", b1 == b2 ? "---" : "***", b1, b2);
			// 	fprintf(g_TestFailOut, "g_WorldState.Camera.vPosition = v3init(%f,%f,%f);", Prev.vLockedCamPos.x, Prev.vLockedCamPos.y, Prev.vLockedCamPos.z);
			// 	fprintf(g_TestFailOut, "g_WorldState.Camera.vDir = v3init(%f,%f,%f);", Prev.vLockedCamDir.x, Prev.vLockedCamDir.y, Prev.vLockedCamDir.z);
			// 	fprintf(g_TestFailOut, "g_WorldState.Camera.vRight = v3init(%f,%f,%f);\n", Prev.vLockedCamRight.x, Prev.vLockedCamRight.y, Prev.vLockedCamRight.z);
			// 	fflush(g_TestFailOut);
			// }
			uplotfnxt("FAIL %d  FALSE %d AGREE %d", nFailCull, nFalsePositives, nAgree);
			if(nNumFail)
			{
				uprintf("FAIL %d  FALSE %d AGREE %d\n", nFailCull, nFalsePositives, nAgree);
				if(g_nRunTest)
				{
					fprintf(g_TestFailOut, "g_WorldState.Camera.vPosition = v3init(%f,%f,%f);\n", Prev.CameraState.vPosition.x, Prev.CameraState.vPosition.y, Prev.CameraState.vPosition.z);
					fprintf(g_TestFailOut, "g_WorldState.Camera.vDir = v3init(%f,%f,%f);\n", Prev.CameraState.vDir.x, Prev.CameraState.vDir.y, Prev.CameraState.vDir.z);
					fprintf(g_TestFailOut, "g_WorldState.Camera.vRight = v3init(%f,%f,%f)\n;", Prev.CameraState.vRight.x, Prev.CameraState.vRight.y, Prev.CameraState.vRight.z);
					uprintf("locking camera\n");
					g_nRunTest = 0;
					g_LockedCamera = Camera;
				}
				else
				{
					uprintf("locking camera\n");
					// g_nUseDebugCameraPos = 1;
					g_LockedCamera = Camera;

				}
				DoBuildBsp(ViewDesc, g_nFailedObject);
// uint32_t BspBuildDebugSearch(SOccluderBsp* pBsp, SOccluderDesc** pPlaneOccluders, uint32 nNumPlaneOccluders,
// 			  SOccluderDesc** pBoxOccluders, uint32 nNumBoxOccluders,
// 			  const SOccluderBspViewDesc& Desc,
// 			  SOccluderDesc* pObject,
// 			  bool* pDebugMask,
// 			  uint32_t nDebugMaskSize)





				WriteCamera(Camera);



			}
		}}
	}

	static float fub = 0;
	fub += 0.1f;
	float fMult = 2.2f + cos(fub) * 5.3f;
	if(nNumFail)
		uplotfnxt("DRAWING FAIL %d", nNumFail);
	// pFailObjects[0] = g_WorldState.WorldObjects + 194;
	// nNumFail = 1;
	//uprintf("bculled %d\n", bCulled[194]);
	for(int i = 0; i < nNumFail; ++i)
	{
		SWorldObject* pObject = pFailObjects[i];
		ZDEBUG_DRAWBOX(pObject->mObjectToWorld, pObject->mObjectToWorld.trans.tov3(), fMult * pObject->vSize, 0xffffff00, 0);

	}


	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	static int usezpass = 1;
	{
		MICROPROFILE_SCOPEI("MAIN", "Main render", 0xff44dddd);
		MICROPROFILE_SCOPEGPUI("GPU", "OBJ PASS", 0xffff77);
		if(usezpass)
		{
			glDepthFunc(GL_LEQUAL);
			{
				//ShaderUse(VS_LIGHTING, PS_LIGHTING2);
				SHADER_SET("ProjectionMatrix", &mprjview);
				MICROPROFILE_SCOPEGPUI("GPU", "zpass", 0x9900ee);
				glColorMask(0,0,0,0);
				WorldDrawObjects(&bCulled[0]);
			}
			ShaderUse(VS_LIGHTING, PS_LIGHTING);
			glDepthFunc(GL_EQUAL);
			glColorMask(1,1,1,1);
			WorldDrawObjects(&bCulled[0]);
			glDepthFunc(GL_LEQUAL);
		}
		else
		{
			glColorMask(1,1,1,1);
			WorldDrawObjects(&bCulled[0]);
		}
	}	
	if(g_nDebugCullDraw & 1)
	{
		WorldDrawCulledObjects(&bCulled[0]);
	}
	ShaderDisable();
	glDisable(GL_CULL_FACE);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	CheckGLError();


	

}
void UpdateEditorState()
{
	//for(int i = 0; i < 10; ++i)
	{
		int but = 0;
		int released = 0;
		if(g_KeyboardState.keys[SDL_SCANCODE_0] & BUTTON_RELEASED)
		{
			but = 0;
			released = 1;

		}
		else if(g_KeyboardState.keys[SDL_SCANCODE_1] & BUTTON_RELEASED)
		{
			but = 1;
			released = 1;
		}
		else if(g_KeyboardState.keys[SDL_SCANCODE_2] & BUTTON_RELEASED)
		{
			but = 2;
			released = 1;
		}

		if(released)
		{
			g_EditorState.Mode = but;
			uprintf("switched mode to %d\n", g_EditorState.Mode);
			if(g_EditorState.Dragging != DRAGSTATE_NONE && g_EditorState.DragTarget == DRAGTARGET_TOOL)
			{
				if(g_EditorState.Manipulators[g_EditorState.Mode])
					g_EditorState.Manipulators[g_EditorState.Mode]->DragEnd(g_EditorState.vDragStart, g_EditorState.vDragEnd);
			}
			g_EditorState.Dragging = DRAGSTATE_NONE;

		}
	}
	uplotfnxt("mode is %d", g_EditorState.Mode);
	if(g_KeyboardState.keys[SDL_SCANCODE_ESCAPE] & BUTTON_RELEASED)
	{
		g_EditorState.pSelected = 0;
	}
	bool bShift = 0 != ((g_KeyboardState.keys[SDL_SCANCODE_RSHIFT]|g_KeyboardState.keys[SDL_SCANCODE_LSHIFT]) & BUTTON_DOWN);

	if(bShift && (g_KeyboardState.keys[SDL_SCANCODE_SPACE] & BUTTON_RELEASED))
	{
		if(g_EditorState.pSelected)
		{
			g_EditorState.bLockSelection = !g_EditorState.bLockSelection;
		}
		else
		{
			g_EditorState.bLockSelection = false;
		}
	}

	// {
	// 	v2 vPos = v2init(g_MouseState.position[0], g_MouseState.position[1]);
	// 	v3 vDir = DirectionFromScreen(vPos, *g_WorldState.pCamera);
	// 	v3 t1 = g_WorldState.Camera.vPosition + vDir * 5.f;//vMouseWorld + v3normalize(vMouseWorld -g_WorldState.Camera.vPosition) *5.f;
	// //	ZDEBUG_DRAWBOX(mid(), t1, v3init(0.2, 0.2, 0.2), -1);

	// }

	if(g_MouseState.button[1]&BUTTON_PUSHED)
	{
		v2 vPos = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		if(!g_EditorState.pSelected || !g_EditorState.Manipulators[g_EditorState.Mode] || !g_EditorState.Manipulators[g_EditorState.Mode]->DragBegin(vPos, vPos, g_EditorState.pSelected))

			// ((g_KeyboardState.keys[SDL_SCANCODE_LCTRL]|g_KeyboardState.keys[SDL_SCANCODE_RCTRL]) & BUTTON_DOWN)|| g_EditorState.pSelected == 0)
		{
			g_EditorState.DragTarget = DRAGTARGET_CAMERA;
			uprintf("Drag begin CAMERA\n");
		}
		else
		{
			g_EditorState.DragTarget = DRAGTARGET_TOOL;
			uprintf("Drag begin TOOL %d\n", g_EditorState.Mode);
		}

		{

			g_EditorState.Dragging = DRAGSTATE_BEGIN;
			g_EditorState.vDragEnd = g_EditorState.vDragStart = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		}
	}
	else if(g_MouseState.button[1]&BUTTON_RELEASED)
	{
		g_EditorState.Dragging = DRAGSTATE_END;
		v2 vPrev = g_EditorState.vDragEnd;
		g_EditorState.vDragEnd = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		g_EditorState.vDragDelta = g_EditorState.vDragEnd - vPrev;
		uprintf("Drag end\n");
	}
	else if(g_MouseState.button[1]&BUTTON_DOWN)
	{
		g_EditorState.Dragging = DRAGSTATE_UPDATE;
		v2 vPrev = g_EditorState.vDragEnd;
		g_EditorState.vDragEnd = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		g_EditorState.vDragDelta = g_EditorState.vDragEnd - vPrev;
		uplotfnxt("DRAGGING %f %f ", g_EditorState.vDragEnd.x, g_EditorState.vDragEnd.y);
	}
	else
	{
		g_EditorState.vDragEnd = g_EditorState.vDragStart = g_EditorState.vDragDelta = v2init(0,0);
		g_EditorState.Dragging = DRAGSTATE_NONE;
	}

	switch(g_EditorState.DragTarget)
	{
		case DRAGTARGET_CAMERA:
		{
			g_WorldState.vCameraRotate = g_EditorState.vDragDelta;
		}
		break;
		case DRAGTARGET_TOOL:
		{
			ZASSERT(g_EditorState.pSelected);
			switch(g_EditorState.Mode)
			{
				default:
				{
					switch(g_EditorState.Dragging)
					{
						case DRAGSTATE_BEGIN:
						{
							g_EditorState.mSelected = g_EditorState.pSelected->mObjectToWorld;
						}
						break;
						case DRAGSTATE_UPDATE:
						{
							uplotfnxt("DRAG UP");
							if(g_EditorState.Manipulators[g_EditorState.Mode])
							{
								g_EditorState.Manipulators[g_EditorState.Mode]->DragUpdate(g_EditorState.vDragStart, g_EditorState.vDragEnd);
							}
						}

					}
				}
			}
		}
		break;
	}

	switch(g_EditorState.Mode)
	{
		case 0: //translate
		{

		}
	}

	if(g_EditorState.Dragging == DRAGSTATE_END)
	{
		g_EditorState.DragTarget = DRAGTARGET_NONE;
	}
}
void UpdatePicking()
{
	v3 vMouse = v3init( g_MouseState.position[0], g_MouseState.position[1], 0);
	v3 vMouseClip = mtransform(g_WorldState.pActiveCamera->mviewportinv, vMouse);
	vMouseClip.z = -0.9f;
	v4 vMouseView_ = mtransform(g_WorldState.pActiveCamera->mprjinv, v4init(vMouseClip, 1.f));
	v3 vMouseView = vMouseView_.tov3() / vMouseView_.w;
	v3 vMouseWorld = mtransform(g_WorldState.pActiveCamera->mviewinv, vMouseView);
	// uplotfnxt("vMouse %f %f %f", vMouse.x, vMouse.y, vMouse.z);
	// uplotfnxt("vMouseClip %f %f %f", vMouseClip.x, vMouseClip.y, vMouseClip.z);
	// uplotfnxt("vMouseView %f %f %f", vMouseView.x, vMouseView.y, vMouseView.z);
	// uplotfnxt("vMouseWorld %f %f %f", vMouseWorld.x, vMouseWorld.y, vMouseWorld.z);
	// v3 test = g_WorldState.Camera.vPosition + g_WorldState.Camera.vDir * 3.f;
	// ZDEBUG_DRAWBOX(mid(), test, v3init(0.3, 0.3, 0.3), -1);
	v3 t0 = vMouseWorld;
	v3 t1 = vMouseWorld + v3normalize(vMouseWorld -g_WorldState.pActiveCamera->vPosition) *5.f;
	//	ZDEBUG_DRAWBOX(mid(), t1, v3init(0.1, 0.1, 0.1), -1);
	// ZDEBUG_DRAWLINE(test, t0, -1, 0);

	if(g_MouseState.button[1] & BUTTON_RELEASED && !g_EditorState.bLockSelection)
	{
		v3 vPos = g_WorldState.pActiveCamera->vPosition;
		v3 vDir = vMouseWorld - vPos;
		vDir = v3normalize(vDir);
		float fNearest = 1e30;
		SObject* pNearest = 0;

		auto Intersect = [&] (SObject* pObject) 
		{ 
			float fColi = rayboxintersect(vPos, vDir, pObject->mObjectToWorld, pObject->vSize);
			if(fColi > g_WorldState.pActiveCamera->fNear && fColi < fNearest)
			{
				pNearest = pObject;
				fNearest = fColi;
			}
		};
		for(SObject& Obj : g_WorldState.WorldObjects)
			Intersect(&Obj);

		for(SObject& Obj : g_WorldState.Occluders)
			Intersect(&Obj);
		g_EditorState.pSelected = pNearest;
	}


}

void SetupCameraState(SCameraState& Camera, v3 vPosition, v3 vDir, v3 vRight)
{
	memset(&Camera, 0xef, sizeof(Camera));
	Camera.vPosition = vPosition;
	Camera.vRight = vRight;
	Camera.vDir = vDir;
	Camera.fFovY = 45.f;
	Camera.fNear = 0.1f;
	Camera.mview = mcreate(Camera.vDir, Camera.vRight, Camera.vPosition);

	float fAspect = (float)g_Width / (float)g_Height;
	
	if(g_nUseOrtho)
	{
		Camera.mprj = mortho(g_fOrthoScale, g_fOrthoScale / fAspect, 1000);
	}
	else
	{
		Camera.mprj = mperspective(Camera.fFovY, ((float)g_Height / (float)g_Width), Camera.fNear, 2000.f);
	}
	
	Camera.mviewport = mviewport(0,0,g_Width, g_Height);
	Camera.mviewportinv = minverse(Camera.mviewport);
	Camera.mprjinv = minverse(Camera.mprj);
	Camera.mviewinv = maffineinverse(Camera.mview);	
}

void UpdateCamera()
{
	if(g_KeyboardState.keys[SDL_SCANCODE_BACKSPACE] & BUTTON_RELEASED)
	{
		if(!g_nRunTest)
		{
			g_CameraSource = ESOURCE_TEST;
			g_CameraSourceBsp = ESOURCE_TEST;
			g_nTestAdvance = 1;
			StartTest();
		}
		else
		{
			StopTest();
		}
	}

	if(g_KeyboardState.keys[SDL_SCANCODE_F2] & BUTTON_RELEASED)
		g_nUseOrtho = !g_nUseOrtho;


	//free cam
	if(g_CameraSource == ESOURCE_FREECAM)
	{{
		//free cam
		v3 vDir = v3init(0,0,0);
		if(g_KeyboardState.keys[SDL_SCANCODE_A] & BUTTON_DOWN)
		{
			vDir.x = -1.f;
		}
		else if(g_KeyboardState.keys[SDL_SCANCODE_D] & BUTTON_DOWN)
		{
			vDir.x = 1.f;
		}

		if(g_KeyboardState.keys[SDL_SCANCODE_W] & BUTTON_DOWN)
		{
			vDir.z = 1.f;
		}
		else if(g_KeyboardState.keys[SDL_SCANCODE_S] & BUTTON_DOWN)
		{
			vDir.z = -1.f;
		}
		float fSpeed = 0.1f;
		if((g_KeyboardState.keys[SDL_SCANCODE_RSHIFT]|g_KeyboardState.keys[SDL_SCANCODE_LSHIFT]) & BUTTON_DOWN)
			fSpeed *= 0.2f;
		if((g_KeyboardState.keys[SDL_SCANCODE_RCTRL]|g_KeyboardState.keys[SDL_SCANCODE_LCTRL]) & BUTTON_DOWN)
			fSpeed *= 12.0f;

		g_FreeCamera.vPosition += g_FreeCamera.vDir * vDir.z * fSpeed;
		g_FreeCamera.vPosition += g_FreeCamera.vRight * vDir.x * fSpeed;


		// static int mousex, mousey;
		// if(g_MouseState.button[1] & BUTTON_PUSHED)
		// {
		// 	mousex = g_MouseState.position[0];
		// 	mousey = g_MouseState.position[1];
		// }

		//if(g_MouseState.button[1] & BUTTON_DOWN)
		{
			int dx = g_WorldState.vCameraRotate.x;//g_MouseState.position[0] - mousex;
			int dy = g_WorldState.vCameraRotate.y;//g_MouseState.position[1] - mousey;
			// mousex = g_MouseState.position[0];
			// mousey = g_MouseState.position[1];

			float fRotX = dy * 0.25f;
			float fRotY = dx * -0.25f;
			m mrotx = mrotatex(fRotX*TORAD);
			m mroty = mrotatey(fRotY*TORAD);
			g_FreeCamera.vDir = mtransform(mroty, g_FreeCamera.vDir);
			g_FreeCamera.vRight = mtransform(mroty, g_FreeCamera.vRight);

			m mview = mcreate(g_FreeCamera.vDir, g_FreeCamera.vRight, v3init(0,0,0));
			m mviewinv = minverserotation(mview);
			v3 vNewDir = mtransform(mrotx, v3init(0,0,-1));
			g_FreeCamera.vDir = mtransform(mviewinv, vNewDir);

		}
		SetupCameraState(g_FreeCamera, g_FreeCamera.vPosition, g_FreeCamera.vDir, g_FreeCamera.vRight);
		ZASSERTNORMALIZED3(g_FreeCamera.vDir);
		ZASSERTNORMALIZED3(g_FreeCamera.vRight);

	}}
	//test cam
	{{
		v3 vPosition, vDir, vRight;
		if(g_nRunTest && g_nTestAdvance)
		{	
			RunTest(vPosition, vDir, vRight);
		}
		else
		{
			EvalTest(g_nTestFrame, vPosition, vDir, vRight);
		}
		SetupCameraState(g_TestCamera, vPosition, vDir, vRight);



	}}
// her til.. opdater resten.


	// {

	// 	if(g_nRunTest)
	// 	{	
	// 		RunTest(g_WorldState.Camera.vPosition, g_WorldState.Camera.vDir, g_WorldState.Camera.vRight);
	// 	}
	// 	else
	// 	{
	// 		g_WorldState.Camera.vPosition += g_WorldState.Camera.vDir * vDir.z * fSpeed;
	// 		g_WorldState.Camera.vPosition += g_WorldState.Camera.vRight * vDir.x * fSpeed;


	// 		// static int mousex, mousey;
	// 		// if(g_MouseState.button[1] & BUTTON_PUSHED)
	// 		// {
	// 		// 	mousex = g_MouseState.position[0];
	// 		// 	mousey = g_MouseState.position[1];
	// 		// }

	// 		//if(g_MouseState.button[1] & BUTTON_DOWN)
	// 		{
	// 			int dx = g_WorldState.vCameraRotate.x;//g_MouseState.position[0] - mousex;
	// 			int dy = g_WorldState.vCameraRotate.y;//g_MouseState.position[1] - mousey;
	// 			// mousex = g_MouseState.position[0];
	// 			// mousey = g_MouseState.position[1];

	// 			float fRotX = dy * 0.25f;
	// 			float fRotY = dx * -0.25f;
	// 			m mrotx = mrotatex(fRotX*TORAD);
	// 			m mroty = mrotatey(fRotY*TORAD);
	// 			g_WorldState.Camera.vDir = mtransform(mroty, g_WorldState.Camera.vDir);
	// 			g_WorldState.Camera.vRight = mtransform(mroty, g_WorldState.Camera.vRight);

	// 			m mview = mcreate(g_WorldState.Camera.vDir, g_WorldState.Camera.vRight, v3init(0,0,0));
	// 			m mviewinv = minverserotation(mview);
	// 			v3 vNewDir = mtransform(mrotx, v3init(0,0,-1));
	// 			g_WorldState.Camera.vDir = mtransform(mviewinv, vNewDir);

	// 		}
	// 		ZASSERTNORMALIZED3(g_WorldState.Camera.vDir);
	// 		ZASSERTNORMALIZED3(g_WorldState.Camera.vRight);
	// 		SetupCameraState(g_FreeCamera, 
	// 	}
	// }



	// g_WorldState.Camera.mview = mcreate(g_WorldState.Camera.vDir, g_WorldState.Camera.vRight, g_WorldState.Camera.vPosition);
	// if(g_KeyboardState.keys[SDL_SCANCODE_F2] & BUTTON_RELEASED)
	// 	g_nUseOrtho = !g_nUseOrtho;

	// float fAspect = (float)g_Width / (float)g_Height;
	
	// if(g_nUseOrtho)
	// {
	// 	g_WorldState.Camera.mprj = mortho(g_fOrthoScale, g_fOrthoScale / fAspect, 1000);
	// }
	// else
	// {
	// 	g_WorldState.Camera.mprj = mperspective(g_WorldState.Camera.fFovY, ((float)g_Height / (float)g_Width), g_WorldState.Camera.fNear, 2000.f);
	// }
	
	// g_WorldState.Camera.mviewport = mviewport(0,0,g_Width, g_Height);
	// g_WorldState.Camera.mviewportinv = minverse(g_WorldState.Camera.mviewport);
	// g_WorldState.Camera.mprjinv = minverse(g_WorldState.Camera.mprj);
	// g_WorldState.Camera.mviewinv = maffineinverse(g_WorldState.Camera.mview);
	SCameraState& Cam = GetCamera(g_CameraSource);
	g_WorldState.pActiveCamera = &Cam;
	uplotfnxt("Dir[%5.2f,%5.2f,%5.2f] Pos[%3.2f,%3.2f,%3.2f]",
		Cam.vDir.x,
		Cam.vDir.y,
		Cam.vDir.z,
		Cam.vPosition.x,
		Cam.vPosition.y,
		Cam.vPosition.z);
}
void foo()
{
	//uprintf("lala\n");
	ZBREAK();
}


void WriteCamera(const SCameraState& Camera)
{
	uint32_t* pData = (uint32_t*)&Camera;
	ZASSERT((sizeof(Camera)%4) == 0);
	uint32 nSize = sizeof(Camera)/4;
	uprintf("\tuint32_t nCameraData[] = {\n\t\t");
	for(uint32_t i = 0; i < nSize; ++i)
	{
		uprintf("0x%08x,", pData[i]);
		if(0 == ((i+1)%10))
		{
			uprintf("\n\t\t");
		}
	}
	uprintf("\n\t};\n");

}

void InitLocked()
{
	uint32_t nCameraData[] = {
		0xefefefef,0xefefefef,0xefefefef,0xefefefef,
		0xefefefef,0xefefefef,0xefefefef,0xefefefef,
		0xefefefef,0xefefefef,0xefefefef,0xefefefef,
		0xefefefef,0xefefefef,0xefefefef,0xefefefef,
		0xefefefef,0xefefefef,0xefefefef,0xefefefef,
		0xefefefef,0xefefefef,0xefefefef,0xefefefef,
		0x3f6d96ed,0xbeb7861a,0x3dce96af,0xbddd4aae,
		0x00000000,0x3f7e803e,0xc3947989,0x414cfdae,
		0xc2f04a0d,0xbddd4aae,0x3eb67311,0xbf6d96ed,
		0x00000000,0x00000000,0x3f6efd2f,0x3eb7861a,
		0x00000000,0x3f7e803e,0x3d1ea45f,0xbdce96af,
		0x00000000,0x42aeb5ba,0x42c504f6,0xc3922727,
		0x3f800000,0xbddd4aae,0x00000000,0x3f7e803e,
		0x00000000,0x3eb67311,0x3f6efd2f,0x3d1ea45f,
		0x00000000,0xbf6d96ed,0x3eb7861a,0xbdce96af,
		0x00000000,0xc394797d,0x414cfd08,0xc2f04a11,
		0x3f800000,0x3fe7c3b6,0x00000000,0x00000000,
		0x00000000,0x00000000,0x401a8279,0x00000000,
		0x00000000,0x00000000,0x00000000,0xbf800347,
		0xbf800000,0x00000000,0x00000000,0xbe4ccf6c,
		0x00000000,0x3f0d6289,0x00000000,0x00000000,
		0x00000000,0x00000000,0x3ed413ce,0x00000000,
		0x00000000,0x80000000,0x80000000,0x00000000,
		0xc09ffdf3,0x00000000,0x00000000,0xbf800000,
		0x40a0020c,0x43c80000,0x00000000,0x00000000,
		0x00000000,0x00000000,0x43960000,0x00000000,
		0x00000000,0x00000000,0x00000000,0x3f800000,
		0x00000000,0x43c80000,0x43960000,0x00000000,
		0x3f800000,0x3b23d70a,0x00000000,0x00000000,
		0x00000000,0x00000000,0x3b5a740e,0x00000000,
		0x00000000,0x00000000,0x00000000,0x3f800000,
		0x00000000,0xbf800000,0xbf800000,0x00000000,
		0x3f800000,0x3dcccccd,0xefefefef,0x42340000,
		
	};

	ZASSERT(sizeof(nCameraData) == sizeof(g_LockedCamera));
	memcpy(&g_LockedCamera, &nCameraData[0], sizeof(g_LockedCamera));


}




void ProgramInit()
{
	m mroty = mid();//mrotatey(45.f * TORAD);
	v3 vPos = v3init(-300.013580,10.000000,-128.605072);
	v3 vDir = v3init(0.918683,-0.030621,0.393807);
	v3 vRight = v3init(-0.393991,0.000000,0.919114);
	SetupCameraState(g_FreeCamera, vPos, vDir, vRight);
	SetupCameraState(g_LockedCamera, vPos, vDir, vRight);
	SetupCameraState(g_TestCamera, vPos, vDir, vRight);
	InitLocked();
	g_WorldState.pActiveCamera = &g_FreeCamera;
	g_Bsp = BspCreate();
	WorldInit();
	EditorStateInit();


	g_SM = AllocateShadowMap();


}


void DisplayUI()
{
	if(g_KeyboardState.keys[SDL_SCANCODE_U] & BUTTON_RELEASED)
	{
		g_UIEnabled = !g_UIEnabled;
	}
	if(!g_UIEnabled)
		return;
	static bool Controls = true;
	static bool Test = true;
	ImGui::Begin("Controls", &Controls, ImVec2(200,100));
	Test ^= ImGui::Button("Test");
	if(ImGui::Button("Locked"))
	{
		g_CameraSource = ESOURCE_LOCKED;
		g_CameraSourceBsp = ESOURCE_LOCKED;
	}
	ImGui::Combo("Camera Source", &g_CameraSource, &g_CameraSourceStrings[0], sizeof(g_CameraSourceStrings) / sizeof(g_CameraSourceStrings[0]), 10);
	ImGui::Combo("Bsp Source", &g_CameraSourceBsp, &g_CameraSourceStrings[0], sizeof(g_CameraSourceStrings) / sizeof(g_CameraSourceStrings[0]));
	ImGui::End();

	if(Test)
	{
		ImGui::Begin("Test", &Test, ImVec2(500,100));
		ImGui::InputInt("FrameInput", &g_nTestFrame);
		ImGui::SliderInt("Frame", &g_nTestFrame, 0, TEST_TOTAL_STEPS-1);
		bool nTestStarted = g_nRunTest != 0;
		ImGui::Checkbox("Start", &nTestStarted);
		if(nTestStarted != (0 != g_nRunTest))
		{
			if(nTestStarted)
			{
				g_CameraSource = ESOURCE_TEST;
				g_CameraSourceBsp = ESOURCE_TEST;
				g_nTestAdvance = 1;
				StartTest();
			}
			else
			{
				StopTest();
			}
		}
		ImGui::Checkbox("Advance", &g_nTestAdvance);
		ImGui::SliderInt("NodeCap", &g_nBspNodeCap, 10, 2048);
		const int nCanVerify = g_CameraSourceBsp == g_CameraSource && 0 == g_nTestFail;
		ImGui::Text("Verify %d, FailCount %d, Failed %d", nCanVerify, g_nTestFail, g_nFailedObject);
		ImGui::Checkbox("DebugMode", &g_nDebugMode);
		if(ImGui::Button("Clear Locked"))
		{
			g_nTestFail = 0;
		}

		ImGui::End();
	}


		// static bool show_test_window = true;
		// static bool show_another_window = false;
		// static float f;
		// ImGui::Text("Hello, world!");
		// ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
		// show_another_window ^= ImGui::Button("Another Window");

  //       // Show another simple window
  //       if (show_another_window)
  //       {
  //           ImGui::Begin("Another Window", &show_another_window, ImVec2(200,100));
  //           ImGui::Text("Hello");
  //           static bool foo = true;
  //           ImGui::Separator();
  //           ImGui::BulletText("lala %f %d", 1344.344f, 10);
  //           ImGui::BulletText("he %f %d", 1344.344f, 10);
  //           ImGui::BulletText("foo %f %d", 1344.344f, 10);
  //           ImGui::BulletText("lala %f %d", 1344.344f, 10);
  //           ImGui::Separator();
  //           const char* names[]=
  //           {
  //           	"ged",
  //           	"faar",
  //           	"fisk",

  //           };
  //           static int cval = 1;
  //           ImGui::Combo("combo", &cval, names, 3);
  //   bool        Combo(const char* label, int* current_item, const char** items, int items_count, int popup_height_items = 7);


  //           ImGui::Checkbox("hest hest", &foo);
  //               // bool        Checkbox(const char* label, bool* v);

  //           ImGui::End();
  //       }

        

  //       ImGui::Text("Hello, world!");
  //       // static float f= 1;
  //       ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
}

int ProgramMain()
{
	static int once = 0;
	if(!once)
	{
		once = 1;
		RunTestOnly();
	}


	DisplayUI();




	//fesetenv(FE_DFL_DISABLE_SSE_DENORMS_ENV);
	if(g_KeyboardState.keys[SDL_SCANCODE_ESCAPE] & BUTTON_RELEASED)
		return 1;

	if(g_KeyboardState.keys[SDL_SCANCODE_SPACE] & BUTTON_RELEASED)
	{
		// g_LockCamera = (g_LockCamera+1)%4;
		// g_nUseDebugCameraPos = (g_nUseDebugCameraPos+1)%3;
	}
	MICROPROFILE_SCOPEGPUI("GPU", "Full Frame", 0x88dd44);
	UpdateEditorState();
	{
		UpdateCamera();
		UpdatePicking();
	}

	glLineWidth(1.f);
	glClearColor(0.3,0.4,0.6,0);
	glViewport(0, 0, g_Width, g_Height);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	CheckGLError();

	m mprj = g_WorldState.pActiveCamera->mprj;
	m mview = g_WorldState.pActiveCamera->mview;
	m mprjview = mmult(mprj, mview);


	// glMatrixMode(GL_PROJECTION);
	// glLoadMatrixf(&g_WorldState.Camera.mprj.x.x);
	// glMultMatrixf(&g_WorldState.Camera.mview.x.x);

	// glMatrixMode(GL_MODELVIEW);
	// glPushMatrix();
	// glLoadIdentity();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthFunc(GL_LEQUAL);
	glDisable(GL_DEPTH_TEST);
	CheckGLError();
	{
		MICROPROFILE_SCOPEGPUI("GPU", "Render World", 0x88dd44);
		WorldRender();
	}
	CheckGLError();
	{
		DebugRender(mprjview);
	}
	CheckGLError();

	//glPopMatrix();

	return 0;
}




v3 DirectionFromScreen(v2 vScreen, SCameraState& Camera)
{
	v3 vMouse = v3init(vScreen.x, vScreen.y, 0);
	v3 vMouseClip = mtransform(Camera.mviewportinv, vMouse);
	vMouseClip.z = -0.9f;
	v4 vMouseView_ = mtransform(Camera.mprjinv, v4init(vMouseClip, 1.f));
	v3 vMouseView = vMouseView_.tov3() / vMouseView_.w;
	v3 vMouseWorld = mtransform(Camera.mviewinv, vMouseView);
	return v3normalize(vMouseWorld - Camera.vPosition);

}

uint32_t CullObjects(uint32 nNumObjects, bool* bCulled)
{
	memset(bCulled, 0, nNumObjects);
	uint32_t nNumCulled = 0;
	for(uint32 i = 0; i < nNumObjects; ++i)
	{
		SWorldObject* pObject = &g_WorldState.WorldObjects[i];
		if(0 != (pObject->nFlags & SObject::OCCLUSION_TEST))
		{
			uint32 nDebug = g_nFailedObject == i;
			if(BspCullObject(g_Bsp, (SOccluderDesc*)&g_WorldState.WorldObjects[i].mObjectToWorld, 0, nDebug))
			{
				bCulled[i] = true;
				nNumCulled++;
			}
		}
	}
	return nNumCulled;
}



void CompareCullResult(uint32_t nNumObjects, bool* bCulled, bool* bCulledFast)
{
	int nFail = 0, nExtra = 0;
	for(int i = 0; i < nNumObjects; ++i)
	{
		if(bCulled[i] != bCulledFast[i])
		{
			ZDEBUG_DRAWBOX(g_WorldState.WorldObjects[i].mObjectToWorld, g_WorldState.WorldObjects[i].mObjectToWorld.trans.tov3(), g_WorldState.WorldObjects[i].vSize*1.02, 0xff5555ff, 0);
			ZBREAK();
			// ZDEBUG_DRAW
			if(bCulled[i])
			{
				// uprintf("fail index %d\n", i);
				nFail++;
			}
			else
			{
				nExtra++;
			}
		}
	}
	if(nFail || nExtra)
	{
		uprintf("verify diff is %d, extra %d, total %d\n", nFail, nExtra, nFail + nExtra);
	}
}


extern uint32_t g_nBaseObjects;
uint32_t g_nDebugGridCounter = 0;

uint32_t CullObjectsFast(uint32 nNumObjects, bool* bCulled, SWorldGrid& Grid)
{
	memset(bCulled, 0, nNumObjects);
	uint32_t nNumCulled;
	{
		MICROPROFILE_SCOPEI("CULLTEST", "BASECULL", 0xff0000);
		nNumCulled = CullObjects(g_nBaseObjects, bCulled);
	}
	SOccluderBspNodes NodeBsp;
	uint32 nD1=0, nD2 = 0 , nDiff = 0;
	for(uint32 i = 0; i < Grid.nNumSectors; ++i)
	{
		// if(i == Grid.nNumSectors / 2)
		{
			SWorldSector& S = Grid.Sector[i];
			// ZDEBUG_D
			v3 trans = v3load(&S.Desc.ObjectToWorld[12]);
			v3 vSize = v3load(S.Desc.Size);
			m matmat= mload(S.Desc.ObjectToWorld);

			SOccluderBspNodes* pNodes = BspBuildSubBsp(NodeBsp, g_Bsp, &S.Desc);
			S.nDebugStatus = pNodes ? 1 : 0;
			if(g_nDebugCullDraw & 2)
			{
				uint32_t color;
				if(S.nDebugStatus)
					color = 0xff00ff00;
				else
					color = 0xffff0000;
				if(i == g_nDebugGridCounter)
				{
					color = 0xff448800;
				}
				
				ZDEBUG_DRAWBOX(matmat, trans, vSize*1.02f, color, 0);
			}
			bool bRes = pNodes == 0;
			if(!bRes)
			{
				for(int j = 0; j < S.nCount; ++j)
				{
				// MICROPROFILE_SCOPEI("CULLTEST", "SUBCULL", 0x3300ff);
					uint32_t nIndex = Grid.nIndices[j + S.nStart];
					bool bTest1 = BspCullObject(g_Bsp, (SOccluderDesc*)&g_WorldState.WorldObjects[nIndex].mObjectToWorld, pNodes);
					// bool bTest2 = BspCullObject(g_Bsp, (SOccluderDesc*)&g_WorldState.WorldObjects[nIndex].mObjectToWorld);
					// if(bTest1 != bTest2)
					// {
					// 	if(bTest1)
					// 		nD1++;
					// 	else
					// 		nD2++;
					// 	nDiff++;
					// }
					if(bTest1)
					{
						if(!bCulled[nIndex])
						{
							bCulled[nIndex] = true;
							nNumCulled++;
						}
					}
				}
			}
			else
			{
				for(int j = 0; j < S.nCount; ++j)
				{
					uint32_t nIndex = Grid.nIndices[j + S.nStart];
					bCulled[nIndex] = true;
				}
			}
		}
		// else
		// {
		// 	// 			SWorldSector& S = Grid.Sector[i];

		// 	// for(int j = 0; j < S.nCount; ++j)
		// 	// {
		// 	// 	uint32_t nIndex = Grid.nIndices[j + S.nStart];
		// 	// 	bCulled[nIndex] = true;
		// 	// }
		// }

	}
//	uplotfnxt("DIFF %d %d %d\n", nDiff, nD1, nD2);
	return nNumCulled;
}




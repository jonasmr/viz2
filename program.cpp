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

extern uint32_t g_Width;
extern uint32_t g_Height;
uint32 g_nUseOrtho = 0;
float g_fOrthoScale = 10;
SOccluderBsp* g_Bsp = 0;
uint32 g_nUseDebugCameraPos = 2;
v3 vLockedCamPos = v3init(0,0,0);
v3 vLockedCamRight = v3init(0,-1,0);
v3 vLockedCamDir = v3init(1,0,0);



SWorldState g_WorldState;
SEditorState g_EditorState;

void DebugRender()
{
	if(g_EditorState.pSelected)
	{
		ZDEBUG_DRAWBOUNDS(g_EditorState.pSelected->mObjectToWorld, g_EditorState.pSelected->vSize, g_EditorState.bLockSelection ? (0xffffff00) : -1);

	}
	glBegin(GL_LINES);
	glColor3f(1,0,0);
	glVertex3f(0, 0, 0.f);
	glVertex3f(1.f, 0, 0.f);
	glColor3f(0,1,0);
	glVertex3f(0, 0, 0.f);
	glVertex3f(0.f, 1.f, 0.f);
	glColor3f(0,0,1);
	glVertex3f(0, 0, 0.f);
	glVertex3f(0.f, 0, 1.f);
	glEnd();


	DebugDrawFlush();
}

void EditorStateInit()
{
	memset(&g_EditorState, 0, sizeof(g_EditorState));
	g_EditorState.Manipulators[0] = new ManipulatorTranslate();
	g_EditorState.Manipulators[1] = new ManipulatorRotate();
}

void WorldInit()
{
	g_WorldState.Occluders[0].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[0].mObjectToWorld.trans = v4init(2.f,0.f,0.1f, 1.f);
	g_WorldState.Occluders[0].vSize = v3init(0.5f, 0.5f, 0.f);

	g_WorldState.Occluders[1].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[1].mObjectToWorld.trans = v4init(2.5f,0.f,-0.5f, 1.f);
	g_WorldState.Occluders[1].vSize = v3init(0.25f, 0.25f, 0);

	g_WorldState.Occluders[2].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.Occluders[2].mObjectToWorld.trans = v4init(2.6f,0.f,-0.5f, 1.f);
	g_WorldState.Occluders[2].vSize = v3init(0.25f, 0.25f, 0);
	g_WorldState.nNumOccluders = 3;


	g_WorldState.WorldObjects[0].mObjectToWorld = mrotatey(90*TORAD);
	g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f,0.f,-0.5f, 1.f);
	g_WorldState.WorldObjects[0].vSize = v3init(0.25f, 0.2f, 0.2); 
	g_WorldState.nNumWorldObjects = 1;
	if(0)
	{
		int idx = 0;
		#define GRID_SIZE 3
		for(uint32 i = 0; i < GRID_SIZE; i++)
		{
		for(uint32 j = 0; j < GRID_SIZE; j++)
		{
		for(uint32 k = 0; k < GRID_SIZE; k++)
		{
			v3 vPos = v3init(i,j,k) / (GRID_SIZE-1.f);
			vPos -= 0.5f;
			vPos *= 5.f;
			vPos += 0.03f;
			vPos.x += 3.4f;
			g_WorldState.WorldObjects[idx].mObjectToWorld = mid();
			g_WorldState.WorldObjects[idx].mObjectToWorld.trans = v4init(vPos,1.f);
			g_WorldState.WorldObjects[idx].vSize = v3init(0.25f, 0.2f, 0.2); 

			idx++;
		}}}
		g_WorldState.nNumWorldObjects = idx;
	}
	g_EditorState.pSelected = 0;
}

int g_nSimulate = 0;
void WorldRender()
{
	if(g_KeyboardState.keys[SDLK_F1] & BUTTON_RELEASED)
	{
		g_nSimulate = !g_nSimulate;
	}
	if(g_nSimulate)
	{
		g_WorldState.WorldObjects[0].mObjectToWorld = mmult(g_WorldState.WorldObjects[0].mObjectToWorld, mrotatey(0.5f*TORAD));
		g_WorldState.Occluders[0].mObjectToWorld = mmult(g_WorldState.Occluders[0].mObjectToWorld, mrotatez(0.3f*TORAD));
		static float xx = 0;
		xx += 0.005f;
		g_WorldState.WorldObjects[0].mObjectToWorld.trans = v4init(3.f, 0.f, sin(xx)*2, 1.f);
	}

	uint32 nNumOccluders = g_WorldState.nNumOccluders;
	static float foo = 0;
	foo += 0.01f;
	v3 vPos = vLockedCamPos;
	v3 vDir = vLockedCamDir;
	v3 vRight = vLockedCamRight;
	switch(g_nUseDebugCameraPos)
	{
		case 2:
			vPos = v3init(0,sin(foo), 0);
			vDir = v3normalize(v3init(3,0,0) - vPos);
			vRight = v3init(0,-1,0);
			{
				v3 vUp = v3normalize(v3cross(vRight, vDir));
				vRight = v3normalize(v3cross(vDir, vUp));
			}
			break;
		case 0:
			vPos = g_WorldState.Camera.vPosition;
			vDir = g_WorldState.Camera.vDir;
			vRight = g_WorldState.Camera.vRight;
			vLockedCamPos = vPos;
			vLockedCamDir = vDir;
			vLockedCamRight = vRight;
			break;
		case 1:
			//locked.
			break;
	}
		
	
	ZDEBUG_DRAWBOX(mid(), vPos, v3rep(0.02f), -1);
	SOccluderBspViewDesc ViewDesc;
	ViewDesc.vOrigin = vPos;
	ViewDesc.vDirection = vDir;
	ViewDesc.vRight = vRight;
	ViewDesc.fFovY = g_WorldState.Camera.fFovY;
	ViewDesc.fAspect = (float)g_Height / (float)g_Width;


	BspBuild(g_Bsp, &g_WorldState.Occluders[0], nNumOccluders, ViewDesc);

	uint32 nNumObjects = g_WorldState.nNumWorldObjects;
	bool* bCulled = (bool*)alloca(nNumObjects);
	for(uint32 i = 0; i < nNumObjects; ++i)
	{
		bCulled[i] = BspCullObject(g_Bsp, &g_WorldState.WorldObjects[i]);
	//	uplotfnxt("culled %d:%d", i, bCulled[0] ? 1: 0);
	}

	for(uint32 i = 0; i < g_WorldState.nNumWorldObjects; ++i)
	{
		glPushMatrix();
		glMultMatrixf(&g_WorldState.WorldObjects[i].mObjectToWorld.x.x);
		float x = g_WorldState.WorldObjects[i].vSize.x;
		float y = g_WorldState.WorldObjects[i].vSize.y;
		float z = g_WorldState.WorldObjects[i].vSize.z;
		glBegin(GL_LINE_STRIP);
		if(!bCulled[i])
			glColor3f(0,1,0);
		else
			glColor3f(1,0,0);

		glVertex3f(x, y, z);
		glVertex3f(x, -y, z);
		glVertex3f(-x, -y, z);
		glVertex3f(-x, y, z);
		glVertex3f(x, y, z);
		glEnd();

		glBegin(GL_LINE_STRIP);
		//glColor3f(0,1,0);
		glVertex3f(x, y, -z);
		glVertex3f(x, -y, -z);
		glVertex3f(-x, -y, -z);
		glVertex3f(-x, y, -z);
		glVertex3f(x, y, -z);
		glEnd();

		glBegin(GL_LINES);
		//glColor3f(0,1,0);
		glVertex3f(x, y, -z);
		glVertex3f(x, y, z);
		glVertex3f(x, -y, -z);
		glVertex3f(x, -y, z);
		glVertex3f(-x, -y, -z);
		glVertex3f(-x, -y, z);
		glVertex3f(-x, y, -z);
		glVertex3f(-x, y, z);
		glVertex3f(x, y, -z);
		glVertex3f(x, y, z);
		glEnd();


		glPopMatrix();
	}

}
void UpdateEditorState()
{
	for(int i = 0; i < 10; ++i)
	{
		if(g_KeyboardState.keys['0' + i] & BUTTON_RELEASED)
		{
			g_EditorState.Mode = i;
			if(g_EditorState.Dragging != DRAGSTATE_NONE && g_EditorState.DragTarget == DRAGTARGET_TOOL)
			{
				if(g_EditorState.Manipulators[g_EditorState.Mode])
					g_EditorState.Manipulators[g_EditorState.Mode]->DragEnd(g_EditorState.vDragStart, g_EditorState.vDragEnd);
			}
			g_EditorState.Dragging = DRAGSTATE_NONE;

		}
	}
	uplotfnxt("mode is %d", g_EditorState.Mode);
	if(g_KeyboardState.keys[SDLK_ESCAPE] & BUTTON_RELEASED)
	{
		g_EditorState.pSelected = 0;
	}
	if(g_KeyboardState.keys[SDLK_SPACE] & BUTTON_RELEASED)
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

	{
		v2 vPos = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		v3 vDir = DirectionFromScreen(vPos, g_WorldState.Camera);
		v3 t1 = g_WorldState.Camera.vPosition + vDir * 5.f;//vMouseWorld + v3normalize(vMouseWorld -g_WorldState.Camera.vPosition) *5.f;
	//	ZDEBUG_DRAWBOX(mid(), t1, v3init(0.2, 0.2, 0.2), -1);

	}

	if(g_MouseState.button[1]&BUTTON_PUSHED)
	{
		v2 vPos = v2init(g_MouseState.position[0], g_MouseState.position[1]);
		if(!g_EditorState.pSelected || !g_EditorState.Manipulators[g_EditorState.Mode] || !g_EditorState.Manipulators[g_EditorState.Mode]->DragBegin(vPos, vPos, g_EditorState.pSelected))

			// ((g_KeyboardState.keys[SDLK_LCTRL]|g_KeyboardState.keys[SDLK_RCTRL]) & BUTTON_DOWN)|| g_EditorState.pSelected == 0)
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
	v3 vMouseClip = mtransform(g_WorldState.Camera.mviewportinv, vMouse);
	vMouseClip.z = -0.9f;
	v4 vMouseView_ = mtransform(g_WorldState.Camera.mprjinv, v4init(vMouseClip, 1.f));
	v3 vMouseView = vMouseView_.tov3() / vMouseView_.w;
	v3 vMouseWorld = mtransform(g_WorldState.Camera.mviewinv, vMouseView);
	// uplotfnxt("vMouse %f %f %f", vMouse.x, vMouse.y, vMouse.z);
	// uplotfnxt("vMouseClip %f %f %f", vMouseClip.x, vMouseClip.y, vMouseClip.z);
	// uplotfnxt("vMouseView %f %f %f", vMouseView.x, vMouseView.y, vMouseView.z);
	// uplotfnxt("vMouseWorld %f %f %f", vMouseWorld.x, vMouseWorld.y, vMouseWorld.z);
	// v3 test = g_WorldState.Camera.vPosition + g_WorldState.Camera.vDir * 3.f;
	// ZDEBUG_DRAWBOX(mid(), test, v3init(0.3, 0.3, 0.3), -1);
	v3 t0 = vMouseWorld;
	v3 t1 = vMouseWorld + v3normalize(vMouseWorld -g_WorldState.Camera.vPosition) *5.f;
	//	ZDEBUG_DRAWBOX(mid(), t1, v3init(0.1, 0.1, 0.1), -1);
	// ZDEBUG_DRAWLINE(test, t0, -1, 0);

	if(g_MouseState.button[1] & BUTTON_RELEASED && !g_EditorState.bLockSelection)
	{
		v3 vPos = g_WorldState.Camera.vPosition;
		v3 vDir = vMouseWorld - vPos;
		vDir = v3normalize(vDir);
		float fNearest = 1e30;
		SObject* pNearest = 0;

		auto Intersect = [&] (SObject* pObject) 
		{ 
			float fColi = rayboxintersect(vPos, vDir, pObject->mObjectToWorld, pObject->vSize);
			if(fColi > g_WorldState.Camera.fNear && fColi < fNearest)
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

void UpdateCamera()
{
	v3 vDir = v3init(0,0,0);
	if(g_MouseState.button[SDL_BUTTON_WHEELUP] & BUTTON_RELEASED)
		g_fOrthoScale *= 0.96;
	if(g_MouseState.button[SDL_BUTTON_WHEELDOWN] & BUTTON_RELEASED)
		g_fOrthoScale /= 0.96;
	//uplotfnxt("ORTHO SCALE %f\n", g_fOrthoScale);



	if(g_KeyboardState.keys['a'] & BUTTON_DOWN)
	{
		vDir.x = -1.f;
	}
	else if(g_KeyboardState.keys['d'] & BUTTON_DOWN)
	{
		vDir.x = 1.f;
	}

	if(g_KeyboardState.keys['w'] & BUTTON_DOWN)
	{
		vDir.z = 1.f;
	}
	else if(g_KeyboardState.keys['s'] & BUTTON_DOWN)
	{
		vDir.z = -1.f;
	}
	float fSpeed = 0.1f;
	if((g_KeyboardState.keys[SDLK_RSHIFT]|g_KeyboardState.keys[SDLK_LSHIFT]) & BUTTON_DOWN)
		fSpeed *= 0.2f;
	g_WorldState.Camera.vPosition += g_WorldState.Camera.vDir * vDir.z * fSpeed;
	g_WorldState.Camera.vPosition += g_WorldState.Camera.vRight * vDir.x * fSpeed;


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
		g_WorldState.Camera.vDir = mtransform(mroty, g_WorldState.Camera.vDir);
		g_WorldState.Camera.vRight = mtransform(mroty, g_WorldState.Camera.vRight);

		m mview = mcreate(g_WorldState.Camera.vDir, g_WorldState.Camera.vRight, v3init(0,0,0));
		m mviewinv = minverserotation(mview);
		v3 vNewDir = mtransform(mrotx, v3init(0,0,-1));
		g_WorldState.Camera.vDir = mtransform(mviewinv, vNewDir);

	}
	ZASSERTNORMALIZED3(g_WorldState.Camera.vDir);
	ZASSERTNORMALIZED3(g_WorldState.Camera.vRight);



	g_WorldState.Camera.mview = mcreate(g_WorldState.Camera.vDir, g_WorldState.Camera.vRight, g_WorldState.Camera.vPosition);
	if(g_KeyboardState.keys[SDLK_F2] & BUTTON_RELEASED)
		g_nUseOrtho = !g_nUseOrtho;

	float fAspect = (float)g_Width / (float)g_Height;
	
	if(g_nUseOrtho)
	{
		g_WorldState.Camera.mprj = mortho(g_fOrthoScale, g_fOrthoScale / fAspect, 1000);
	}
	else
	{
		g_WorldState.Camera.mprj = mperspective(g_WorldState.Camera.fFovY, ((float)g_Height / (float)g_Width), 0.001f, 100.f);
	}
	
	g_WorldState.Camera.mviewport = mviewport(0,0,g_Width, g_Height);
	g_WorldState.Camera.mviewportinv = minverse(g_WorldState.Camera.mviewport);
	g_WorldState.Camera.mprjinv = minverse(g_WorldState.Camera.mprj);
	g_WorldState.Camera.mviewinv = maffineinverse(g_WorldState.Camera.mview);

	uplotfnxt("FPS %4.2f Dir[%5.2f,%5.2f,%5.2f] Pos[%3.2f,%3.2f,%3.2f]" , 1.f, 
		g_WorldState.Camera.vDir.x,
		g_WorldState.Camera.vDir.y,
		g_WorldState.Camera.vDir.z,
		g_WorldState.Camera.vPosition.x,
		g_WorldState.Camera.vPosition.y,
		g_WorldState.Camera.vPosition.z);


}
void foo()
{
	uprintf("lala\n");
	ZBREAK();
}


void ProgramInit()
{
	m mroty = mrotatey(45.f * TORAD);
	g_WorldState.Camera.vDir = mtransform(mroty, v3init(0,0,-1));
	g_WorldState.Camera.vRight = mtransform(mroty, v3init(1,0,0));
	g_WorldState.Camera.vPosition = g_WorldState.Camera.vDir * -5.f;
	g_WorldState.Camera.fFovY = 45.f;
	g_Bsp = BspCreate();
	WorldInit();
	EditorStateInit();

}

int ProgramMain()
{

	if(g_KeyboardState.keys[SDLK_ESCAPE] & BUTTON_RELEASED)
		return 1;

	if(g_KeyboardState.keys[SDLK_SPACE] & BUTTON_RELEASED)
	{
		g_nUseDebugCameraPos = (g_nUseDebugCameraPos+1)%3;
	}
	UpdateEditorState();
	{
		UpdateCamera();
		UpdatePicking();
	}

	glLineWidth(1.f);
	glClearColor(0.3,0.4,0.6,0);
	glViewport(0, 0, g_Width, g_Height);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(&g_WorldState.Camera.mprj.x.x);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf(&g_WorldState.Camera.mview.x.x);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CheckGLError();
	WorldRender();
	DebugRender();


	glPopMatrix();

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
// GLuint genTexture();
// GLuint genComputeProg(GLuint texHandle);

// void glTest()
// {
// 	GLuint tex = genTexture();
// 	GLuint computePtr = genComputeProg(tex);
// }

// GLuint genTexture() {
//         // We create a single float channel 512^2 texture
//         GLuint texHandle;
//         glGenTextures(1, &texHandle);

//         glActiveTexture(GL_TEXTURE0);
//         glBindTexture(GL_TEXTURE_2D, texHandle);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//         glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//         glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, 512, 512, 0, GL_RED, GL_FLOAT, NULL);

//         // Because we're also using this tex as an image (in order to write to it),
//         // we bind it to an image unit as well
//         //glBindImageTexture(0, texHandle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);
//         CheckGLError();     
//         return texHandle;
// }

// GLuint genComputeProg(GLuint texHandle) {
// 	if(GLEW_ARB_compute_shader)
// 	{
// 		uprintf("YAY\n");
// 	}
// 	else
// 	{
// 		uprintf("NAY\n");
// 	}
//     // Creating the compute shader, and the program object containing the shader
//     GLuint progHandle = glCreateProgram();
//     GLuint cs = glCreateShader(GL_COMPUTE_SHADER);

//     // In order to write to a texture, we have to introduce it as image2D.
//     // local_size_x/y/z layout variables define the work group size.
//     // gl_GlobalInvocationID is a uvec3 variable giving the global ID of the thread,
//     // gl_LocalInvocationID is the local index within the work group, and
//     // gl_WorkGroupID is the work group's index
//     const char *csSrc[] = {
//         "#version 430\n",
//         "uniform float roll;\
//          uniform image2D destTex;\
//          layout (local_size_x = 16, local_size_y = 16) in;\
//          void main() {\
//              ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);\
//              float localCoef = length(vec2(ivec2(gl_LocalInvocationID.xy)-8)/8.0);\
//              float globalCoef = sin(float(gl_WorkGroupID.x+gl_WorkGroupID.y)*0.1 + roll)*0.5;\
//              imageStore(destTex, storePos, vec4(1.0-globalCoef*localCoef, 0.0, 0.0, 0.0));\
//          }"
//     };

//     glShaderSource(cs, 2, csSrc, NULL);
//     glCompileShader(cs);
//     int rvalue;
//     glGetShaderiv(cs, GL_COMPILE_STATUS, &rvalue);
//     if (!rvalue) {
//         fprintf(stderr, "Error in compiling the compute shader\n");
//         GLchar log[10240];
//         GLsizei length;
//         glGetShaderInfoLog(cs, 10239, &length, log);
//         fprintf(stderr, "Compiler log:\n%s\n", log);
//         exit(40);
//     }
//     glAttachShader(progHandle, cs);

//     glLinkProgram(progHandle);
//     glGetProgramiv(progHandle, GL_LINK_STATUS, &rvalue);
//     if (!rvalue) {
//         fprintf(stderr, "Error in linking compute shader program\n");
//         GLchar log[10240];
//         GLsizei length;
//         glGetProgramInfoLog(progHandle, 10239, &length, log);
//         fprintf(stderr, "Linker log:\n%s\n", log);
//         exit(41);
//     }   
//     glUseProgram(progHandle);
    
//     glUniform1i(glGetUniformLocation(progHandle, "destTex"), 0);

//     CheckGLError();
//     return progHandle;
// }



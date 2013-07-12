#pragma once
// TODO: 
//
// blinking/ line over
// scrolling
// buffer overflow signal + handling
// drawing line skipping
// menu
// GPU timers
// unify ?
// cross frame timers
// pix visualization
// cross graph display
// mouseover for both

#include <stdint.h>
#include <string.h>
#if 1
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <unistd.h>
#include <libkern/OSAtomic.h>
#define MP_TICK() mach_absolute_time()
inline int64_t MicroProfileTickToNs(int64_t nTicks)
{
	static mach_timebase_info_data_t sTimebaseInfo;	
	if(sTimebaseInfo.denom == 0) 
	{
        (void) mach_timebase_info(&sTimebaseInfo);
    }
    return nTicks * sTimebaseInfo.numer / sTimebaseInfo.denom;
}


inline int64_t MicroProfileNsToTick(int64_t nNs)
{
	static mach_timebase_info_data_t sTimebaseInfo;	
	if(sTimebaseInfo.denom == 0) 
	{
        (void) mach_timebase_info(&sTimebaseInfo);
    }
    return nNs * sTimebaseInfo.denom / sTimebaseInfo.numer;
}

#define MP_CAS(ptr, value, old) OSAtomicCompareAndSwapPtr(old, value, (void**)ptr)
#define MP_CAS_U64(ptr, value, old) OSAtomicCompareAndSwap64(old, value, (int64_t*)ptr)
#define MP_BREAK() __builtin_trap()
#elif defined(_WIN32)
#define MP_BREAK() __debugbreak()
#endif

#define MP_ASSERT(a) do{if(!(a)){MP_BREAK();} }while(0)
#define MP_TICK_TO_MS(a) (MicroProfileTickToNs(a) / 1000000.f)
#define MP_MS_TO_TICK(a) (MicroProfileNsToTick(a*1000000.f))
#define MICROPROFILE_DECLARE(var) extern MicroProfileToken g_mp_##var
#define MICROPROFILE_DEFINE(var, group, name, color) MicroProfileToken g_mp_##var(group, name, color)
#define MICROPROFILE_TOKEN_PASTE0(a, b) a ## b
#define MICROPROFILE_TOKEN_PASTE(a, b)  MICROPROFILE_TOKEN_PASTE0(a,b)
#define MICROPROFILE_SCOPE(var) MicroProfileScopeHandler<false> MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var)
#define MICROPROFILE_SCOPE_THREADSAFE(var) MicroProfileScopeHandler<true> MICROPROFILE_TOKEN_PASTE(foo, __LINE__)(g_mp_##var)
#define MICROPROFILE_SCOPEI(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken(group, name, color); MicroProfileScopeHandler<false> MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_SCOPEI_THREADSAFE(group, name, color) static MicroProfileToken MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__) = MicroProfileGetToken(group, name, color); MicroProfileScopeHandler<true> MICROPROFILE_TOKEN_PASTE(foo,__LINE__)( MICROPROFILE_TOKEN_PASTE(g_mp,__LINE__))
#define MICROPROFILE_TEXT_WIDTH 5
#define MICROPROFILE_TEXT_HEIGHT 8
#define MICROPROFILE_DETAILED_BAR_HEIGHT 12
#define MICROPROFILE_FRAME_TIME (1000.f / 30.f)
#define MICROPROFILE_FRAME_TIME_TO_PRC (1.f / MICROPROFILE_FRAME_TIME)
#define MICROPROFILE_GRAPH_WIDTH 256
#define MICROPROFILE_GRAPH_HEIGHT 256
#define MICROPROFILE_BORDER_SIZE 1
#define MICROPROFILE_MAX_GRAPH_TIME 100.f
#define MICROPROFILE_SINGLE_COUNTERS 0



typedef uint32_t MicroProfileToken;
typedef uint16_t MicroProfileGroupId;


#define MICROPROFILE_INVALID_TOKEN (uint32_t)-1

void MicroProfileInit();
MicroProfileToken MicroProfileGetToken(const char* sGroup, const char* sName, uint32_t nColor);
uint64_t MicroProfileEnter(MicroProfileToken nToken);
void MicroProfileLeave(MicroProfileToken nToken, uint64_t nTick);
void MicroProfileLeaveThreadSafe(MicroProfileToken nToken, uint64_t nTick);
inline uint16_t MicroProfileGetTimerIndex(MicroProfileToken t){ return (t&0xffff); }
inline uint16_t MicroProfileGetGroupMask(MicroProfileToken t){ return ((t>>16)&0xffff);}
inline MicroProfileToken MicroProfileMakeToken(uint16_t nGroupMask, uint16_t nTimer){ return ((uint32_t)nGroupMask<<16) | nTimer;}
inline uint64_t MicroProfileGetPackedCount(uint64_t nTimer)
{
	return nTimer >> 48llu;
}
inline uint64_t MicroProfileGetPackedTicks(uint64_t nTimer)
{
	return nTimer & 0xffffffffffff;
}
inline uint64_t MicroProfilePackTimer(int nCount, uint64_t nTicks)
{
	return ((0x000000000000ffff&(uint64_t)nCount) << 48llu) | (nTicks&0xffffffffffff);
}
inline uint64_t MicroProfileAddTimer(uint64_t nTimer, uint64_t nCount, uint64_t nTicks)
{
	uint64_t nPackedCount = MicroProfileGetPackedCount(nTimer);
	uint64_t nPackedTicks = MicroProfileGetPackedTicks(nTimer);
	nPackedCount += nCount;
	MP_ASSERT(nPackedCount <= 0xffff);
	nPackedTicks += nTicks;
	return MicroProfilePackTimer(nPackedCount, nPackedTicks);
}


void MicroProfileFlip(); //! called once per frame.
void MicroProfileDraw(uint32_t nWidth, uint32_t nHeight); //! call if drawing microprofilers
void MicroProfileSetAggregateCount(uint32_t nCount); //!Set no. of frames to aggregate over. 0 for infinite
void MicroProfileNextAggregatePreset(); //! Flip over some aggregate presets
void MicroProfilePrevAggregatePreset(); //! Flip over some aggregate presets
void MicroProfileNextGroup(); //! Toggle Group of bars displayed
void MicroProfilePrevGroup(); //! Toggle Group of bars displayed
void MicroProfileToggleDisplayMode(); //switch between off, bars, detailed
void MicroProfileToggleTimers();//Toggle timer view
void MicroProfileToggleAverageTimers(); //Toggle average view
void MicroProfileToggleMaxTimers(); //Toggle max view
void MicroProfileToggleCallCount(); // Toggle call count view
void MicroProfileClearGraph();
void MicroProfileToggleFlipDetailed();
void MicroProfileMouseMove(uint32_t nX, uint32_t nY);
void MicroProfileMouseClick(uint32_t nLeft, uint32_t nRight);
void MicroProfileMoveGraph(int nDir, int nPan);
void MicroProfileOnThreadCreate(const char* pThreadName); //should be called from newly created threads

//UNDEFINED: MUST BE IMPLEMENTED ELSEWHERE
void MicroProfileDrawText(uint32_t nX, uint32_t nY, uint32_t nColor, const char* pText);
void MicroProfileDrawBox(uint32_t nX, uint32_t nY, uint32_t nWidth, uint32_t nHeight, uint32_t nColor);
void MicroProfileDrawBoxFade(uint32_t nX, uint32_t nY, uint32_t nX1, uint32_t nY1, uint32_t nColor);
void MicroProfileDrawLine2D(uint32_t nVertices, float* pVertices, uint32_t nColor);

extern void* g_pFUUU;
template<bool bThreadSafe>
struct MicroProfileScopeHandler
{
	MicroProfileToken nToken;
	uint64_t nTick;
	MicroProfileScopeHandler(MicroProfileToken Token):nToken(Token)
	{
		nTick = MicroProfileEnter(nToken);
	}
	~MicroProfileScopeHandler()
	{
		if(bThreadSafe)
		{
			MicroProfileLeaveThreadSafe(nToken, nTick);
		}
		else
		{
			if((void*)this == g_pFUUU)
			{
				int a = 0;
			}
			MicroProfileLeave(nToken, nTick);
		}
	}
};




#ifdef MICRO_PROFILE_IMPL
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <thread>
#include <mutex>

#define S g_MicroProfile
#define MICROPROFILE_MAX_TIMERS 1024
#define MICROPROFILE_MAX_GROUPS 16
#define MICROPROFILE_MAX_GRAPHS 5
#define MICROPROFILE_GRAPH_HISTORY 128
#define MICROPROFILE_LOG_BUFFER_SIZE (((4*2048)<<10)/sizeof(MicroProfileLogEntry))
#define MICROPROFILE_LOG_MAX_THREADS 16
#define MICROPROFILE_STACK_MAX 64


//#include "debug.h"

enum MicroProfileDrawMask
{
	MP_DRAW_OFF		= 0x0,
	MP_DRAW_BARS		= 0x1,
	MP_DRAW_DETAILED	= 0x2,
};
enum MicroProfileDrawBarsMask
{
	MP_DRAW_TIMERS 	= 0x1,
	MP_DRAW_AVERAGE	= 0x2,
	MP_DRAW_MAX		= 0x4,
	MP_DRAW_CALL_COUNT	= 0x8,
	MP_DRAW_ALL = 0xf,

};

struct MicroProfileTimer
{
	uint64_t nTicks;
	uint32_t nCount;
};

struct MicroProfileGroupInfo
{
	const char* pName;
	uint32_t nGroupIndex;
	uint32_t nNumTimers;
};

struct MicroProfileTimerInfo
{
	MicroProfileToken nToken;
	uint32_t nTimerIndex;
	uint32_t nGroupIndex;
	const char* pName;
	uint32_t nColor;
};

struct MicroProfileGraphState
{
	float fHistory[MICROPROFILE_GRAPH_HISTORY];
	MicroProfileToken nToken;
	int32_t nKey;
};

struct MicroProfileLogEntry
{
	enum EType
	{
		EEnter, ELeave,
	};
	EType eType;
	MicroProfileToken nToken;
	uint64_t nTick;
};

struct MicroProfileThreadLog
{
	MicroProfileThreadLog*  pNext;
	MicroProfileLogEntry	Log[MICROPROFILE_LOG_BUFFER_SIZE];

	uint32_t 				nLogPut;
	uint32_t 				nLogGet;
	uint32_t 				nLogPos;
	uint64_t 				nOwningThread;
	uint32_t 				nActive;
	enum{
		THREAD_MAX_LEN = 64,
	};
	char					ThreadName[64];
};


struct 
{
	uint32_t nTotalTimers;
	uint32_t nGroupCount;
	uint32_t nAggregateFlip;
	uint32_t nAggregateFlipCount;
	uint32_t nAggregateFrames;
	
	uint32_t nDisplay;
	uint32_t nBars;
	uint32_t nActiveGroup;

	//menu/mouse over stuff
	uint32_t nMenuActiveGroup;
	uint32_t nMenuAllGroups;
	uint32_t nMenuAllThreads;
	uint32_t nHoverToken;
	uint32_t nHoverTime;

	uint32_t nGroupMask;
	uint32_t nDirty;
	uint32_t nStoredGroup;
	uint32_t nFlipLog;
	uint32_t nMaxGroupSize;
	float fGraphBaseTime;
	float fGraphBaseTimePos;

	uint32_t nWidth;
	uint32_t nHeight;

	uint32_t nBarWidth;
	uint32_t nBarHeight;

	uint64_t nFrameStartPrev;
	uint64_t nFrameStart;


	MicroProfileGroupInfo 	GroupInfo[MICROPROFILE_MAX_GROUPS];
	MicroProfileTimerInfo 	TimerInfo[MICROPROFILE_MAX_TIMERS];
	
	#if MICROPROFILE_SINGLE_COUNTERS
	uint64_t 				FrameTimer[MICROPROFILE_MAX_TIMERS];
	#endif
	MicroProfileTimer 		AggregateTimers[MICROPROFILE_MAX_TIMERS];
	uint64_t				MaxTimers[MICROPROFILE_MAX_TIMERS];

	MicroProfileTimer 		Frame[MICROPROFILE_MAX_TIMERS];
	MicroProfileTimer 		Aggregate[MICROPROFILE_MAX_TIMERS];
	uint64_t				AggregateMax[MICROPROFILE_MAX_TIMERS];

	MicroProfileGraphState	Graph[MICROPROFILE_MAX_GRAPHS];
	uint32_t				nGraphPut;

	uint32_t 				nMouseX;
	uint32_t 				nMouseY;
	uint32_t 				nMouseLeft;
	uint32_t 				nMouseRight;
	uint32_t 				nActiveMenu;

	uint32_t				nThreadActive[MICROPROFILE_LOG_MAX_THREADS];
	MicroProfileThreadLog 	Pool[MICROPROFILE_LOG_MAX_THREADS];
	MicroProfileThreadLog 	DisplayPool[MICROPROFILE_LOG_MAX_THREADS];

	uint64_t DisplayPoolStart;
	uint64_t DisplayPoolEnd;
	
	MicroProfileThreadLog* pFreeThreadLogList;



} g_MicroProfile;

__thread MicroProfileThreadLog* g_MicroProfileThreadLog = 0;
std::mutex g_MicroProfileMutex;
static uint32_t 				g_nMicroProfileBackColors[2] = {  0x474747, 0x313131 };
static uint32_t g_AggregatePresets[] = {0, 10, 20, 30, 60, 120};


template<typename T>
T MicroProfileMin(T a, T b)
{ return a < b ? a : b; }

template<typename T>
T MicroProfileMax(T a, T b)
{ return a > b ? a : b; }


inline uint16_t MicroProfileGetGroupIndex(MicroProfileToken t){ return S.TimerInfo[MicroProfileGetTimerIndex(t)].nGroupIndex;}


#include "debug.h"

void MicroProfileInit()
{
	std::lock_guard<std::mutex> Lock(g_MicroProfileMutex);
	static bool bOnce = true;
	if(bOnce)
	{
		bOnce = false;
		memset(&S, 0, sizeof(S));
		S.nGroupCount = 0;
		S.nBarWidth = 100;
		S.nBarHeight = MICROPROFILE_TEXT_HEIGHT;
		S.nActiveGroup = 1;
		S.nMenuAllGroups = 1;
		S.nMenuActiveGroup = 1;
		S.nMenuAllThreads = 1;
		S.nAggregateFlip = 30;
		S.nTotalTimers = 0;
		for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		}
		S.nBars = MP_DRAW_ALL;
		S.nFlipLog = 1;
		S.fGraphBaseTime = 40.f;
		S.nWidth = 100;
		S.nHeight = 100;
		S.nActiveMenu = (uint32_t)-1;

		for(uint32_t i = 0; i < MICROPROFILE_LOG_MAX_THREADS; ++i)
		{
			if(i + 1 < MICROPROFILE_LOG_MAX_THREADS)
			{
				S.Pool[i].pNext = &S.Pool[i+1];
			}
			else
			{
				S.Pool[i].pNext = 0;
			}
			S.Pool[i].nLogPut = 0;
			S.Pool[i].nLogGet = 0;
		}
		S.pFreeThreadLogList = &S.Pool[0];


	}
}


void MicroProfileOnThreadCreate(const char* pThreadName)
{
	std::lock_guard<std::mutex> Lock(g_MicroProfileMutex);
	MP_ASSERT(g_MicroProfileThreadLog == 0);
	MicroProfileThreadLog* pLog = 0;
	MicroProfileThreadLog* pNext = 0;
	pLog = S.pFreeThreadLogList;
	MP_ASSERT(pLog);
	pNext = pLog->pNext;
	S.pFreeThreadLogList = pNext;
	g_MicroProfileThreadLog = pLog;
	int len = strlen(pThreadName);
	int maxlen = sizeof(pLog->ThreadName)-1;
	len = len < maxlen ? len : maxlen;
	memcpy(&pLog->ThreadName[0], pThreadName, len);
	pLog->ThreadName[len] = '\0';
}



MicroProfileToken MicroProfileGetToken(const char* pGroup, const char* pName, uint32_t nColor)
{
	std::lock_guard<std::mutex> Lock(g_MicroProfileMutex);
	uint16_t nGroupIndex = 0xffff;
	for(uint32_t i = 0; i < S.nGroupCount; ++i)
	{
		if(!strcmp(pGroup, S.GroupInfo[i].pName))
		{
			nGroupIndex = i;
			break;
		}
	}
	if(0xffff == nGroupIndex)
	{
		S.GroupInfo[S.nGroupCount].pName = pGroup;
		S.GroupInfo[S.nGroupCount].nGroupIndex = S.nGroupCount;
		S.GroupInfo[S.nGroupCount].nNumTimers = 0;
		nGroupIndex = S.nGroupCount++;
		S.nGroupMask = (S.nGroupMask<<1)|1;
		uprintf("***** CREATED group %s index %d mask %08x\n", pGroup, nGroupIndex, 1 << nGroupIndex);
		ZASSERT(nGroupIndex < 16);//limit is 16 groups
	}
	uint16_t nTimerIndex = S.nTotalTimers++;
	uint16_t nGroupMask = 1 << nGroupIndex;
	MicroProfileToken nToken = MicroProfileMakeToken(nGroupMask, nTimerIndex);
	S.GroupInfo[nGroupIndex].nNumTimers++;
	S.nMaxGroupSize = MicroProfileMax(S.nMaxGroupSize, S.GroupInfo[nGroupIndex].nNumTimers);
	S.TimerInfo[nTimerIndex].nToken = nToken;
	S.TimerInfo[nTimerIndex].pName = pName;
	S.TimerInfo[nTimerIndex].nColor = nColor;
	S.TimerInfo[nTimerIndex].nGroupIndex = nGroupIndex;
	S.nDirty = 1;

	uprintf("***** CREATED TIMER %08x .. idx %d grp %d grp %s name %s\n", nToken, nTimerIndex, nGroupIndex, S.GroupInfo[nGroupIndex].pName, pName);
	return nToken;
}

inline void MicroProfileLogPut(MicroProfileToken nToken_, uint64_t nTick, MicroProfileLogEntry::EType eEntry)
{
#if MICROPROFILE_SINGLE_COUNTERS
	if(S.nDisplay == MP_DRAW_DETAILED)
#else
	if(S.nDisplay)
#endif
	{
		MicroProfileThreadLog* pLog = g_MicroProfileThreadLog;
		MP_ASSERT(pLog != 0); //this assert is hit if MicroProfileOnCreateThread is not called
		uint32_t nPos = pLog->nLogPut;
		uint32_t nNextPos = (nPos+1) % MICROPROFILE_LOG_BUFFER_SIZE;
		if(nNextPos == pLog->nLogGet)
		{
			uprintf("WARNING BUFFER FULL!!!\n");
		}
		else
		{
			pLog->Log[nPos].nToken = nToken_;
			pLog->Log[nPos].nTick = nTick;
			pLog->Log[nPos].eType = eEntry;
			pLog->nLogPut = nNextPos;
		}
	}		

}

uint64_t MicroProfileEnter(MicroProfileToken nToken_)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		MicroProfileToken nToken = nToken_ & 0xffff;
		uint64_t nTick = MP_TICK();
		MicroProfileLogPut(nToken_, nTick, MicroProfileLogEntry::EEnter);
		return nTick;
	}
	return 0;
}



void MicroProfileLeave(MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		MicroProfileToken nToken = nToken_ & 0xffff;
		uint64_t nTick = MP_TICK();
		#if MICROPROFILE_SINGLE_COUNTERS
		S.FrameTimer[nToken] = MicroProfileAddTimer(S.FrameTimer[nToken], 1, nTick - nTickStart);
		#endif
		MicroProfileLogPut(nToken_, nTick, MicroProfileLogEntry::ELeave);
	}
}

void MicroProfileLeaveThreadSafe(MicroProfileToken nToken_, uint64_t nTickStart)
{
	if(MicroProfileGetGroupMask(nToken_) & S.nActiveGroup)
	{
		MicroProfileToken nToken = nToken_ & 0xffff;
		uint64_t nTick = MP_TICK();
		#if MICROPROFILE_SINGLE_COUNTERS
		uint64_t nTimer, nNewTimer;
		uint64_t nDelta = nTick - nTickStart;
		do
		{
			nTimer = S.FrameTimer[nToken];
			nNewTimer = MicroProfileAddTimer(nTimer, 1, nDelta);
		}while(!MP_CAS_U64(&S.FrameTimer[nToken], nNewTimer, nTimer));
		#endif
		MicroProfileLogPut(nToken_, nTick, MicroProfileLogEntry::ELeave);
	}
}


void MicroProfileFlip()
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileFlip", 0x3355ee);
	uint64_t nFrameStart = S.nFrameStart;
	uint64_t nFrameEnd = MP_TICK();
	S.nFrameStartPrev = S.nFrameStart;
	S.nFrameStart = nFrameEnd;
	//uprintf("FRAME START %llx , %fms\n", nFrameStart, MP_TICK_TO_MS(nFrameStart));


	uint32_t nPutStart[MICROPROFILE_LOG_MAX_THREADS];
	for(uint32_t i = 0; i < MICROPROFILE_LOG_MAX_THREADS; ++i)
	{
		nPutStart[i] = S.Pool[i].nLogPut;
	}
#if MICROPROFILE_SINGLE_COUNTERS
	for(uint32_t i = 0; i < S.nTotalTimers; ++i)
	{
		uint64_t nTicks = MicroProfileGetPackedTicks(S.FrameTimer[i]);
		uint64_t nCount = MicroProfileGetPackedCount(S.FrameTimer[i]);
		S.AggregateTimers[i].nTicks += nTicks;
		S.AggregateTimers[i].nCount += nCount;
		S.Frame[i].nTicks = nTicks;
		S.Frame[i].nCount = nCount;
		S.MaxTimers[i] = MicroProfileMax(S.MaxTimers[i], nTicks);
		S.FrameTimer[i] = 0;
	}
#else
	if(S.nFlipLog)
	{
		{	
			MICROPROFILE_SCOPEI("MicroProfile", "Clear", 0x3355ee);
			for(uint32_t i = 0; i < S.nTotalTimers; ++i)
			{
				//S.FrameTimer[i] = 0;
				S.Frame[i].nTicks = 0;
				S.Frame[i].nCount = 0;
			}
		}
		{
			MICROPROFILE_SCOPEI("MicroProfile", "ThreadLoop", 0x3355ee);
			for(uint32_t i = 0; i < MICROPROFILE_LOG_MAX_THREADS; ++i)
			{
				uint32_t nPut = nPutStart[i];
				uint32_t nGet = S.Pool[i].nLogGet;
				uint32_t nRange[2][2]{ {0, 0}, {0, 0}, };
				MicroProfileThreadLog* pLog = &S.Pool[i];

				if(nPut > nGet)
				{
					nRange[0][0] = nGet;
					nRange[0][1] = nPut;
					nRange[1][0] = nRange[1][1] = 0;
					//uprintf("LOG CNT %d\n",  (nPut - nGet));
				}
				else if(nPut != nGet)
				{
					MP_ASSERT(nGet != MICROPROFILE_LOG_BUFFER_SIZE);
					uint32_t nCountEnd = MICROPROFILE_LOG_BUFFER_SIZE - nGet;
					nRange[0][0] = nGet;
					nRange[0][1] = nGet + nCountEnd;
					nRange[1][0] = 0;
					nRange[1][1] = nPut;
					//uprintf("LOG CNT %d\n", nPut + (nCountEnd - nGet));
				}
		//		uint32_t nSize = pLog->nLogPos;
				uint32_t nMaxStackDepth = 0;
				if(0==S.nThreadActive[i] && 0==S.nMenuAllThreads)
					continue;

				uint32_t nStack[MICROPROFILE_STACK_MAX];
				uint32_t nStackPos = 0;

				for(uint32_t j = 0; j < 2; ++j)
				{
					uint32_t nStart = nRange[j][0];
					uint32_t nEnd = nRange[j][1];
					for(uint32_t k = nStart; k < nEnd; ++k)
					{
						MicroProfileLogEntry& LE = pLog->Log[k];
						switch(LE.eType)
						{
							case MicroProfileLogEntry::EEnter:
							{
								MP_ASSERT(nStackPos < MICROPROFILE_STACK_MAX);
								nStack[nStackPos++] = k;
							}
							break;
							case MicroProfileLogEntry::ELeave:
							{
								uint64_t nTickStart = 0 != nStackPos ? pLog->Log[nStack[nStackPos-1]].nTick : nFrameStart;
								if(0 != nStackPos)
								{
									MP_ASSERT(pLog->Log[nStack[nStackPos-1]].nToken == LE.nToken);
									nStackPos--;
								}
								uint64_t nTicks = LE.nTick - nTickStart;
								uint32_t nTimerIndex = LE.nToken&0xffff;
								if(nTimerIndex == 0)
								{
									//uprintf("LEAVE ADD %f .. nul: %d\n", MP_TICK_TO_MS(nTicks), 0 == nStackPos);
								}
								S.Frame[nTimerIndex].nTicks += nTicks;
								S.Frame[nTimerIndex].nCount += 1;
							}
						}
					}
				}
				for(uint32_t j = 0; j < nStackPos; ++j)
				{
					MicroProfileLogEntry& LE = pLog->Log[nStack[j]];
					uint64_t nTicks = nFrameEnd - LE.nTick;
					uint32_t nTimerIndex = LE.nToken&0xffff;
					S.Frame[nTimerIndex].nTicks += nTicks;
					//uprintf("Remains ADD %f .. nul: %d\n", MP_TICK_TO_MS(nTicks));
				}
			}
		}
		{
			MICROPROFILE_SCOPEI("MicroProfile", "Accumulate", 0x3355ee);
			for(uint32_t i = 0; i < S.nTotalTimers; ++i)
			{
				S.AggregateTimers[i].nTicks += S.Frame[i].nTicks;
				S.AggregateTimers[i].nCount += S.Frame[i].nCount;
				S.MaxTimers[i] = MicroProfileMax(S.MaxTimers[i], S.Frame[i].nTicks);
			}
		}
	}
#endif
	bool bFlipAggregate = false;
	uint32_t nFlipFrequency = S.nAggregateFlip ? S.nAggregateFlip : 30;
	if(S.nFlipLog && S.nAggregateFlip <= ++S.nAggregateFlipCount)
	{
		memcpy(&S.Aggregate[0], &S.AggregateTimers[0], sizeof(S.Aggregate[0]) * S.nTotalTimers);
		memcpy(&S.AggregateMax[0], &S.MaxTimers[0], sizeof(S.AggregateMax[0]) * S.nTotalTimers);
		S.nAggregateFrames = S.nAggregateFlipCount;
		if(S.nAggregateFlip)// if 0 accumulate indefinitely
		{
			memset(&S.AggregateTimers[0], 0, sizeof(S.Aggregate[0]) * S.nTotalTimers);
			memset(&S.MaxTimers[0], 0, sizeof(S.MaxTimers[0]) * S.nTotalTimers);
			S.nAggregateFlipCount = 0;
		}
		if(S.nFlipLog)
		{
			for(uint32_t i = 0; i < MICROPROFILE_LOG_MAX_THREADS; ++i)
			{
				uint32_t nPut = nPutStart[i];
				uint32_t nGet = S.Pool[i].nLogGet;
				if(nPut > nGet)
				{
					MP_ASSERT(nPut < MICROPROFILE_LOG_BUFFER_SIZE);
					memcpy(&S.DisplayPool[i].Log[0], &S.Pool[i].Log[nGet], (nPut - nGet) * sizeof(S.Pool[0].Log[0]));
					S.DisplayPool[i].nLogPos = nPut - nGet;
				}
				else if(nPut != nGet)
				{
					MP_ASSERT(nGet != MICROPROFILE_LOG_BUFFER_SIZE);
					uint32_t nCountEnd = MICROPROFILE_LOG_BUFFER_SIZE - nGet;
					memcpy(&S.DisplayPool[i].Log[0], &S.Pool[i].Log[nGet], nCountEnd * sizeof(S.Pool[0].Log[0]));
					memcpy(&S.DisplayPool[i].Log[nCountEnd], &S.Pool[i].Log[0], nPut * sizeof(S.Pool[0].Log[0]));
					S.DisplayPool[i].nLogPos = nCountEnd + nPut;
				}
				else
				{
					S.DisplayPool[i].nLogPos = 0;
				}
				S.DisplayPool[i].nLogGet = 0;
				S.DisplayPool[i].nLogPut = 0;
				S.DisplayPool[i].nOwningThread = S.Pool[i].nOwningThread;
				memcpy(&S.DisplayPool[i].ThreadName[0], &S.Pool[i].ThreadName[0], sizeof(S.Pool[i].ThreadName));
			}
			S.DisplayPoolStart = nFrameStart;
			S.DisplayPoolEnd = nFrameEnd;
		}
	}
	for(uint32_t i = 0; i < MICROPROFILE_LOG_MAX_THREADS; ++i)
	{
		S.Pool[i].nLogGet = nPutStart[i];
	}
}




void MicroProfileSetAggregateCount(uint32_t nCount)
{
	S.nAggregateFlip = nCount;
}
void MicroProfileNextAggregatePreset()
{
	uint32_t nNext = g_AggregatePresets[0];
	uint32_t nCurrent = S.nAggregateFlip;
	for(uint32_t nPreset : g_AggregatePresets)
	{
		if(nPreset > nCurrent)
		{
			nCurrent = nPreset;
			break;
		}
	}
	S.nAggregateFlip = nCurrent;
	S.nDirty = 1;
}
void MicroProfilePrevAggregatePreset()
{
	S.nDirty = 1;
	uint32_t nCurrent = S.nAggregateFlip;
	uint32_t nBest = g_AggregatePresets[(sizeof(g_AggregatePresets)/sizeof(g_AggregatePresets[0]))-1];

	for(uint32_t nPreset : g_AggregatePresets)
	{
		if(nPreset < nCurrent)
		{
			nBest = nPreset;
		}
	}
	S.nAggregateFlip = nBest;
}

void MicroProfileNextGroup()
{
	if(0xffff == S.nActiveGroup) return;
	S.nActiveGroup = (S.nActiveGroup+1)%S.nGroupCount;
	MicroProfileClearGraph();
}
void MicroProfilePrevGroup()
{
	if(0xffff == S.nActiveGroup) return;
	S.nActiveGroup = (S.nActiveGroup+S.nGroupCount-1)%S.nGroupCount;
	MicroProfileClearGraph();
}

void MicroProfileToggleDisplayMode()
{
	switch(S.nDisplay)
	{
		case 2:
		{
			S.nDisplay = 0;
			S.nActiveGroup = S.nStoredGroup;
		}
		break;
		case 1:
		{
			S.nDisplay = 2;
			S.nStoredGroup = S.nActiveGroup;
			S.nActiveGroup = 0xffff;
		}
		break;		
		case 0:
		{
			S.nDisplay = 1;
		}
		break;		
	}
}
void MicroProfileToggleTimers()
{
	S.nBars ^= MP_DRAW_TIMERS;
}
void MicroProfileToggleAverageTimers()
{
	S.nBars ^= MP_DRAW_AVERAGE;
}
void MicroProfileToggleMaxTimers()
{
	S.nBars ^= MP_DRAW_MAX;
}

void MicroProfileToggleCallCount()
{
	S.nBars ^= MP_DRAW_CALL_COUNT;
}


void MicroProfileDrawFloatInfo(uint32_t nX, uint32_t nY, uint32_t nToken, uint64_t nTime)
{
#define MAX_STRINGS 32
	uint32_t nIndex = MicroProfileGetTimerIndex(nToken);
	uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
	uint32_t nAggregateCount = S.Aggregate[nIndex].nCount ? S.Aggregate[nIndex].nCount : 1;

	float fMs = MP_TICK_TO_MS(nTime);
	float fFrameMs = MP_TICK_TO_MS(S.Frame[nIndex].nTicks);
	float fAverage = MP_TICK_TO_MS(S.Aggregate[nIndex].nTicks/nAggregateFrames);
	float fCallAverage = MP_TICK_TO_MS(S.Aggregate[nIndex].nTicks / nAggregateCount);
	float fMax = MP_TICK_TO_MS(S.AggregateMax[nIndex]);

	{
		const char* ppStrings[MAX_STRINGS];
		uint32_t nColors[MAX_STRINGS];
		memset(&nColors[0], 0xff, sizeof(nColors));
		char buffer[1024];
		const char* pBufferStart = &buffer[0];
		char* pBuffer = &buffer[0];

		uint32_t nGroupId = MicroProfileGetGroupIndex(nToken);
		uint32_t nTimerId = MicroProfileGetTimerIndex(nToken);
		const char* pGroupName = S.GroupInfo[nGroupId].pName;
		const char* pTimerName = S.TimerInfo[nTimerId].pName;
#define SZ (intptr_t)(sizeof(buffer)-1-(pBufferStart - pBuffer))
		uint32_t nTextCount = 0;


		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer,SZ, "%s", pGroupName);
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer,SZ, "%s", pTimerName);
		
		if(nTime != (uint64_t)-1)
		{
			ppStrings[nTextCount++] = "Time:";
			ppStrings[nTextCount++] = pBuffer;
			pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fMs);
			ppStrings[nTextCount++] = "";
			ppStrings[nTextCount++] = "";
		}

		ppStrings[nTextCount++] = "Frame Time:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fFrameMs);

		ppStrings[nTextCount++] = "Frame Call Average:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fCallAverage);

		ppStrings[nTextCount++] = "Frame Call Count:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ, "%6d",  nAggregateCount / nAggregateFrames);
		
		ppStrings[nTextCount++] = "Average:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fAverage);

		ppStrings[nTextCount++] = "Max:";
		ppStrings[nTextCount++] = pBuffer;
		pBuffer += 1 + snprintf(pBuffer, SZ,"%6.3fms",  fMax);
		
		MP_ASSERT(nTextCount < MAX_STRINGS);
		nTextCount /= 2;
#undef SZ
		uint32_t nWidth = 0;
		uint32_t nStringLengths[MAX_STRINGS];
		for(uint32_t i = 0; i < nTextCount; ++i)
		{
			uint32_t i0 = i * 2;
			uint32_t s0, s1;
			nStringLengths[i0] = s0 = strlen(ppStrings[i0]);
			nStringLengths[i0+1] = s1 = strlen(ppStrings[i0+1]);
			nWidth = MicroProfileMax(s0+s1, nWidth);
		}
		nWidth = (MICROPROFILE_TEXT_WIDTH+1) * (2+nWidth) + 2 * MICROPROFILE_BORDER_SIZE;
		uint32_t nHeight = (MICROPROFILE_TEXT_HEIGHT+1) * nTextCount + 2 * MICROPROFILE_BORDER_SIZE;
		nY += 20;
		if(nX + nWidth > S.nWidth)
			nX = S.nWidth - nWidth;
		if(nY + nHeight > S.nHeight)
			nY = S.nHeight - nHeight;
		MicroProfileDrawBox(nX, nY, nWidth, nHeight, 0);
		for(uint32_t i = 0; i < nTextCount; ++i)
		{
			int i0 = i * 2;
			MicroProfileDrawText(nX + 1, nY + 1 + (MICROPROFILE_TEXT_HEIGHT+1) * i, (uint32_t)-1, ppStrings[i0]);
			MicroProfileDrawText(nX + nWidth - nStringLengths[i0+1] * (MICROPROFILE_TEXT_WIDTH+1), nY + 1 + (MICROPROFILE_TEXT_HEIGHT+1) * i, (uint32_t)-1, ppStrings[i0+1]);
		}

	}
#undef MAX_STRINGS
}
void MicroProfileDrawDetailedView(uint32_t nWidth, uint32_t nHeight)
{
	MICROPROFILE_SCOPEI("MicroProfile", "Detailed View", 0x8888000);
	uint64_t nFrameEnd = S.DisplayPoolEnd;
	uint64_t nFrameStart = S.DisplayPoolStart;

	uint32_t nX = 0;
	uint32_t nY = S.nBarHeight + 1;

	float fMsBase = S.fGraphBaseTimePos;
	uint64_t nBaseTicks = MP_MS_TO_TICK(fMsBase);
	float fMs = S.fGraphBaseTime;
	float fMsEnd = fMs + fMsBase;
	float fWidth = nWidth;
	float fMsToScreen = fWidth / fMs;

	int nColorIndex = floor(fMsBase);
	int i = 0;
	int nSkip = (fMsEnd-fMsBase)> 50 ? 2 : 1;
	for(float f = fMsBase; f < fMsEnd; ++i)
	{
		float fStart = f;
		float fNext = MicroProfileMin<float>(floor(f)+1.f, fMsEnd);
		uint32_t nXPos = nX + ((fStart-fMsBase) * fMsToScreen);
		MicroProfileDrawBox(nXPos, nY, (fNext-fMsBase) * fMsToScreen+1, nHeight, g_nMicroProfileBackColors[nColorIndex++ & 1]);
		f = fNext;
	}

	for(float f = fMsBase; f < fMsEnd; ++i)
	{
		float fStart = f;
		float fNext = MicroProfileMin<float>(floor(f)+1.f, fMsEnd);
		uint32_t nXPos = nX + ((fStart-fMsBase) * fMsToScreen);
		if(0 == (i%nSkip))
		{
			char buf[10];
			snprintf(buf, 9, "%d", (int)f);
			MicroProfileDrawText(nXPos, nY, (uint32_t)-1, buf);
		}
		f = fNext;
	}
	nY += MICROPROFILE_TEXT_HEIGHT + 1;


	float fMouseX = S.nMouseX;
	float fMouseY = S.nMouseY;
	uint32_t nHoverToken = -1;
	uint64_t nHoverTime = 0;
	float fHoverDist = 0.5f;
	float fBestDist = 100000.f;
	for(uint32_t j = 0; j < MICROPROFILE_LOG_MAX_THREADS; ++j)
	{
		MicroProfileThreadLog* pLog = &S.DisplayPool[j];
		uint32_t nSize = pLog->nLogPos;
		uint32_t nMaxStackDepth = 0;
		if(0==S.nThreadActive[j] && 0==S.nMenuAllThreads)
			continue;

		nY += 3;
		MicroProfileDrawText(nX, nY, (uint32_t)-1, &pLog->ThreadName[0]);
		nY += 3;
		nY += MICROPROFILE_TEXT_HEIGHT + 1;

		uint32_t nStack[MICROPROFILE_STACK_MAX];
		uint32_t nStackPos = 0;
		for(uint32_t i = 0; i < nSize; ++i)
		{
			MicroProfileLogEntry& LE = pLog->Log[i];
			switch(LE.eType)
			{
				case MicroProfileLogEntry::EEnter:
				{
					MP_ASSERT(nStackPos < MICROPROFILE_STACK_MAX);
					nStack[nStackPos++] = i;
					nMaxStackDepth = MicroProfileMax(nStackPos, nMaxStackDepth);

				}
				break;
				case MicroProfileLogEntry::ELeave:
				{
					if(0 == nStackPos)
						continue;
					MP_ASSERT(pLog->Log[nStack[nStackPos-1]].nToken == LE.nToken);
					uint64_t nTickStart = pLog->Log[nStack[nStackPos-1]].nTick;
					nStackPos--;
					uint64_t nTickEnd = LE.nTick;
					uint32_t nColor = S.TimerInfo[ LE.nToken & 0xffff].nColor;

					float fMsStart = MP_TICK_TO_MS(nTickStart - nFrameStart - nBaseTicks);
					float fMsEnd = MP_TICK_TO_MS(nTickEnd - nFrameStart - nBaseTicks);
					MP_ASSERT(fMsStart <= fMsEnd);
					float fXStart = nX + fMsStart * fMsToScreen;
					float fXEnd = nX + fMsEnd * fMsToScreen;


					float fYStart = nY + nStackPos * (MICROPROFILE_DETAILED_BAR_HEIGHT+1);
					float fYEnd = fYStart + (MICROPROFILE_DETAILED_BAR_HEIGHT);
					uint32_t nIntegerWidth = fXEnd - fXStart;
					float fXDist = MicroProfileMax(fXStart - fMouseX, fMouseX - fXEnd);

					if(fXDist < fHoverDist)
					{
						if(fYStart <= fMouseY && fMouseY <= fYEnd)
						{
							fHoverDist = fXDist;
							nHoverToken = LE.nToken;
							nHoverTime = nTickEnd - nTickStart;
						}
					}

					if(nIntegerWidth)
					{
						MicroProfileDrawBoxFade(fXStart, fYStart, fXEnd, fYEnd, nColor);
					}
					else
					{
						float fXAvg = 0.5f * (fXStart + fXEnd);
						float fLine[] = {
							fXAvg, fYStart + 0.5f,
							fXAvg, fYEnd + 0.5f

						};
						MicroProfileDrawLine2D(2, &fLine[0], nColor);
					}
					
				}
				break;
			}
		}
		nY += nMaxStackDepth * (MICROPROFILE_DETAILED_BAR_HEIGHT+1);
	}
	if(nHoverToken != (uint32_t)-1 && nHoverTime)
	{
		S.nHoverToken = nHoverToken;
		S.nHoverTime = nHoverTime;
	}
}



void MicroProfileLoopActiveGroups(uint32_t nX, uint32_t nY, const char* pName, std::function<void (uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)> CB)
{
	if(pName)
		MicroProfileDrawText(nX, nY, (uint32_t)-1, pName);

	nY += S.nBarHeight + 2;
	uint32_t nGroup = S.nActiveGroup;
	uint32_t nGroupMask = (uint32_t)-1;
	uint32_t nCount = 0;
	//TODO: do this is one pass through the data
	#if 0
	for(uint32_t i = 0; i < S.nTotalTimers;++i)
	{
		uint32_t nMask = MicroProfileGetGroupMask(S.TimerInfo[i].nToken);
		if(0 != (nMask & nGroup))
		{
			if(nMask != nGroupMask)
			{
				nY += S.nBarHeight + 1;
				nGroupMask = nMask;
			}
			CB(i, nCount, nGroupMask, nX, nY);
			nCount += 2;
			nY += S.nBarHeight + 1;
		}
	}

	#else
	for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
	{
		uint32_t nMask = 1 << j;
		if(nMask & nGroup)
		{
			nY += S.nBarHeight + 1;
			for(uint32_t i = 0; i < S.nTotalTimers;++i)
			{
				uint32_t nTokenMask = MicroProfileGetGroupMask(S.TimerInfo[i].nToken);
				if(nTokenMask & nMask)
				{
					CB(i, nCount, nMask, nX, nY);
					nCount += 2;
					nY += S.nBarHeight + 1;
				}
			}
		}
	}
	#endif
}

uint32_t MicroProfileLoopActiveGroupsFake(uint32_t nX, uint32_t nY, const char* pName, std::function<void (uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)> CB)
{
	// if(pName)
	// 	MicroProfileDrawText(nX, nY, (uint32_t)-1, pName);


	nY += S.nBarHeight + 2;
	uint32_t nGroup = S.nActiveGroup;
	uint32_t nGroupMask = (uint32_t)-1;
	uint32_t nCount = 0;

	for(uint32_t j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
	{
		uint32_t nMask = 1 << j;
		if(nMask & nGroup)
		{
			nY += S.nBarHeight + 1;
			for(uint32_t i = 0; i < S.nTotalTimers;++i)
			{
				uint32_t nTokenMask = MicroProfileGetGroupMask(S.TimerInfo[i].nToken);
				if(nTokenMask & nMask)
				{
					//CB(i, nCount, nMask, nX, nY);
					nCount += 2;
					nY += S.nBarHeight + 1+ nCount;
				}
			}
		}
	}
	return nY;
}



void MicroProfileCalcTimers(float* pTimers, float* pAverage, float* pMax, float* pCallAverage, uint16_t nGroup, uint32_t nSize)
{
	MICROPROFILE_SCOPEI("MicroProfile", "MicroProfileCalcTimers", 0x773300);
	MicroProfileLoopActiveGroups(0, 0, 0, 
		[&](uint32_t nTimer, uint32_t nIdx, uint32_t nGroupMask, uint32_t nX, uint32_t nY){
			uint32_t nAggregateFrames = S.nAggregateFrames ? S.nAggregateFrames : 1;
			uint32_t nAggregateCount = S.Aggregate[nTimer].nCount ? S.Aggregate[nTimer].nCount : 1;
			float fMs = MicroProfileTickToNs(S.Frame[nTimer].nTicks) / 1000000.f;
			float fPrc = MicroProfileMin(fMs * MICROPROFILE_FRAME_TIME_TO_PRC, 1.f);
			float fAverageMs = MicroProfileTickToNs(S.Aggregate[nTimer].nTicks / nAggregateFrames) / 1000000.f;
			float fAveragePrc = MicroProfileMin(fAverageMs * MICROPROFILE_FRAME_TIME_TO_PRC, 1.f);
			float fMaxMs = MicroProfileTickToNs(S.AggregateMax[nTimer]) / 1000000.f;
			float fMaxPrc = MicroProfileMin(fMaxMs * MICROPROFILE_FRAME_TIME_TO_PRC, 1.f);
			float fCallAverageMs = MicroProfileTickToNs(S.Aggregate[nTimer].nTicks / nAggregateCount) / 1000000.f;
			float fCallAveragePrc = MicroProfileMin(fCallAverageMs * MICROPROFILE_FRAME_TIME_TO_PRC, 1.f);
			//ZASSERT(nCount++ < nSize);
			pTimers[nIdx] = fMs;
			pTimers[nIdx+1] = fPrc;
			pAverage[nIdx] = fAverageMs;
			pAverage[nIdx+1] = fAveragePrc;
			pMax[nIdx] = fMaxMs;
			pMax[nIdx+1] = fMaxPrc;
			pCallAverage[nIdx] = fCallAverageMs;
			pCallAverage[nIdx+1] = fCallAveragePrc;
		}
	);
}

#define SBUF_MAX 32

uint32_t MicroProfileDrawBarArray(uint32_t nX, uint32_t nY, float* pTimers, const char* pName)
{
	const uint32_t nHeight = S.nBarHeight;
	const uint32_t nWidth = S.nBarWidth;
	const uint32_t nTextWidth = 6 * (1+MICROPROFILE_TEXT_WIDTH);
	const float fWidth = S.nBarWidth;
	MicroProfileLoopActiveGroups(nX, nY, pName, 
		[=](uint32_t nTimer, uint32_t nIdx, uint32_t nGroupMask, uint32_t nX, uint32_t nY){
			//HER:
			char sBuffer[SBUF_MAX];
			snprintf(sBuffer, SBUF_MAX-1, "%5.2f", pTimers[nIdx]);
			MicroProfileDrawBoxFade(nX + nTextWidth, nY, nX + nTextWidth + fWidth * pTimers[nIdx+1], nY + nHeight, S.TimerInfo[nTimer].nColor);
			MicroProfileDrawText(nX, nY, (uint32_t)-1, sBuffer);
			
		});
	return nWidth + 5 + nTextWidth;

}

uint32_t MicroProfileDrawBarCallCount(uint32_t nX, uint32_t nY, const char* pName)
{
	MicroProfileLoopActiveGroups(nX, nY, pName, 
		[](uint32_t nTimer, uint32_t nIdx, uint32_t nGroupMask, uint32_t nX, uint32_t nY){
			char sBuffer[SBUF_MAX];
			snprintf(sBuffer, SBUF_MAX-1, "%5d", S.Frame[nTimer].nCount);//fix
			MicroProfileDrawText(nX, nY, (uint32_t)-1, sBuffer);
		});
	uint32_t nTextWidth = 6 * MICROPROFILE_TEXT_WIDTH;
	return 5 + nTextWidth;
}



uint32_t MicroProfileDrawBarLegend(uint32_t nX, uint32_t nY)
{
	MicroProfileLoopActiveGroups(nX, nY, 0, 
		[](uint32_t nTimer, uint32_t nIdx, uint32_t nGroupMask, uint32_t nX, uint32_t nY){
			MicroProfileDrawText(nX, nY, S.TimerInfo[nTimer].nColor, S.TimerInfo[nTimer].pName);
			if(S.nMouseY >= nY && S.nMouseY < nY + MICROPROFILE_TEXT_HEIGHT+1  && S.nMouseX < nX + 20 * (MICROPROFILE_TEXT_WIDTH+1))
			{
				S.nHoverToken = nTimer;
				S.nHoverTime = 0;
			}
		});
	return nX;
}

void MicroProfileDrawGraph(uint32_t nScreenWidth, uint32_t nScreenHeight)
{
	MICROPROFILE_SCOPEI("MicroProfile", "DrawGraph", 0xff0033);
	uint32_t nX = nScreenWidth - MICROPROFILE_GRAPH_WIDTH;
	uint32_t nY = nScreenHeight - MICROPROFILE_GRAPH_HEIGHT;
	MicroProfileDrawBox(nX, nY, MICROPROFILE_GRAPH_WIDTH, MICROPROFILE_GRAPH_HEIGHT, 0x0);

	
	float fY = nScreenHeight;
	float fDX = MICROPROFILE_GRAPH_WIDTH * 1.f / MICROPROFILE_GRAPH_HISTORY;  
	float fDY = MICROPROFILE_GRAPH_HEIGHT;

	uint32_t nPut = S.nGraphPut;
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		if(S.Graph[i].nToken != MICROPROFILE_INVALID_TOKEN)
		{
			float fX = nX;
			float* pGraphData = (float*)alloca(sizeof(float)* MICROPROFILE_GRAPH_HISTORY*2);
			for(uint32_t j = 0; j < MICROPROFILE_GRAPH_HISTORY; ++j)
			{
				float fWeigth = S.Graph[i].fHistory[(j+nPut)%MICROPROFILE_GRAPH_HISTORY];
				pGraphData[(j*2)] = fX;
				pGraphData[(j*2)+1] = fY - fDY * fWeigth;
				fX += fDX;
			}
			MicroProfileDrawLine2D(MICROPROFILE_GRAPH_HISTORY, pGraphData, S.TimerInfo[MicroProfileGetTimerIndex(S.Graph[i].nToken)].nColor);
		}
	}
}

void MicroProfileDrawBarView(uint32_t nScreenWidth, uint32_t nScreenHeight)
{
	if(!S.nActiveGroup)
		return;
	MICROPROFILE_SCOPEI("MicroProfile", "DrawBarView", 0x00dd77);

	const uint32_t nHeight = S.nBarHeight;
	const uint32_t nWidth = S.nBarWidth;
	int nColorIndex = 0;
	uint32_t nX = 0;
	uint32_t nY = nHeight + 1;
	
	uint32_t nNumTimers = 0;
	uint32_t nNumGroups = 0;
	for(uint32 j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
	{
		if(S.nActiveGroup & (1 << j))
		{
			nNumTimers += S.GroupInfo[j].nNumTimers;
			nNumGroups += 1;
		}
	}
	uint32_t nBlockSize = 2 * nNumTimers;
	float* pTimers = (float*)alloca(nBlockSize * 4 * sizeof(float));
	float* pAverage = pTimers + nBlockSize;
	float* pMax = pTimers + 2 * nBlockSize;
	float* pCallAverage = pTimers + 3 * nBlockSize;
	MicroProfileCalcTimers(pTimers, pAverage, pMax, pCallAverage, S.nActiveGroup, nNumTimers);
	for(uint32_t i = 0; i < nNumTimers+nNumGroups+1; ++i)
	{
		MicroProfileDrawBox(nX, nY + i * (nHeight + 1), nScreenWidth, (nHeight+1)+1, g_nMicroProfileBackColors[nColorIndex++ & 1]);
	}
	uint32_t nLegendOffset = 1;
	for(uint32 j = 0; j < MICROPROFILE_MAX_GROUPS; ++j)
	{
		if(S.nActiveGroup & (1 << j))
		{
			MicroProfileDrawText(nX, nY + (1+nHeight) * nLegendOffset, (uint32_t)-1, S.GroupInfo[j].pName);
			nLegendOffset += S.GroupInfo[j].nNumTimers+1;
		}
	}
	if(S.nBars & MP_DRAW_TIMERS)		
		nX += MicroProfileDrawBarArray(nX, nY, pTimers, "Time") + 1;
	if(S.nBars & MP_DRAW_AVERAGE)		
		nX += MicroProfileDrawBarArray(nX, nY, pAverage, "Average") + 1;
	if(S.nBars & MP_DRAW_MAX)		
		nX += MicroProfileDrawBarArray(nX, nY, pMax, "Max Time") + 1;
	if(S.nBars & MP_DRAW_CALL_COUNT)		
	{
		nX += MicroProfileDrawBarArray(nX, nY, pCallAverage, "Call Average") + 1;
		nX += MicroProfileDrawBarCallCount(nX, nY, "Count") + 1; 
	}
	nX += MicroProfileDrawBarLegend(nX, nY) + 1;

	uint32_t tidx = 0;
	// for(uint32_t i = 0; i < S.nTotalTimers;++i)
	// {
	// 	if(0 == i || 0 != (MicroProfileGetGroupMask(S.TimerInfo[i].nToken) & S.nActiveGroup))
	// 	{
	// 		for(uint32_t j = 0; j < MICROPROFILE_MAX_GRAPHS; ++j)
	// 		{
	// 			if(S.Graph[j].nToken != MICROPROFILE_INVALID_TOKEN && S.TimerInfo[i].nToken == S.Graph[j].nToken)
	// 			{
	// 				S.Graph[j].fHistory[S.nGraphPut] = pTimers[1+tidx];
	// 			}
	// 		}
	// 		tidx += 2;
	// 	}
	// }
	// S.nGraphPut = (S.nGraphPut+1) % MICROPROFILE_GRAPH_HISTORY;
	// MicroProfileDrawGraph(nScreenWidth, nScreenHeight);
}

void MicroProfileDrawMenu(uint32_t nWidth, uint32_t nHeight)
{
	uint32_t nX = 0;
	uint32_t nY = 0;
	char buffer[128];


	MICROPROFILE_SCOPEI("MicroProfile", "Menu", 0x0373e3);
	MicroProfileDrawBox(nX, nY, nWidth, (S.nBarHeight+1)+1, g_nMicroProfileBackColors[1]);

#define MICROPROFILE_MENU_MAX 16
	const char* pMenuText[MICROPROFILE_MENU_MAX] = {0};
	uint32_t 	nMenuX[MICROPROFILE_MENU_MAX] = {0};
	uint32_t nNumMenuItems = 0;

	snprintf(buffer, 127, "MicroProfile");
	MicroProfileDrawText(nX, nY, -1, buffer);
	nX += (sizeof("MicroProfile")+2) * (MICROPROFILE_TEXT_WIDTH+1);
	//mode
	pMenuText[nNumMenuItems++] = "Mode";
	pMenuText[nNumMenuItems++] = "Group";
	pMenuText[nNumMenuItems++] = "Thread";
	pMenuText[nNumMenuItems++] = "Aggregate";
	pMenuText[nNumMenuItems++] = "Timers";
	const int nPauseIndex = nNumMenuItems;
	pMenuText[nNumMenuItems++] = S.nFlipLog ? "Pause" : "Unpause";


	typedef std::function<const char* (int, bool&)> SubmenuCallback; 
	typedef std::function<void(int)> ClickCallback;
	SubmenuCallback GroupCallback[] = 
	{	[] (int index, bool& bSelected) -> const char*{
			switch(index)
			{
				case 0: 
					bSelected = S.nDisplay == MP_DRAW_DETAILED;
					return "Detailed";
				case 1:
					bSelected = S.nDisplay == MP_DRAW_BARS; 
					return "Timers";
				default: return 0;
			}
		},
		[] (int index, bool& bSelected) -> const char*{
			if(index == 0)
			{
				bSelected = S.nMenuAllGroups != 0;
				return "ALL";
			}
			else
			{
				index = index-1;
				bSelected = 0 != (S.nMenuActiveGroup & (1 << index));
				if(index < MICROPROFILE_MAX_GROUPS && S.GroupInfo[index].pName)
					return S.GroupInfo[index].pName;
				else
					return 0;
			}
		},
		[] (int index, bool& bSelected) -> const char*{
			if(0 == index)
			{
				bSelected = S.nMenuAllThreads != 0;
				return "All Threads";
			}
			else if(index-1 < MICROPROFILE_LOG_MAX_THREADS)
			{
				bSelected = S.nThreadActive[index-1];
				return S.Pool[index-1].ThreadName[0]?&S.Pool[index-1].ThreadName[0]:0;
			}
			return 0;
		},
		[] (int index, bool& bSelected) -> const char*{
			if(index < sizeof(g_AggregatePresets)/sizeof(g_AggregatePresets[0]))
			{
				int val = g_AggregatePresets[index];
				bSelected = S.nAggregateFlip == val;
				if(0 == val)
					return "Infinite";
				else
				{
					static char buf[128];
					snprintf(buf, sizeof(buffer)-1, "%7d", val);
					return buf;
				}
			}
			return 0;
		},
		[] (int index, bool& bSelected) -> const char*{
			bSelected = 0 != (S.nBars & (1 << index));
			switch(index)
			{
				case 0: return "Timers";
				case 1: return "Average";
				case 2: return "Max";
				case 3: return "Call Count";
			}
			return 0;
		},
		[] (int index, bool& bSelected) -> const char*{
			return 0;
		},
	};
	ClickCallback CBClick[] = 
	{
		[](int nIndex)
		{
			if(0 == nIndex)
				S.nDisplay = MP_DRAW_DETAILED;
			else
				S.nDisplay = MP_DRAW_BARS;
		},
		[](int nIndex)
		{
			if(nIndex == 0)
				S.nMenuAllGroups = 1-S.nMenuAllGroups;
			else
				S.nMenuActiveGroup ^= (1 << (nIndex-1));
		},
		[](int nIndex)
		{
			if(nIndex == 0)
				S.nMenuAllThreads = 1-S.nMenuAllThreads;
			else
			{
				S.nThreadActive[nIndex-1] = 1-S.nThreadActive[nIndex-1];
			}
		},
		[](int nIndex)
		{
			S.nAggregateFlip = g_AggregatePresets[nIndex];
		},
		[](int nIndex)
		{
			S.nBars ^= (1 << nIndex);
		},
		[](int nIndex)
		{
		},
	};


	uint32_t nSelectMenu = (uint32_t)-1;
	for(uint32_t i = 0; i < nNumMenuItems; ++i)
	{
		nMenuX[i] = nX;
		int nLen = strlen(pMenuText[i]);
		int nEnd = nX + nLen * (MICROPROFILE_TEXT_WIDTH+1);
		if(S.nMouseY <= MICROPROFILE_TEXT_HEIGHT && S.nMouseX <= nEnd && S.nMouseX >= nX)
		{
			MicroProfileDrawBox(nX-1, nY, 1+nLen * (MICROPROFILE_TEXT_WIDTH+1), (S.nBarHeight+1)+1, 0x888888);
			nSelectMenu = i;
			if((S.nMouseLeft || S.nMouseRight) && i == nPauseIndex)
			{
				S.nFlipLog = !S.nFlipLog;
			}


		}
		MicroProfileDrawText(nX, nY, -1, pMenuText[i]);
		nX += (nLen+1) * (MICROPROFILE_TEXT_WIDTH+1);
	}
	uint32_t nMenu = nSelectMenu != (uint32_t)-1 ? nSelectMenu : S.nActiveMenu;
	S.nActiveMenu = nMenu;
	if((uint32_t)-1 != nMenu)
	{
		#define SBUF_SIZE 256
		char sBuffer[SBUF_SIZE];
		nX = nMenuX[nMenu];
		nY += MICROPROFILE_TEXT_HEIGHT+1;
		SubmenuCallback CB = GroupCallback[nMenu];
		int nNumLines = 0;
		bool bSelected = false;
		const char* pString = CB(nNumLines, bSelected);
		uint32_t nWidth = 0, nHeight = 0;
		while(pString)
		{
			nWidth = MicroProfileMax<int>(nWidth, strlen(pString));
			nNumLines++;
			pString = CB(nNumLines, bSelected);
		}
		nWidth = (2+nWidth) * (MICROPROFILE_TEXT_WIDTH+1);
		nHeight = nNumLines * (MICROPROFILE_TEXT_HEIGHT+1);
		if(S.nMouseY <= nY + nHeight+0 && S.nMouseY >= nY-0 && S.nMouseX <= nX + nWidth+0 && S.nMouseX >= nX-0)
		{
			S.nActiveMenu = nMenu;
		}
		else if(nSelectMenu == (uint32_t)-1)
		{
			S.nActiveMenu = (uint32_t)-1;
		}
		MicroProfileDrawBox(nX, nY, nWidth, nHeight, g_nMicroProfileBackColors[1]);
		for(int i = 0; i < nNumLines; ++i)
		{
			bool bSelected = false;
			const char* pString = CB(i, bSelected);
			if(S.nMouseY >= nY && S.nMouseY < nY + MICROPROFILE_TEXT_HEIGHT + 1)
			{
				if(S.nMouseLeft || S.nMouseRight)
				{
					CBClick[nMenu](i);
				}
				MicroProfileDrawBox(nX, nY, nWidth, MICROPROFILE_TEXT_HEIGHT + 1, 0x888888);
			}
			snprintf(buffer, SBUF_SIZE-1, "%c %s", bSelected ? '*' : ' ' ,pString);
			MicroProfileDrawText(nX, nY, -1, buffer);
			nY += MICROPROFILE_TEXT_HEIGHT+1;
		}
	}
}


void MicroProfileDraw(uint32_t nWidth, uint32_t nHeight)
{
	MICROPROFILE_SCOPEI("MicroProfile", "Draw", 0x737373);
	if(!S.nDisplay)
		return;
	S.nWidth = nWidth;
	S.nHeight = nHeight;
	S.nHoverToken = MICROPROFILE_INVALID_TOKEN;
	S.nHoverTime = 0;


	if(S.nDisplay & MP_DRAW_DETAILED)
	{
		MicroProfileDrawDetailedView(nWidth, nHeight);
	}
	else if(0 != (S.nDisplay & MP_DRAW_BARS) && S.nBars)
	{
		MicroProfileDrawBarView(nWidth, nHeight);
	}
	MicroProfileDrawMenu(nWidth, nHeight);

	if(S.nHoverToken != MICROPROFILE_INVALID_TOKEN)
	{
		MicroProfileDrawFloatInfo(S.nMouseX, S.nMouseY, S.nHoverToken, S.nHoverTime);

	}

	S.nActiveGroup = S.nMenuAllGroups ? (S.nGroupMask & (uint32_t)-1) : S.nMenuActiveGroup;
	S.nMouseLeft = S.nMouseRight = 0;

}
void MicroProfileMouseMove(uint32_t nX, uint32_t nY)
{
	S.nMouseX = nX;
	S.nMouseY = nY;
}
void MicroProfileClearGraph()
{
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		if(S.Graph[i].nToken != 0)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
		}
	}
}
void MicroProfileToggleFlipDetailed()
{
	S.nFlipLog = !S.nFlipLog;
}

void MicroProfileToggleGraph(MicroProfileToken nToken)
{
	int32_t nMinSort = 0x7fffffff;
	int32_t nFreeIndex = -1;
	int32_t nMinIndex = 0;
	int32_t nMaxSort = 0x80000000;
	for(uint32_t i = 0; i < MICROPROFILE_MAX_GRAPHS; ++i)
	{
		if(S.Graph[i].nToken == MICROPROFILE_INVALID_TOKEN)
			nFreeIndex = i;
		if(S.Graph[i].nToken == nToken)
		{
			S.Graph[i].nToken = MICROPROFILE_INVALID_TOKEN;
			return;
		}
		if(S.Graph[i].nKey < nMinSort)
		{
			nMinSort = S.Graph[i].nKey;
			nMinIndex = i;
		}
		if(S.Graph[i].nKey > nMaxSort)
		{
			nMaxSort = S.Graph[i].nKey;
		}
	}
	int nIndex = nFreeIndex > -1 ? nFreeIndex : nMinIndex;
	S.Graph[nIndex].nToken = nToken;
	S.Graph[nIndex].nKey = nMaxSort+1;
	memset(&S.Graph[nIndex].fHistory[0], 0, sizeof(S.Graph[nIndex].fHistory));
}
void MicroProfileMouseClick(uint32_t nLeft, uint32_t nRight)
{
	S.nMouseLeft = nLeft;
	S.nMouseRight = nRight;
	return;
	if(nLeft)
	{
		uint32_t nBaseHeight = 2*(S.nBarHeight + 1);
		int nIndex = (S.nMouseY - nBaseHeight) / (S.nBarHeight+1);
		if(nIndex < S.GroupInfo[S.nActiveGroup].nNumTimers)
		{
			MicroProfileToken nSelected = MICROPROFILE_INVALID_TOKEN;
			uint32_t nCount = 0;
			for(uint32_t i = 0; i < S.nTotalTimers;++i)
			{
				if(0 == i || 0 != (MicroProfileGetGroupMask(S.TimerInfo[i].nToken) & S.nActiveGroup))
				{
					if(nCount == nIndex)
					{
						nSelected = S.TimerInfo[i].nToken;
					}
					nCount++;
				}
			}
			if(nSelected != MICROPROFILE_INVALID_TOKEN)
				MicroProfileToggleGraph(nSelected);
		}
	}
}

void* g_pFUUU = 0;


void MicroProfileMoveGraph(int nZoom, int nPan)
{
	if(nZoom)
	{
		float fBasePos = S.fGraphBaseTimePos;
		float fBaseTime = S.fGraphBaseTime;
		float fMousePrc = MicroProfileMax((S.nMouseX - 10.f) / S.nWidth ,0.f);
		float fMouseTimeCenter = fMousePrc * fBaseTime + fBasePos;
		if(nZoom > 0)
		{
			S.fGraphBaseTime *= 1.05f;
			S.fGraphBaseTime = MicroProfileMin(S.fGraphBaseTime, (float)MICROPROFILE_MAX_GRAPH_TIME);
		}
		else
			S.fGraphBaseTime /= 1.05f;

		S.fGraphBaseTimePos = fMouseTimeCenter - fMousePrc * S.fGraphBaseTime;
		S.fGraphBaseTimePos = MicroProfileMax(0.f, S.fGraphBaseTimePos);
	}
	if(nPan)
	{
		S.fGraphBaseTimePos += nPan * 2 * S.fGraphBaseTime / S.nWidth;
		S.fGraphBaseTimePos = MicroProfileMax(S.fGraphBaseTimePos, 0.f);
		S.fGraphBaseTimePos = MicroProfileMin(S.fGraphBaseTimePos, MICROPROFILE_MAX_GRAPH_TIME - S.fGraphBaseTime);
	}
}


#endif
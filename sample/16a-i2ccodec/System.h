#ifndef _System_h
#define _System_h

#include <circle/types.h>
#include <circle/logger.h>
#include <circle/screen.h>


// Logger
#define LogScreenMsg(Msg, MsgLength) do { if (g_ScreenPtr) g_ScreenPtr->Write(Msg, MsgLength); } while(0)
#define LogScreen(Msg) do { if (g_ScreenPtr) { CString msg; msg.Format(Msg); g_ScreenPtr->Write(msg, msg.GetLength());} } while(0)
#define LogSerial(Source, Severity, Msg) do { if (g_LoggerPtr) g_LoggerPtr->Write(Source, Severity, Msg); } while(0)

extern CLogger*       g_LoggerPtr;
extern CScreenDevice* g_ScreenPtr;

// Interrupt System
extern CInterruptSystem* g_interruptSysPtr;

// Timer
extern CTimer* g_timerPtr;
#define delay(x) do { if (g_timerPtr) g_timerPtr->MsDelay(x);} while(0)

#endif
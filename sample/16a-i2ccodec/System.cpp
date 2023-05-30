#include "System.h"

// Logging
CLogger*       g_LoggerPtr = nullptr;
CScreenDevice* g_ScreenPtr = nullptr;

// Peripherals
CInterruptSystem* g_interruptSysPtr = nullptr;
CTimer*           g_timerPtr        = nullptr;


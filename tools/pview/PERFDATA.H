
/******************************************************************************\
*       This is a part of the Microsoft Source Code Samples. 
*       Copyright (C) 1993-1997 Microsoft Corporation.
*       All rights reserved. 
*       This source code is only intended as a supplement to 
*       Microsoft Development Tools and/or WinHelp documentation.
*       See these sources for detailed information regarding the 
*       Microsoft samples programs.
\******************************************************************************/


/******************************************************************************

                    P E R F O R M A N C E   D A T A

    Name:       perfdata.h

    Description:
        This module contains function prototypes and defines used in
        objdata.c, instdata.c, and cntrdata.c.

******************************************************************************/






typedef PERF_DATA_BLOCK             PERF_DATA,      *PPERF_DATA;
typedef PERF_OBJECT_TYPE            PERF_OBJECT,    *PPERF_OBJECT;
typedef PERF_INSTANCE_DEFINITION    PERF_INSTANCE,  *PPERF_INSTANCE;
typedef PERF_COUNTER_DEFINITION     PERF_COUNTER,   *PPERF_COUNTER;



DWORD   GetPerfData (HKEY       hPerfKey,
                     LPTSTR     szObjectIndex,
                     PPERF_DATA *ppData,
                     DWORD      *pDataSize);


DWORD   GetPerfTitleSz
                    (HKEY       hKeyMachine,
                     HKEY       hKeyPerf,
                     LPTSTR     *TitleBuffer,
                     LPTSTR     *TitleSz[],
                     DWORD      *TitleLastIdx);



PPERF_OBJECT    FirstObject (PPERF_DATA pData);
PPERF_OBJECT    NextObject (PPERF_OBJECT pObject);
PPERF_OBJECT    FindObject (PPERF_DATA pData, DWORD TitleIndex);
PPERF_OBJECT    FindObjectN (PPERF_DATA pData, DWORD N);

PPERF_INSTANCE  FirstInstance (PPERF_OBJECT pObject);
PPERF_INSTANCE  NextInstance (PPERF_INSTANCE pInst);
PPERF_INSTANCE  FindInstanceN (PPERF_OBJECT pObject, DWORD N);
PPERF_INSTANCE  FindInstanceParent (PPERF_INSTANCE pInst, PPERF_DATA pData);
LPTSTR          InstanceName (PPERF_INSTANCE pInst);

PPERF_COUNTER   FirstCounter (PPERF_OBJECT pObject);
PPERF_COUNTER   NextCounter (PPERF_COUNTER pCounter);
PPERF_COUNTER   FindCounter (PPERF_OBJECT pObject, DWORD TitleIndex);
PVOID           CounterData (PPERF_INSTANCE pInst, PPERF_COUNTER pCount);



#define PN_PROCESS                          TEXT("Process")
#define PN_PROCESS_CPU                      TEXT("% Processor Time")
#define PN_PROCESS_PRIV                     TEXT("% Privileged Time")
#define PN_PROCESS_USER                     TEXT("% User Time")
#define PN_PROCESS_WORKING_SET              TEXT("Working Set")
#define PN_PROCESS_PEAK_WS                  TEXT("Working Set Peak")
#define PN_PROCESS_PRIO                     TEXT("Priority Base")
#define PN_PROCESS_ELAPSE                   TEXT("Elapsed Time")
#define PN_PROCESS_ID                       TEXT("ID Process")
#define PN_PROCESS_PRIVATE_PAGE             TEXT("Private Bytes")
#define PN_PROCESS_VIRTUAL_SIZE             TEXT("Virtual Bytes")
#define PN_PROCESS_PEAK_VS                  TEXT("Virtual Bytes Peak")
#define PN_PROCESS_FAULT_COUNT              TEXT("Page Faults/sec")


#define PN_THREAD                           TEXT("Thread")
#define PN_THREAD_ID						TEXT("ID Thread")
#define PN_THREAD_CPU                       TEXT("% Processor Time")
#define PN_THREAD_PRIV                      TEXT("% Privileged Time")
#define PN_THREAD_USER                      TEXT("% User Time")
#define PN_THREAD_START                     TEXT("Start Address")
#define PN_THREAD_SWITCHES                  TEXT("Context Switches/sec")
#define PN_THREAD_PRIO                      TEXT("Priority Current")
#define PN_THREAD_BASE_PRIO                 TEXT("Priority Base")
#define PN_THREAD_ELAPSE                    TEXT("Elapsed Time")

#define PN_THREAD_DETAILS                   TEXT("Thread Details")
#define PN_THREAD_PC                        TEXT("User PC")

#define PN_IMAGE                            TEXT("Image")
#define PN_IMAGE_NOACCESS                   TEXT("No Access")
#define PN_IMAGE_READONLY                   TEXT("Read Only")
#define PN_IMAGE_READWRITE                  TEXT("Read/Write")
#define PN_IMAGE_WRITECOPY                  TEXT("Write Copy")
#define PN_IMAGE_EXECUTABLE                 TEXT("Executable")
#define PN_IMAGE_EXE_READONLY               TEXT("Exec Read Only")
#define PN_IMAGE_EXE_READWRITE              TEXT("Exec Read/Write")
#define PN_IMAGE_EXE_WRITECOPY              TEXT("Exec Write Copy")


#define PN_PROCESS_ADDRESS_SPACE            TEXT("Process Address Space")
#define PN_PROCESS_PRIVATE_NOACCESS         TEXT("Reserved Space No Access")
#define PN_PROCESS_PRIVATE_READONLY         TEXT("Reserved Space Read Only")
#define PN_PROCESS_PRIVATE_READWRITE        TEXT("Reserved Space Read/Write")
#define PN_PROCESS_PRIVATE_WRITECOPY        TEXT("Reserved Space Write Copy")
#define PN_PROCESS_PRIVATE_EXECUTABLE       TEXT("Reserved Space Executable")
#define PN_PROCESS_PRIVATE_EXE_READONLY     TEXT("Reserved Space Exec Read Only")
#define PN_PROCESS_PRIVATE_EXE_READWRITE    TEXT("Reserved Space Exec Read/Write")
#define PN_PROCESS_PRIVATE_EXE_WRITECOPY    TEXT("Reserved Space Exec Write Copy")


#define PN_PROCESS_MAPPED_NOACCESS          TEXT("Mapped Space No Access")
#define PN_PROCESS_MAPPED_READONLY          TEXT("Mapped Space Read Only")
#define PN_PROCESS_MAPPED_READWRITE         TEXT("Mapped Space Read/Write")
#define PN_PROCESS_MAPPED_WRITECOPY         TEXT("Mapped Space Write Copy")
#define PN_PROCESS_MAPPED_EXECUTABLE        TEXT("Mapped Space Executable")
#define PN_PROCESS_MAPPED_EXE_READONLY      TEXT("Mapped Space Exec Read Only")
#define PN_PROCESS_MAPPED_EXE_READWRITE     TEXT("Mapped Space Exec Read/Write")
#define PN_PROCESS_MAPPED_EXE_WRITECOPY     TEXT("Mapped Space Exec Write Copy")


#define PN_PROCESS_IMAGE_NOACCESS           TEXT("Image Space No Access")
#define PN_PROCESS_IMAGE_READONLY           TEXT("Image Space Read Only")
#define PN_PROCESS_IMAGE_READWRITE          TEXT("Image Space Read/Write")
#define PN_PROCESS_IMAGE_WRITECOPY          TEXT("Image Space Write Copy")
#define PN_PROCESS_IMAGE_EXECUTABLE         TEXT("Image Space Executable")
#define PN_PROCESS_IMAGE_EXE_READONLY       TEXT("Image Space Exec Read Only")
#define PN_PROCESS_IMAGE_EXE_READWRITE      TEXT("Image Space Exec Read/Write")
#define PN_PROCESS_IMAGE_EXE_WRITECOPY      TEXT("Image Space Exec Write Copy")




#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))




typedef     struct _TIME_FIELD
    TIME_FIELD,
    *PTIME_FIELD;

struct _TIME_FIELD
{
    INT     Hours;
    INT     Mins;
    INT     Secs;
    INT     mSecs;
};




// pview.cpp : Defines the entry point for the console application.
//

#include <windows.h>
#include <winperf.h>
#include <stdio.h>
#include "pview.h"
#include "perfdata.h"

using namespace std;



/****
Globals
****/

static DWORD   PX_PROCESS;
static DWORD   PX_PROCESS_CPU;
static DWORD   PX_PROCESS_PRIV;
static DWORD   PX_PROCESS_USER;
static DWORD   PX_PROCESS_ID;

static DWORD   PX_THREAD;
static DWORD   PX_THREAD_ID;
static DWORD   PX_THREAD_CPU;
static DWORD   PX_THREAD_PRIV;
static DWORD   PX_THREAD_USER;
static DWORD   PX_THREAD_START;

#define INDEX_STR_LEN       10

static TCHAR           INDEX_PROCTHRD_OBJ[2*INDEX_STR_LEN];

static DWORD           gPerfDataSize = 50*1024;            // start with 50K
static PPERF_DATA      gpPerfData;

static PPERF_OBJECT    gpProcessObject;                    // pointer to process objects
static PPERF_OBJECT    gpThreadObject;                     // pointer to thread objects

static HKEY            ghPerfKey = HKEY_PERFORMANCE_DATA;  // get perf data from this key
static HKEY            ghMachineKey = HKEY_LOCAL_MACHINE;  // get title index from this key


typedef std::map< DWORD, std::string >		thread_name_map_type;
typedef thread_name_map_type::iterator		thread_name_iterator_type;
typedef pair< DWORD, std::string >			thread_name_pair_type;

static thread_name_map_type thread_name_map;





void registerThreadName( DWORD thread_id, std::string thread_name )
{
	thread_name_map.erase( thread_id );
	thread_name_map.insert( thread_name_pair_type( thread_id, thread_name ) );
}



void FormatTimeFields( double fTime, PTIME_FIELD pTimeFld )
{
	INT     i;
	double   f;

    f = fTime/3600;

    pTimeFld->Hours = i = (int)f;

    f = f - i;
    pTimeFld->Mins = i = (int)(f = f * 60);

    f = f - i;
    pTimeFld->Secs = i = (int)(f = f * 60);

    f = f - i;
    pTimeFld->mSecs = (int)(f * 1000);
}



void FormatThreadInfo ( ostream& strm,
					    PPERF_INSTANCE  pInst,
                        PPERF_COUNTER   pCPU,
                        PPERF_COUNTER   pPRIV,
						PPERF_COUNTER	pID,
                        double          fTime,
                        LPTSTR          str)
{
	LARGE_INTEGER   *liCPU = 0;
	LARGE_INTEGER   *liPRIV = 0;
	DWORD			*pdwID = 0;
	double          fCPU = 0;
	double          fPRIV = 0;
	INT             PcntPRIV = 0;
	INT             PcntUSER = 0;
	TIME_FIELD      TimeFld;
	TCHAR           szTemp[100];
	string			thread_name;


    if (pCPU)
    {
        liCPU = (LARGE_INTEGER *) CounterData (pInst, pCPU);
        fCPU  = Li2Double (*liCPU);
    }

    if (pPRIV)
    {
        liPRIV = (LARGE_INTEGER *) CounterData (pInst, pPRIV);
        fPRIV  = Li2Double (*liPRIV);
    }

    if (fCPU > 0)
    {
        PcntPRIV = (INT)(fPRIV / fCPU * 100 + 0.5);
        PcntUSER = 100 - PcntPRIV;
    }


    if (pID)
    {
        pdwID = (DWORD *) CounterData( pInst, pID );

		// try and get name associated with this thread ID from our map structure
		thread_name_iterator_type i = thread_name_map.find( *pdwID );
		if ( i != thread_name_map.end() )
			thread_name = (*i).second;

        wsprintf (szTemp, TEXT("%ls (%#x) "), InstanceName(pInst), *pdwID );
    }
    else
        wsprintf (szTemp, TEXT("%ls"), InstanceName(pInst));



    FormatTimeFields (fCPU/1.0e7, &TimeFld);

    wsprintf (str,
              TEXT( "Thread %-15.15s\t"
					"processor time: %3ld:%02ld:%02ld.%03ld   \t"
					"user mode: %3ld%%    "
					"privileged mode: %3ld%%  \t%s%s%s\n" ),
              szTemp,
              TimeFld.Hours,
              TimeFld.Mins,
              TimeFld.Secs,
              TimeFld.mSecs,
              PcntUSER,
              PcntPRIV,
			  ( thread_name.length() ? "[" : "" ),
			  thread_name.c_str(),
			  ( thread_name.length() ? "]" : "" ) );

	strm << str;
}



void ReadThreadInfo (ostream& strm,
					 PPERF_OBJECT    pObject,
                     DWORD           ParentIndex)
{
	PPERF_INSTANCE  pInstance;
	TCHAR           szListText[256];

	PPERF_COUNTER   pCounterCPU;
	PPERF_COUNTER   pCounterPRIV;
	PPERF_COUNTER	pCounterID;
	double          fObjectFreq;
	double          fObjectTime;
	double          fTime;

	INT             InstanceIndex = 0;


    if (pObject)
    {
        if ((pCounterCPU  = FindCounter (pObject, PX_THREAD_CPU)) &&
            (pCounterPRIV = FindCounter (pObject, PX_THREAD_PRIV)) &&
			(pCounterID	  = FindCounter (pObject, PX_THREAD_ID)))
        {

            fObjectFreq = Li2Double (pObject->PerfFreq);
            fObjectTime = Li2Double (pObject->PerfTime);
            fTime = fObjectTime / fObjectFreq;


            pInstance = FirstInstance (pObject);

            while (pInstance && InstanceIndex < pObject->NumInstances)
            {
                if (ParentIndex == pInstance->ParentObjectInstance)
                {
                    FormatThreadInfo ( strm,
									   pInstance,
                                       pCounterCPU,
                                       pCounterPRIV,
									   pCounterID,
                                       fTime,
                                       szListText);
                }

                pInstance = NextInstance (pInstance);
                InstanceIndex++;
            }
        }
    }

}



DWORD   GetTitleIdx (LPTSTR Title[], DWORD LastIndex, LPTSTR Name)
{
	DWORD   Index;

    for (Index = 0; Index <= LastIndex; Index++)
        if (Title[Index])
            if (!lstrcmpi (Title[Index], Name))
                return Index;

    return 0;
}



void SetPerfIndexes()
{
	LPTSTR  TitleBuffer;
	LPTSTR  *Title;
	DWORD   Last;
	DWORD   dwR;


    dwR = GetPerfTitleSz (ghMachineKey, ghPerfKey, &TitleBuffer, &Title, &Last);

    if (dwR != ERROR_SUCCESS)
        {
        printf (TEXT("Unable to retrieve counter indexes, ERROR -> %#x"), dwR);
        return;
        }


    PX_PROCESS                       = GetTitleIdx (  Title, Last, PN_PROCESS);
    PX_PROCESS_CPU                   = GetTitleIdx (  Title, Last, PN_PROCESS_CPU);
    PX_PROCESS_PRIV                  = GetTitleIdx (  Title, Last, PN_PROCESS_PRIV);
    PX_PROCESS_USER                  = GetTitleIdx (  Title, Last, PN_PROCESS_USER);
    PX_PROCESS_ID                    = GetTitleIdx (  Title, Last, PN_PROCESS_ID);

    PX_THREAD                        = GetTitleIdx (  Title, Last, PN_THREAD);
	PX_THREAD_ID					 = GetTitleIdx (  Title, Last, PN_THREAD_ID );
    PX_THREAD_CPU                    = GetTitleIdx (  Title, Last, PN_THREAD_CPU);
    PX_THREAD_PRIV                   = GetTitleIdx (  Title, Last, PN_THREAD_PRIV);
    PX_THREAD_USER                   = GetTitleIdx (  Title, Last, PN_THREAD_USER);
    PX_THREAD_START                  = GetTitleIdx (  Title, Last, PN_THREAD_START);


    wsprintf (INDEX_PROCTHRD_OBJ, TEXT("%ld %ld"), PX_PROCESS, PX_THREAD);


    LocalFree (TitleBuffer);
    LocalFree (Title);

}



PPERF_DATA ReadPerfData (HKEY        hPerfKey,
                           LPTSTR      szObjectIndex,
                           PPERF_DATA  pData,
                           DWORD       *pDataSize)
{
    if (GetPerfData (hPerfKey, szObjectIndex, &pData, pDataSize) == ERROR_SUCCESS)
        return pData;
    else
        return NULL;
}



void FormatProcessInfo ( ostream& strm,
						 PPERF_INSTANCE pInst,
                         PPERF_COUNTER  pCPU,
                         PPERF_COUNTER  pPRIV,
                         PPERF_COUNTER  pProcID,
                         double         fTime,
                         LPTSTR         str)
{
	DWORD           *pdwProcID;
	LARGE_INTEGER   *liCPU;
	LARGE_INTEGER   *liPRIV;
	double          fCPU = 0;
	double          fPRIV = 0;
	INT             PcntPRIV = 0;
	INT             PcntUSER = 0;
	TIME_FIELD      TimeFld;
	TCHAR           szTemp[100];


    if (pCPU)
    {
        liCPU = (LARGE_INTEGER *) CounterData (pInst, pCPU);
        fCPU  = Li2Double (*liCPU);
    }

    if (pPRIV)
    {
        liPRIV = (LARGE_INTEGER *) CounterData (pInst, pPRIV);
        fPRIV  = Li2Double (*liPRIV);
    }

    if (fCPU > 0)
    {
        PcntPRIV = (INT)(fPRIV / fCPU * 100 + 0.5);
        PcntUSER = 100 - PcntPRIV;
    }



    if (pProcID)
    {
        pdwProcID = (DWORD *) CounterData (pInst, pProcID);
        wsprintf (szTemp, TEXT("%ls (%#x)"), InstanceName(pInst), *pdwProcID);
    }
    else
        wsprintf (szTemp, TEXT("%ls"), InstanceName(pInst));


    FormatTimeFields (fCPU/1.0e7, &TimeFld);

    wsprintf( str,
              TEXT( "Process: %-35.35s "
					"processor time: %3ld:%02ld:%02ld.%03ld    "
					"user mode: %3ld%%    "
					"privileged mode: %3ld%%\n" ), 
              szTemp,
              TimeFld.Hours,
              TimeFld.Mins,
              TimeFld.Secs,
              TimeFld.mSecs,
              PcntUSER,
              PcntPRIV);

	strm << str;
}



void showProcessThreadInfo( ostream& strm, const char* app_name )
{
	SetPerfIndexes();


    // get performance data
    gpPerfData = ReadPerfData (ghPerfKey, INDEX_PROCTHRD_OBJ, gpPerfData, &gPerfDataSize);

    gpProcessObject = FindObject (gpPerfData, PX_PROCESS);
    gpThreadObject  = FindObject (gpPerfData, PX_THREAD);


	// get process info
	PPERF_INSTANCE  pInstance;
	TCHAR           szListText[256];

	PPERF_COUNTER   pCounterCPU;
	PPERF_COUNTER   pCounterPRIV;
	PPERF_COUNTER   pCounterProcID;
	double          fObjectFreq;
	double          fObjectTime;
	double          fTime;

	INT             InstanceIndex = 0;


	if (gpProcessObject)
	{
		if ((pCounterCPU    = FindCounter (gpProcessObject, PX_PROCESS_CPU))  &&
			(pCounterPRIV   = FindCounter (gpProcessObject, PX_PROCESS_PRIV)) &&
			(pCounterProcID = FindCounter (gpProcessObject, PX_PROCESS_ID)))
		{
			fObjectFreq = Li2Double (gpProcessObject->PerfFreq);
			fObjectTime = Li2Double (gpProcessObject->PerfTime);
			fTime = fObjectTime / fObjectFreq;

			pInstance = FirstInstance (gpProcessObject);

			while (pInstance && InstanceIndex < gpProcessObject->NumInstances)
			{
				char temp[ 1000 ];
				wsprintf ( temp, TEXT("%ls"), InstanceName(pInstance));
				if ( strcmp( temp, app_name ) == 0 )
				{
					FormatProcessInfo ( strm, 
										pInstance,
										pCounterCPU,
										pCounterPRIV,
										pCounterProcID,
										fTime,
										szListText);
					strm << endl;
				
					PPERF_INSTANCE  pProcessInstance;

					// get process info
					pProcessInstance = FindInstanceN (gpProcessObject, InstanceIndex);


					// get thread info
					ReadThreadInfo( strm, gpThreadObject, InstanceIndex );
					strm << endl;

				}
				pInstance = NextInstance (pInstance);
				InstanceIndex++;
			}
		}
	}
}



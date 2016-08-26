
#ifndef FRAMEWORK_H
#define FRAMEWORK_H


#include "RunnableObject.h"			// provides OS-specific thread control and synchronisation

#include "ProcessorNode.h"			// the basic processing node class for the pipeline

#include "ReadJpegSequence.h"		// node: JPEG sequence reader
#include "WriteBmpSequence.h"		// node: BMP sequence writer
#include "Colour2GrayNode.h"		// node: converts a colour image to grayscale
#include "Gray2ColourNode.h"		// node: converts a grayscale image to a colour format image
#include "SyncNode.h"				// node: synchronises the output with a timer
#include "CorrectAspectRatio.h"		// node: corrects for the non-square pixel aspect ratio of PAL/NTSC video cameras
#include "ViewerNode.h"				// node: bridge between pipeline and the GUI system
#include "SlideshowViewerNode.h"	// node: variation of ViewerNode

#include "IplData.h"				// IPL-specific data that is passed/processed through the pipeline

#include "MemoryPool.h"				// memory pool manager

#include "MemoryResource.h"			// the memory resource handled by the memory pool manager
#include "IplImageResource.h"		// IPL-specific data memory resource



#endif


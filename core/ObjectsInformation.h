
#ifndef OBJECTS_INFORMATION_H
#define OBJECTS_INFORMATION_H


#include "Blob.h"
#include "Object.h"
#include "..\tools\LWList.h"
#include "..\tools\LWGraph.h"



typedef LWList< Blob >						BlobList;
typedef LWList< Object >					ObjectList;
typedef LWGraph< BlobList, BlobList >		BlobClusterGraph;
typedef LWGraph< ObjectList, ObjectList >	ClusterObjectGraph;



extern ObjectList* objects;




#endif


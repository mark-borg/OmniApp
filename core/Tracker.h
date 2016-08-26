
#ifndef TRACKER_H
#define TRACKER_H


#include "..\framework\ProcessorNode.h"
#include "..\framework\IplData.h"
#include "..\framework\IplImageResource.h"
#include "..\tools\CvUtils.h"
#include "ObjectsInformation.h"



// this file defines some functions common to all tracker nodes 
// (i.e. to BlobTracker and ColourTracker).



void drawObjectBoundingBox( IplImage* out, 
							ObjectList* objects, 
							long frame );


void nearestDistance( IplImage* blob_map, 
					  int label1, int label2,
					  CvRect rect1, CvRect rect2,
  					  int max_distance,
					  double& nearest_distance,
					  CvPoint& nearest_point );


void labelBlobs( IplImage* lbl_map, 
			     int unlabeled_value,
			     BlobList* blobs,
			     IplImage* mask,
				 int minimum_area =0 );



#endif

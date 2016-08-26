
#ifndef MATCH_SCORES_H
#define MATCH_SCORES_H


#include "Object.h"
#include "..\tools\LWList.h"




class MatchScores
{
public:

	typedef LWList< Object >		ObjectList;


	int max_objects;
	int max_clusters;


	// usage: distance_measure[ cluster_id ][ object_id ]
	double** overlap;			// overlap of bounding boxes: [0..1]
	double** distance;			// distance of centroids: [0..1]
	double** colour;			// colour difference: [0..1]
	double** area;				// area ratio: [0..1]	(1 = equal area)
	
	double** total;



	MatchScores( int max_objects, int max_clusters );


	~MatchScores();


	void sumTotalScore( ObjectList* objects );



	// individual weights for the similarity measures
	static float OVERLAP_WEIGHT;
	static float DISTANCE_WEIGHT;
	static float COLOUR_WEIGHT;
	static float AREA_WEIGHT;

	//and the total weight
	static float TOTAL_WEIGHT;

};





#endif


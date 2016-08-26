
#ifndef OBJECT_H
#define OBJECT_H


#include "..\tools\LWList.h"
#include "BivariateGaussian.h"
#include "MixtureModelMap.h"


// The Object class is used as a model for both the blob-based tracking method
// and also the colour-based tracking method. The program structure can be
// improved by separating the two into separate classes.



class Object
{
public:

	enum {	H_NUM_BINS	= 32 };
	enum {	S_NUM_BINS	= 32 };
	enum {	V_NUM_BINS	= 16 };


	//---- object's state ----
	int				label;
	CvPoint2D32f	centre;
	CvRect			bounding_box;
	float			area;

	CvPoint2D32f	first_pos;
	float			max_area;
	CvRect			old_bounding_box;
	CvPoint2D32f	old_centre;


	// colour histogram
	double			h_bins[ H_NUM_BINS ];
	double			s_bins[ S_NUM_BINS ];
	double			v_bins[ V_NUM_BINS ];
	bool			histogram_empty;

	
	// GMM
	enum{ GMM_COMPONENTS = 3 };

	typedef BivariateGaussian< CvPoint2D32f >	BivariateGaussian;
	typedef MixtureModelMap< CvPoint2D32f, BivariateGaussian, GMM_COMPONENTS > GaussianMixtureModel;

	GaussianMixtureModel gmm;
	long gmm_updated;


	// bounds of object
	double			min_radius, max_radius;	// min and max radial distance from centre of mirror centre in image
	double			min_azimuth, max_azimuth;	// azimuth



	//---- visibility state ----
	long			born;			// frame number (time of creation of the object)	
	long			lives;			// a count indicating the number of time object has been successfully tracked
	long			lost_count;		// a count of the number of frames the object has been lost;  
									// this variable is reset when the object is no longer lost
	
	bool			lost;			// true if object has been lost
	bool			merged;			// has merged with another object (possibly including occlusion)
	bool			occluded;		// merged object that has disappeared (overlap is zero)

	bool			was_last_merged;		// used for consistency checking of object merging


	//---- history
	LWList< CvPoint2D32f >*	path;


	Object();


	Object& operator= ( const Object& other_obj );


	void initHistory();


	void clearHistory();


	~Object();

};




#endif


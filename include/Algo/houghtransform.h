#ifndef houghtransform_h
#define houghtransform_h
/*
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Fredman
 Date:		18-12-2002
 RCS:		$Id: houghtransform.h,v 1.1 2003-01-15 11:22:42 niclas Exp $
________________________________________________________________________

*/


#include "thread.h"
#include "arrayndimpl.h"

class Array3DInfo;
template <class T> class Array3D;
class BasicTask;
template <class T> class ObjectSet;
class PlaneFrom3DSpaceHoughTransformTask;
class Plane3;
class Vector3;
namespace Threads { class Mutex; };

/*!\brief
Finds planes in Array3D<float>'s regions with high values. All positions
in the array above a threshold (defined by cliprate) is used to find the planes.

The planes are defined with three parameters: dip (0-90), azi(0-360) and the
normal's distance to origo. Depending on your application, you might need
different resolution, which is set with setParamSpaceSize.

Usage:
1) Create
2) Set setParamSpaceSize, cliprate and data
3) Get the tasks and run them
4) call sortParamSpace
5) get your planes and their scores.

*/

class PlaneFrom3DSpaceHoughTransform
{
public:
    				PlaneFrom3DSpaceHoughTransform();
    virtual			~PlaneFrom3DSpaceHoughTransform();

    void			setParamSpaceSize( int dipsize, int azisize,
	    					   int distsize );
    				/*!< Here the user can set the resolution for
				     the hough space. It also calculates the 
				     normals from every combination of dip and
				     azimuth and stores it in the variable 
				     normals.
				*/
    const Array3DInfo*		paramSpaceSize() const { return datainfo; }

    void			setClipRate( float );
    				/*!< Between 0-1. For instance, cliprate 0.6 
				     will set Data with the 60% highest 
				     values of indata.
				*/
				     
    float			clipRate() const;

    void			setData( const Array3D<float>* );
    ObjectSet<BasicTask>*	createCalculators();
    				/*!< Creates a number of BasicTasks dep
				     on the no of processors on the machine.
    				     Result is managed by caller.
				*/
    void			sortParamSpace(int nrplanes );
    				/*!< Sorts the nrplanes best scores from
				     paramspace. Writes the result in variable
				     houghscores. It also saves the positions 
				     in the variable houghpositions.  
				*/

    Plane3			getPlane( int nrplane ) const;
    				/*!< Returns the nrplane plane from 
				     houghpositions converted to the 
				     ( x,y,z ) space.
				*/ 
    unsigned int		getHoughScore( int ) const;
    int				getNrPointsAfterClip() const; 
    				/*!< Returns the number of datapoints left 
				     after the function setClipRate() has 
				     been run.
				*/
			     

protected:
    void			incParamPos( int, int, int );
    float			cliprate;
    TypeSet<unsigned int>	calcpositions;
    Array3DInfo*		datainfo;

    Array3D<unsigned int>*	paramspace;
    float			deltadist;
    Vector3*			normals;
    friend			class ::PlaneFrom3DSpaceHoughTransformTask;

    Threads::Mutex&		paramspacemutex;
    TypeSet<unsigned int>	houghscores;
    TypeSet<unsigned int>	houghpositions;
};

#endif

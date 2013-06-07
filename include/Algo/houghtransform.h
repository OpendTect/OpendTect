#ifndef houghtransform_h
#define houghtransform_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Fredman
 Date:		18-12-2002
 RCS:		$Id$
________________________________________________________________________

*/


#include "thread.h"
#include "arrayndimpl.h"
#include "ranges.h"

class Array3DInfo;
template <class T> class Array3D;
class SequentialTask;
template <class T> class ObjectSet;
template <class T, class B> class TopList;
class PlaneFrom3DSpaceHoughTransformTask;
class Plane3;
class Coord3;
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

mClass PlaneFrom3DSpaceHoughTransform
{
public:
    				PlaneFrom3DSpaceHoughTransform();
    virtual			~PlaneFrom3DSpaceHoughTransform();

    void			setResolution( double dangle,
					       int distsize );
    				/*!< \param dangle is the angle between the
					    planes that are tested.
				     \param distsize is the number of bins in
				     	    the distance domain.
				*/
    int				getParamSpaceSize() const;
    int				getNrDistVals() const;

    void			setClipRate( float );
    				/*!< Between 0-1. For instance, cliprate 0.6 
				     will set Data with the 60% highest 
				     values of indata. Default is 0.7
				*/
				     
    float			clipRate() const;

    void			setData( const Array3D<float>* );
    ObjectSet<SequentialTask>*	createCalculators();

    TopList<unsigned int, unsigned int>*
				sortParamSpace(int) const;
    				/*!< Sorts the paramspace and returns
				     an array with the indexes of the planes,
				     from less likely planes to likely planes.
				     The best plane is thus the
				     res[getParamSpaceSize()-1] value.
				     Result is managed by caller and
				     should be deleted with [];
				*/
    Plane3			getPlane( int plane ) const;
    				/*!< Returns the plane plane from 
				     houghpositions converted to the 
				     ( x,y,z ) space.
				*/ 
    int				getNrPointsAfterClip() const; 
    				/*!< Returns the number of datapoints left 
				     after the function setClipRate() has 
				     been run.
				*/
			     

protected:
    void			incParamPos( int normal, double dist );
    float			cliprate_;
    TypeSet<unsigned int>	calcpositions_;
    Array3DInfo*		datainfo_;

    Array2D<unsigned int>*	paramspace_;
    double			deltadist_;
    TypeSet<Coord3>*		normals_;
    friend			class ::PlaneFrom3DSpaceHoughTransformTask;

    Threads::Mutex&		paramspacemutex_;
};


mClass LineFrom2DSpaceHoughTransform
{
public:
    				LineFrom2DSpaceHoughTransform(
					const Array2D<float>&);
    				~LineFrom2DSpaceHoughTransform();

    bool			compute();

    void			setThreshold(float val,bool above_val);
    				/*<Make hough array based on above/below val.*/
    void			setLineAngleRange(Interval<float>rg);
    				/*<rg is between 0-PI in radian.*/
    void			setTopList(int nr)	{ toplistnr_ = nr; }
    				/*<nr is the number of top confident lines.*/
    Array2D<unsigned char>*	getResult() const	{ return result_; }
    				/*<0/1 array, 1 means on a line. */

//protected:

    bool			setLineFlag(float radius,float theta);

    const Array2D<float>&	input_;
    Array2D<int>*		origcnt_;	
    Array2D<int>*		hougharr_;
    Array2D<unsigned char>*	result_;

    float			threshold_;
    bool			abovethreshold_;
    int				toplistnr_; 
    Interval<float>		anglerg_;
};


#endif

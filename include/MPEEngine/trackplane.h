#ifndef trackplane_h
#define trackplane_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: trackplane.h,v 1.1 2005-01-06 09:25:55 kristofer Exp $
________________________________________________________________________


-*/

#include "cubesampling.h"


template <class T> class MathFunction;

namespace MPE
{

/*!\brief

*/

class TrackPlane
{
public:
    			TrackPlane( const BinID& start,
			       const BinID& stop,
			       float time );

   			TrackPlane( const BinID& start,
			       const BinID& stop,
			       float starttime,
			       float stoptime );
			TrackPlane() {}

    bool		isVertical() const;
    const CubeSampling&	boundingBox() const { return cubesampling; }
    CubeSampling&	boundingBox() { return cubesampling; }

    Coord3		normal(const MathFunction<float>* t2d=0) const;
    float		distance(const Coord3&,
	    			 const MathFunction<float>* t2d=0) const;
    			/*!<\note does not check the plane's boundaries */
    const BinIDValue&	motion() const { return motion_; }
    void		setMotion( int inl, int crl, float z );

protected:

    CubeSampling	cubesampling;
    BinIDValue		motion_;
};

}; // Namespace

#endif

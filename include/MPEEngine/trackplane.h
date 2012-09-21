#ifndef trackplane_h
#define trackplane_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "mpeenginemod.h"
#include "cubesampling.h"
#include "mathfunc.h"
#include "enums.h"

class Plane3;


namespace MPE
{

/*!\brief

*/

mClass(MPEEngine) TrackPlane
{
public:
    enum TrackMode	{ None, Extend, ReTrack, Erase, Move };
    			DeclareEnumUtils(TrackMode);

    			TrackPlane( const BinID& start,
			       const BinID& stop,
			       float time );

   			TrackPlane( const BinID& start,
			       const BinID& stop,
			       float starttime,
			       float stoptime );
			TrackPlane() : trackmode(None) {}

    bool		isVertical() const;
    const CubeSampling&	boundingBox() const { return cubesampling; }
    CubeSampling&	boundingBox() { return cubesampling; }

    Coord3		normal(const FloatMathFunction* t2d=0) const;
    float		distance(const Coord3&,
	    			 const FloatMathFunction* t2d=0) const;
    			/*!<\note does not check the plane's boundaries */
    const BinIDValue&	motion() const { return motion_; }
    void		setMotion( int inl, int crl, float z );

    void		computePlane(Plane3&) const;

    void		setTrackMode(TrackMode tm)	{ trackmode = tm; }
    TrackMode		getTrackMode() const		{ return trackmode; }

    void		fillPar(IOPar&) const;
    bool		usePar( const IOPar& );

protected:

    static const char*	sKeyTrackMode()	{ return "Track Mode"; }

    CubeSampling	cubesampling;
    BinIDValue		motion_;

    TrackMode		trackmode;
};

}; // Namespace

#endif


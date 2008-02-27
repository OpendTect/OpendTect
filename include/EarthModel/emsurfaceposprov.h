#ifndef emsurfaceposprov_h
#define emsurfaceposprov_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: emsurfaceposprov.h,v 1.2 2008-02-27 13:42:08 cvsbert Exp $
________________________________________________________________________


-*/

#include "posprovider.h"
#include "horsampling.h"
#include "multiid.h"
namespace EM { class Surface; }

namespace Pos
{

/*!\brief Provider based on surface(s)
 
  For one horizon, the provider iterates trhough the horizon. For two
  horizons, the points between the horizons are visited with the
  specified Z step.
 
 */

class EMSurfaceProvider3D : public Provider3D
{
public:

			EMSurfaceProvider3D();
			EMSurfaceProvider3D(const EMSurfaceProvider3D&);
			~EMSurfaceProvider3D();
    EMSurfaceProvider3D& operator =(const EMSurfaceProvider3D&);
    const char*		type() const;	//!< sKey::Polygon
    Provider*		clone() const
    			{ return new EMSurfaceProvider3D(*this); }

    virtual Executor*	initializer();
    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const;
    virtual float	curZ() const;
    virtual bool	includes(const BinID&,float) const;
    virtual bool	hasZAdjustment() const;
    virtual float	adjustedZ(const Coord&,float) const;

    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent(BinID&,BinID&) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual int		estNrPos() const;
    virtual int		estNrZPerPos() const;

    int			nrSurfaces() const;
    MultiID		surfaceID( int idx ) const { return idx ? id2_ : id1_; }
    EM::Surface*	surface( int idx )
    			{ return idx ? surf2_ : surf1_; }
    const EM::Surface*	surface( int idx ) const
    			{ return idx ? surf2_ : surf1_; }
    float		zStep() const		{ return zstep_; }
    void		setZStep( float s )	{ zstep_ = s; }

    static const char*	id1Key();
    static const char*	id2Key();
    static const char*	zstepKey();

protected:

    EM::Surface*	surf1_;
    EM::Surface*	surf2_;
    MultiID		id1_;
    MultiID		id2_;
    float		zstep_;
    HorSampling		hs_;
    Interval<float>	zrg1_;
    Interval<float>	zrg2_;


public:

    static void		initClass();
    static Provider3D*	create()	{ return new EMSurfaceProvider3D; }

};


} // namespace

#endif

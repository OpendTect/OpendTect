#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id$
________________________________________________________________________


-*/

#include "geometrymod.h"
#include "posprovider.h"
#include "multiid.h"
class TrcKeySampling;
template <class T> class ODPolygon;

namespace Pos
{

/*!\brief Volume/Area provider based on Polygon */

mExpClass(Geometry) PolyProvider3D : public Provider3D
{
public:

			PolyProvider3D();
			PolyProvider3D(const PolyProvider3D&);
			~PolyProvider3D();
    PolyProvider3D&	operator =(const PolyProvider3D&);
    const char*		type() const;	//!< sKey::Polygon()
    const char*		factoryKeyword() const { return type(); }
    Provider*		clone() const	{ return new PolyProvider3D(*this); }

    virtual bool	initialize(TaskRunner* tr=0);
    virtual void	reset()		{ initialize(); }

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const	{ return curbid_; }
    virtual float	curZ() const		{ return curz_; }
    virtual bool	includes(const BinID&,float) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent(BinID&,BinID&) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const	{ return zrg_.nrSteps()+1; }

    const MultiID&	getMultiID() const	{ return mid_; }
    void		setMultiID(const MultiID& mid)	{ mid_ = mid; }

    ODPolygon<float>&	polygon()		{ return poly_; }
    const ODPolygon<float>& polygon() const	{ return poly_; }
    StepInterval<float>& zRange()		{ return zrg_; }
    const StepInterval<float>& zRange() const	{ return zrg_; }
    TrcKeySampling&	horSampling()		{ return hs_; }
    const TrcKeySampling&	horSampling() const	{ return hs_; }

    static ODPolygon<float>* polyFromPar(const IOPar&,int nr=0);

    virtual bool	includes( const Coord& c, float z ) const
			{ return Provider3D::includes(c,z); }

    void		setUseAreaInside(bool);
    bool		usesAreaInside() const;

    static const char*	sInside()		{ return "Inside"; }
    static const char*	sBoundingBox()		{ return "Bounding Box"; }

protected:

    ODPolygon<float>&	poly_;
    StepInterval<float>	zrg_;
    TrcKeySampling&	hs_;
    MultiID		mid_;

    BinID		curbid_;
    float		curz_;

public:

    static void		initClass();
    static Provider3D*	create()		{ return new PolyProvider3D; }

};


} // namespace


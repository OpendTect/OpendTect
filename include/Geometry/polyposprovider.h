#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
    const char*		type() const override;	//!< sKey::Polygon()
    const char*		factoryKeyword() const override { return type(); }
    Provider*		clone() const override
			    { return new PolyProvider3D(*this); }

    bool		initialize(TaskRunner* tr=0) override;
    void		reset() override		{ initialize(); }

    bool		toNextPos() override;
    bool		toNextZ() override;

    BinID		curBinID() const override	{ return curbid_; }
    float		curZ() const override		{ return curz_; }
    bool		includes(const BinID&,float) const override;
    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    void		getExtent(BinID&,BinID&) const override;
    void		getZRange(Interval<float>&) const override;
    od_int64		estNrPos() const override;
    int			estNrZPerPos() const override
			    { return zrg_.nrSteps()+1; }

    const MultiID&	getMultiID() const	{ return mid_; }
    void		setMultiID(const MultiID& mid)	{ mid_ = mid; }

    ODPolygon<float>&	polygon()		{ return poly_; }
    const ODPolygon<float>& polygon() const	{ return poly_; }
    StepInterval<float>& zRange()		{ return zrg_; }
    const StepInterval<float>& zRange() const	{ return zrg_; }
    TrcKeySampling&	horSampling()		{ return hs_; }
    const TrcKeySampling&	horSampling() const	{ return hs_; }

    static ODPolygon<float>* polyFromPar(const IOPar&,int nr=0);
    static ODPolygon<float>* polyFromPar(const IOPar&,int nr,BufferString* nm);

    bool		includes( const Coord& c, float z ) const override
			{ return Provider3D::includes(c,z); }

    void		setUseAreaInside( bool yn ) { useinside_ = yn; }
    bool		usesAreaInside() const { return useinside_; }

    static const char*	sInside()		{ return "Inside"; }
    static const char*	sBoundingBox()		{ return "Bounding Box"; }

protected:

    ODPolygon<float>&	poly_;
    StepInterval<float> zrg_;
    TrcKeySampling&	hs_;
    MultiID		mid_;
    bool	useinside_ = true;

    BinID		curbid_;
    float		curz_;

public:

    static void		initClass();
    static Provider3D*	create()		{ return new PolyProvider3D; }

};

} // namespace Pos

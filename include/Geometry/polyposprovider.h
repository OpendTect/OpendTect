#ifndef polyposprovider_h
#define polyposprovider_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: polyposprovider.h,v 1.11 2011/04/22 13:28:56 cvsbert Exp $
________________________________________________________________________


-*/

#include "posprovider.h"
class HorSampling;
template <class T> class ODPolygon;

namespace Pos
{

/*!\brief Volume/Area provider based on Polygon */

mClass PolyProvider3D : public Provider3D
{
public:

			PolyProvider3D();
			PolyProvider3D(const PolyProvider3D&);
			~PolyProvider3D();
    PolyProvider3D&	operator =(const PolyProvider3D&);
    const char*		type() const;	//!< sKey::Polygon
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

    ODPolygon<float>&	polygon()		{ return poly_; }
    const ODPolygon<float>& polygon() const	{ return poly_; }
    StepInterval<float>& zRange()		{ return zrg_; }
    const StepInterval<float>& zRange() const	{ return zrg_; }
    HorSampling&	horSampling()		{ return hs_; }
    const HorSampling&	horSampling() const	{ return hs_; }

    static ODPolygon<float>* polyFromPar(const IOPar&,int nr=0);

    virtual bool	includes( const Coord& c, float z ) const
			{ return Provider3D::includes(c,z); }

protected:

    ODPolygon<float>&	poly_;
    StepInterval<float>	zrg_;
    HorSampling&	hs_;

    BinID		curbid_;
    float		curz_;

public:

    static void		initClass();
    static Provider3D*	create()		{ return new PolyProvider3D; }

};


} // namespace

#endif

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "algomod.h"
#include "posfilter.h"
#include "ranges.h"
class TrcKeyZSampling;


namespace Pos
{

/*!
\brief Provides a series of positions; can also be used for subselection.

  toNextPos() will ignore any Z settings and go to the first Z on the next
  position. toNextZ() is the normal 'iterator increment'. After initialization,
  you need to do toNextZ() or toNextPos() for a valid position.
*/

mExpClass(Algo) Provider : public virtual Filter
{
public:

    bool		isProvider() const override;

    virtual bool	toNextPos()				= 0;
    virtual bool	toNextZ()				= 0;
    virtual Coord	curCoord() const			= 0;
    virtual float	curZ() const				= 0;
    virtual TrcKey	curTrcKey() const			= 0;
    virtual OD::GeomSystem survID() const			= 0;

    virtual od_int64	estNrPos() const			= 0;
    virtual int		estNrZPerPos() const			{ return 1; }

    float		estRatio(const Provider&) const override;
    virtual void	getTrcKeyZSampling(TrcKeyZSampling&) const;

    static Provider*	make(const IOPar&,bool is2d);
};


/*!
\brief Provides a subselection for 3D surveys.
*/

mExpClass(Algo) Provider3D : public Filter3D
			   , public Provider
{
public:

    bool		is2D() const override		{ return false; }

    virtual BinID	curBinID() const				= 0;
    TrcKey		curTrcKey() const override { return TrcKey(curBinID());}
    Coord		curCoord() const override;

    bool		includes(const BinID&,
				 float z=mUdf(float)) const override = 0;
    bool		includes(const Coord&,
				 float z=mUdf(float)) const override;

    virtual void	getExtent(BinID& start,BinID& stop) const	= 0;
    virtual void	getZRange(Interval<float>&) const	= 0;
    OD::GeomSystem	survID() const override	{ return gs_; }

    static ::Factory<Provider3D>& factory();
    uiString		factoryDisplayName() const override
			{ return uiString::empty(); }
    const char*		factoryKeyword() const override { return nullptr; }

    static Provider3D*	make(const IOPar&);

protected:
			Provider3D();

private:
    OD::GeomSystem	gs_;

public:

    mDeprecatedObs
    virtual void	setSurvID( OD::GeomSystem gs )	{ gs_ = gs; }

};


/*!
\brief Provides a subselection for 2D surveys - requires the line name(s).
*/

mExpClass(Algo) Provider2D : public Filter2D
			   , public Provider
{
public:

    bool		is2D() const override			{ return true; }

    virtual Pos::GeomID	curGeomID() const			= 0;
    virtual int		curNr() const				= 0;
    bool		includes(int,float z=mUdf(float),
				 int nr=0) const override = 0;
    bool		includes(const Coord&,
				 float z=mUdf(float)) const override = 0;

    virtual void	getExtent(Interval<int>&,int lidx) const = 0;
    virtual void	getZRange(Interval<float>&,int lidx) const = 0;

    OD::GeomSystem	survID() const override;

    static ::Factory<Provider2D>& factory();
    uiString		factoryDisplayName() const override
			{ return uiString::empty(); }
    const char*		factoryKeyword() const override { return nullptr; }
    static Provider2D*	make(const IOPar&);

};

} // namespace Pos

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________


-*/

#include "seismod.h"
#include "posprovider.h"
#include "posinfo.h"

namespace PosInfo { class Line2DData; }

mDeclEmptyTranslatorBundle(General,PosProviders,dgb,"subsel")

namespace Pos
{

/*!\brief 3D provider based on TrcKeyZSampling */

mExpClass(General) SeisCubeProvider3D : public Provider3D
{
public:

			SeisCubeProvider3D();
			SeisCubeProvider3D(const SeisCubeProvider3D&);
			~SeisCubeProvider3D();
    SeisCubeProvider3D&	operator =(const SeisCubeProvider3D&);
    const char*		type() const		{ return "Seismic Cube"; }
    const char*		factoryKeyword() const	{ return type(); }
    virtual Provider*	clone() const	{ return new SeisCubeProvider3D(*this);}

    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const	{ return curbid_; }
    virtual float	curZ() const;
    virtual bool	includes(const BinID&,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(BufferString&) const;

    virtual void	getExtent(BinID& start,BinID& stop) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const;

    const ZSampling&	zSampling() const	{ return zsamp_; }
    void		setZSampling( const ZSampling& zrg )
						{ zsamp_ = zrg; }

    virtual bool	includes( const Coord& c, float z=mUdf(float) ) const
			{ return Pos::Provider3D::includes(c,z); }

protected:

    PosInfo::CubeData	cubedata_;
    ZSampling		zsamp_;
    PosInfo::CubeDataPos curpos_;
    int			curzidx_;

public:

    static void		initClass();
    static Provider3D*	create()	{ return new SeisCubeProvider3D; }

};


} // namespace

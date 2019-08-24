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
#include "cubedata.h"
#include "dbkey.h"
#include "keystrs.h"


namespace Pos
{

/*!\brief 3D provider based on cube of PS data store */

mExpClass(Seis) SeisProvider3D : public Provider3D
{ mODTextTranslationClass(SeisProvider3D)
public:

			SeisProvider3D();
			SeisProvider3D(const SeisProvider3D&);
			~SeisProvider3D();
    SeisProvider3D&	operator =(const SeisProvider3D&);
    const char*		type() const		{ return sKeyType(); }
    const char*		factoryKeyword() const	{ return type(); }
    static const char*	sKeyType()	{ return sKey::SeisCubePositions(); }
    static uiString	dispType()	{ return tr("Seismic Cube Positions");}
    virtual Provider*	clone() const	{ return new SeisProvider3D(*this); }

    uiRetVal		setSeisID(const DBKey&);
    const DBKey&	seisID() const		{ return id_; }

    virtual void	reset();

    virtual bool	toNextPos();
    virtual bool	toNextZ();

    virtual BinID	curBinID() const;
    virtual float	curZ() const;
    virtual bool	includes(const BinID&,float z=mUdf(float)) const;
    virtual void	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual void	getSummary(uiString&) const;

    virtual void	getExtent(BinID& start,BinID& stop) const;
    virtual void	getZRange(Interval<float>&) const;
    virtual od_int64	estNrPos() const;
    virtual int		estNrZPerPos() const;

    const ZSampling&	zSampling() const	{ return zsamp_; }
    void		setZSampling( const ZSampling& zrg )
						{ zsamp_ = zrg; }
    int			nrSamples() const	{ return zsamp_.nrSteps()+1; }
    virtual bool	includes( const Coord& c, float z=mUdf(float) ) const
			{ return Pos::Provider3D::includes(c,z); }

protected:

    DBKey		id_;
    PosInfo::CubeData	cubedata_;
    ZSampling		zsamp_;
    PosInfo::CubeDataPos curpos_;
    int			curzidx_;

public:

    static void		initClass();
    static Provider3D*	create()	{ return new SeisProvider3D; }

};


} // namespace

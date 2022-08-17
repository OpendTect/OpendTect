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
#include "multiid.h"
#include "uistring.h"


namespace Pos
{

/*!\brief 3D provider based on cube of PS data store */

mExpClass(Seis) SeisProvider3D : public Provider3D
{ mODTextTranslationClass(Pos::SeisProvider3D);
public:

    typedef StepInterval<float>	ZSampling;

			SeisProvider3D();
			SeisProvider3D(const SeisProvider3D&);
			~SeisProvider3D();
    SeisProvider3D&	operator =(const SeisProvider3D&);
    const char*		type() const override		{ return sKeyType(); }
    const char*		factoryKeyword() const override { return type(); }
    static const char*	sKeyType()	{ return "Seismic Cube Positions"; }
    Provider*		clone() const override
			{ return new SeisProvider3D(*this); }

    uiRetVal		setSeisID(const MultiID&);
    MultiID		seisID() const		{ return id_; }

    void		reset() override;

    bool		toNextPos() override;
    bool		toNextZ() override;

    BinID		curBinID() const override;
    float		curZ() const override;
    bool		includes(const BinID&,
				 float z=mUdf(float)) const override;
    void		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    void		getSummary(BufferString&) const override;

    void		getExtent(BinID& start,BinID& stop) const override;
    void		getZRange(Interval<float>&) const override;
    od_int64		estNrPos() const override;
    int			estNrZPerPos() const override;

    const ZSampling&	zSampling() const	{ return zsamp_; }
    void		setZSampling( const ZSampling& zrg )
						{ zsamp_ = zrg; }
    int			nrSamples() const	{ return zsamp_.nrSteps()+1; }
    bool		includes( const Coord& c,
				  float z=mUdf(float) ) const override
			    { return Pos::Provider3D::includes(c,z); }

protected:

    MultiID		id_;
    PosInfo::CubeData	cubedata_;
    ZSampling		zsamp_;
    PosInfo::CubeDataPos curpos_;
    int			curzidx_;

public:

    static void		initClass();
    static Provider3D*	create()	{ return new SeisProvider3D; }

};

} // namespace Pos

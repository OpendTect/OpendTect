#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2002
________________________________________________________________________

-*/

#include "basicmod.h"
#include "cubesubsel.h"
#include "horsampling.h"


/*!\brief HorSampling and Z sampling.

  This class is meant to define Survey Geometries, not for subselection of
  existing geometries. See comments in HorSampling.

*/

mExpClass(Basic) CubeSampling
{
public:

    mUseType( HorSampling,	idx_type );
    mUseType( HorSampling,	size_type );
    mUseType( HorSampling,	pos_steprg_type );
    typedef Pos::Z_Type		z_type;
    typedef StepInterval<z_type> z_steprg_type;

			CubeSampling(bool settoSI=false,
				     OD::SurvLimitType slt=OD::FullSurvey);
			CubeSampling(const CubeSubSel&);
			CubeSampling(const TrcKeySampling&);
			CubeSampling(const TrcKeyZSampling&);
			CubeSampling(const BinID&,z_type);
			CubeSampling(const CubeSampling&);
    bool		operator==(const CubeSampling&) const;
    bool		operator!=(const CubeSampling&) const;
    bool		isDefined() const	{ return hsamp_.isDefined(); }

    void		init(bool settoSI=true,
			     OD::SurvLimitType slt=OD::FullSurvey);
    inline void		setEmpty()		{ init(false); }
    void		normalise();

    pos_steprg_type	inlRange() const	{ return hsamp_.inlRange(); }
    pos_steprg_type	crlRange() const	{ return hsamp_.crlRange(); }
    z_steprg_type	zRange() const		{ return zsamp_; }
    bool		includes(const BinID&,z_type) const;
    bool		includes(const CubeSampling&) const;

    bool		isEmpty() const		{ return totalNr() < 1; }
    bool		isFlat() const;
    size_type		nrInl() const		{ return hsamp_.nrInl(); }
    size_type		nrCrl() const		{ return hsamp_.nrCrl(); }
    size_type		nrZ() const		{ return zsamp_.nrSteps() + 1; }

    inline idx_type	idx4Z(z_type) const;
    inline z_type	z4Idx(idx_type) const;

    od_int64		totalNr() const;
    bool		isEqual(const CubeSampling&,
				z_type zeps=mUdf(z_type)) const;

    void		include(const BinID&,z_type);
    void		include(const CubeSampling&);
    void		limitTo(const CubeSampling&);

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;
    void		useJSON(const OD::JSON::Object&);
    void		fillJSON(OD::JSON::Object&) const;
    static void		removeInfo(IOPar&);

    HorSampling		hsamp_;
    ZSampling		zsamp_;

};


inline CubeSampling::idx_type CubeSampling::idx4Z( z_type z ) const
{
    return zsamp_.nearestIndex( z );
}


inline CubeSampling::z_type CubeSampling::z4Idx( idx_type idx ) const
{
    return zsamp_.atIndex( idx );
}

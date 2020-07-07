#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008 / Nov 2018
________________________________________________________________________

-*/

#include "ranges.h"
class CubeHorSubSel;
class TrcKeySampling;
class TrcKeyZSampling;

namespace OD
{
    namespace JSON
    {
	class Object;
    };
};


/*!\brief Horizontal sampling (inline and crossline range and steps).

  The access functions accept rg_type& (Interval), and will use the step
  if the Interval is actually a StepInterval (a sampling_type).

  Note that this class is only used to define geometries.
  They are established during import-like operations. If you want to
  subselect existing geometries, use the Cube- or Line-HorSubSel class.

 */

mExpClass(Basic) HorSampling
{ mODTextTranslationClass(HorSampling)
public:

    typedef Pos::Index_Type		idx_type;
    typedef idx_type			pos_type;
    typedef Pos::rg_type		pos_rg_type;
    typedef Pos::steprg_type		pos_steprg_type;
    typedef idx_type			size_type;

			HorSampling(bool settosi=false,
				    OD::SurvLimitType slt=OD::FullSurvey);
			HorSampling(const CubeHorSubSel&);
			HorSampling(const TrcKeySampling&);
			HorSampling(const TrcKeyZSampling&);
    explicit		HorSampling(const BinID&);
    bool		operator==(const HorSampling&) const;
    bool		operator!=(const HorSampling&) const;
    bool		isDefined() const;

    pos_steprg_type	inlRange() const;
    pos_steprg_type	crlRange() const;
    void		get(pos_rg_type&,pos_rg_type&) const;
    bool		includes(const BinID&) const;
    bool		includesInl(pos_type) const;
    bool		includesCrl(pos_type) const;

    void		setInlRange(const pos_rg_type&);
    void		setCrlRange(const pos_rg_type&);
    void		set(const pos_rg_type&,const pos_rg_type&);
    void		include(const BinID&);
    void		includeInl(pos_type);
    void		includeCrl(pos_type);
    void		include(const HorSampling&,bool ignoresteps=false);

    inline int		idx4Inl(pos_type) const;
    inline int		idx4Crl(pos_type) const;
    inline pos_type	inl4Idx(idx_type) const;
    inline pos_type	crl4Idx(idx_type) const;
    inline BinID	atIndex(idx_type,idx_type) const;

    bool		isEmpty() const	    { return totalNr() < 1; }
    bool		isLine() const;
    size_type		nrInl() const;
    size_type		nrCrl() const;
    BinID		center() const;
    bool		toNext(BinID&) const;
    od_int64		totalNr() const
			{ return ((od_int64)nrInl())*nrCrl(); }
    void		limitTo(const HorSampling&);
    void		limitTo(const CubeHorSubSel&);

    void		init(bool settoSI=true,
			     OD::SurvLimitType slt=OD::FullSurvey);
    void		normalise();
			    //!< Makes sure start_<stop_ and steps are non-zero

    void		usePar(const IOPar&);
    void		fillPar(IOPar&) const;
    void		useJSON(const OD::JSON::Object&);
    void		fillJSON(OD::JSON::Object&) const;
    static void		removeInfo(IOPar&);
    void		toString(uiPhrase&) const; //!< Nice text for info

    BinID		start_;
    BinID		stop_;
    BinID		step_;

    mDeprecated pos_steprg_type	lineRange() const { return inlRange(); }
    mDeprecated pos_steprg_type	trcRange() const { return crlRange(); }
    mDeprecated void setLineRange( const pos_rg_type& rg ) { setInlRange(rg);}
    mDeprecated void setTrcRange( const pos_rg_type& rg) { setCrlRange(rg);}
    mDeprecated size_type nrLines() const { return nrInl(); }
    mDeprecated size_type nrTrcs() const { return nrCrl(); }

};


inline HorSampling::idx_type HorSampling::idx4Inl( pos_type inl ) const
{
    return step_.inl()
	? (inl-start_.inl()) / step_.inl()
	: (inl==start_.inl() ? 0 : -1);
}


inline HorSampling::idx_type HorSampling::idx4Crl( pos_type crl ) const
{
    return step_.crl()
	? (crl-start_.crl()) / step_.crl()
	: (crl==start_.crl() ? 0 : -1);
}


inline HorSampling::pos_type HorSampling::inl4Idx( idx_type idx ) const
{
    return start_.inl() + step_.inl() * idx;
}


inline HorSampling::pos_type HorSampling::crl4Idx( idx_type idx ) const
{
    return start_.crl() + step_.crl() * idx;
}


inline BinID HorSampling::atIndex( idx_type inlidx, idx_type crlidx ) const
{
    return BinID( inl4Idx(inlidx), crl4Idx(crlidx) );
}

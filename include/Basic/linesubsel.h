#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "basicmod.h"
#include "horsubsel.h"
class TrcKeyZSampling;


/*!\brief subselection of a 2D line geometry. */

mExpClass(Basic) LineSubSel : public ArrRegSubSel2D
			    , public Survey::FullSubSel
{
public:

    mUseType( Pos,		IdxSubSel1D );
    mUseType( Pos,		IdxSubSelData );
    mUseType( Data,		idx_type );
    mUseType( Data,		size_type );
    mUseType( IdxSubSelData,	pos_type );
    mUseType( IdxSubSelData,	pos_steprg_type );
    mUseType( Survey,		HorSubSel );
    typedef IdxSubSel1D		TrcNrSubSel;
    typedef IdxSubSelData	TrcNrSubSelData;

			LineSubSel(GeomID);
			LineSubSel(const Geometry2D&);
			LineSubSel(const pos_steprg_type&);
			LineSubSel(const pos_steprg_type&,const z_steprg_type&);
			LineSubSel(const pos_steprg_type&,const ZSubSel&);
			LineSubSel(const TrcKeyZSampling&);
			mImplArrRegSubSelClone(LineSubSel)

    const TrcNrSubSelData& trcNrSubSel() const	{ return hss_.trcNrSubSel(); }
    TrcNrSubSelData& trcNrSubSel()		{ return hss_.trcNrSubSel(); }

    pos_steprg_type	trcNrRange() const	{ return hss_.trcNrRange(); }
    pos_steprg_type	fullTrcNrRange() const	{ return hss_.fullTrcNrRange();}
    idx_type		idx4TrcNr( pos_type trcnr ) const
			{ return hss_.idx4TrcNr( trcnr ); }
    pos_type		trcNr4Idx( idx_type idx ) const
			{ return hss_.trcNr4Idx( idx ); }
    size_type		nrTrcs() const		{ return hss_.nrTrcs(); }
    void		setTrcNrRange( const pos_steprg_type& rg )
			{ hss_.setTrcNrRange( rg ); }

    LineHorSubSel&	lineHorSubSel()		{ return hss_; }
    const LineHorSubSel& lineHorSubSel() const	{ return hss_; }

protected:

    LineHorSubSel hss_;

    HorSubSel&	gtHorSubSel() const
		{
		    return mSelf().hss_;
		}

    Data&	gtData( idx_type idim ) const override
		{
		    const Data* ret = &zSubSel();
		    if ( idim < 1 )
			ret = &trcNrSubSel();
		    return const_cast<Data&>( *ret );
		}

};

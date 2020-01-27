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
#include "enums.h"

class CubeSampling;
class TrcKeyZSampling;


/*!\brief subselection in all directions of a cube, directly usable as
  3D array subselector. It holds a HorSubSel and a ZSubSel.

  When slices are to be taken, order as follows:

  Dir |   Dim1  |  Dim2
  ----|---------|------
  Inl |   Crl   |  Z
  Crl |   Inl   |  Z
  Z   |   Inl   |  Crl

  See also the direction() and dimension() free functions.
  defaultDir() will return the 'flattest' slices.
*/

mExpClass(Basic) CubeSubSel : public ArrRegSubSel3D
			    , public Survey::GeomSubSel
{
public:

    mUseType( Survey,		HorSubSel );
    mUseType( Pos,		IdxSubSelData );
    mUseType( IdxSubSelData,	idx_type );
    mUseType( IdxSubSelData,	pos_type );
    mUseType( IdxSubSelData,	pos_steprg_type );
    mUseType( IdxSubSelData,	size_type );
    mUseType( OD,		SliceType );
				mDeclareEnumUtils(SliceType);

			CubeSubSel(OD::SurvLimitType slt=OD::FullSurvey);
			CubeSubSel(const SurveyInfo*);
			CubeSubSel(const Geometry3D&);
			CubeSubSel(const CubeHorSubSel&);
			CubeSubSel(const CubeHorSubSel&,const z_steprg_type&);
			CubeSubSel(const CubeHorSubSel&,const ZSubSel&);
			CubeSubSel(const pos_steprg_type&,
				   const pos_steprg_type&);
			CubeSubSel(const pos_steprg_type&,
				   const pos_steprg_type&,
				   const z_steprg_type&);
			CubeSubSel(const BinID&);
			CubeSubSel(const HorSampling&);
			CubeSubSel(const HorSampling&,const z_steprg_type&);
			CubeSubSel(const CubeSampling&);
			CubeSubSel(const TrcKeySampling&);
			CubeSubSel(const TrcKeyZSampling&);
			CubeSubSel(const CubeSubSel&,int nrchunks,int chunknr);
    bool		operator ==( const CubeSubSel& oth ) const
			{ return equals( oth ); }
			mImplSimpleIneqOper(CubeSubSel)
    bool		equals(const SubSel&) const override;
    bool		includes(const CubeSubSel&) const;
    bool		includes(const BinID&) const;
    const SurveyInfo&	survInfo() const	{ return hss_.survInfo(); }

    const IdxSubSelData& inlSubSel() const	{ return hss_.inlSubSel(); }
    IdxSubSelData&	inlSubSel()		{ return hss_.inlSubSel(); }
    const IdxSubSelData& crlSubSel() const	{ return hss_.crlSubSel(); }
    IdxSubSelData&	crlSubSel()		{ return hss_.crlSubSel(); }

    RowCol		horSizes() const	{ return hss_.arraySize(); }
    BinID		origin() const
			{ return BinID(inlRange().start,crlRange().start); }
    bool		isAll() const override	{ return GeomSubSel::isAll(); }

    size_type		nrInl() const		{ return inlSubSel().size(); }
    size_type		nrCrl() const		{ return crlSubSel().size(); }
    pos_steprg_type	inlRange() const
			{ return inlSubSel().outputPosRange(); }
    pos_steprg_type	crlRange() const
			{ return crlSubSel().outputPosRange(); }
    pos_steprg_type	fullInlRange() const
			{ return inlSubSel().inputPosRange(); }
    pos_steprg_type	fullCrlRange() const
			{ return crlSubSel().inputPosRange(); }
    BinID		binID4RowCol( const RowCol& rc ) const
			{ return hss_.binID4RowCol( rc ); }
    RowCol		rowCol4BinID( const BinID& bid ) const
			{ return hss_.rowCol4BinID( bid ); }
    bool		getIntersection(const CubeSubSel& oth,
					CubeSubSel&) const;
    void		normalise();

    void		setInlRange( const pos_steprg_type& rg )
			{ inlSubSel().setOutputPosRange( rg ); }
    void		setCrlRange( const pos_steprg_type& rg )
			{ crlSubSel().setOutputPosRange( rg ); }
    void		setRange(const BinID&,const BinID&,const BinID& step);
    void		setToAll();
    void		clearSubSel()
			{ hss_.clearSubSel(); zss_.clearSubSel(); }
    void		merge(const CubeSubSel&);
    void		limitTo(const CubeSubSel&);
    void		addStepout( pos_type i, pos_type c )
			{ hss_.addStepout(i,c); }

    bool		isFlat() const;
    SliceType		defaultDir() const;
    size_type		size(SliceType) const;
    void		getDefaultNormal(Coord3&) const;

    CubeHorSubSel&	cubeHorSubSel()		{ return hss_; }
    const CubeHorSubSel& cubeHorSubSel() const	{ return hss_; }

protected:

    CubeHorSubSel hss_;

		mImplArrRegSubSelClone(CubeSubSel)

    HorSubSel&	gtHorSubSel() const override
		{
		    return mSelf().hss_;
		}

    SSData&	gtSSData( idx_type idim ) const override
		{
		    const SSData* ret = &zSubSelData();
		    if ( idim < 2 )
			ret = idim ? &crlSubSel() : &inlSubSel();
		    return const_cast<SSData&>( *ret );
		}

};


inline OD::SliceType direction( OD::SliceType slctype, int dimnr )
{
    if ( dimnr == 0 )
	return slctype;
    else if ( dimnr == 1 )
	return slctype == OD::InlineSlice ? OD::CrosslineSlice
					  : OD::InlineSlice;
    else
	return slctype == OD::ZSlice ? OD::CrosslineSlice
				     : OD::ZSlice;
}


inline int dimension( OD::SliceType slctype, OD::SliceType direction )
{
    if ( slctype == direction )
	return 0;
    else if ( direction == OD::ZSlice )
	return 2;
    else if ( direction == OD::InlineSlice )
	return 1;

    return slctype == OD::ZSlice ? 2 : 1;
}

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2018
________________________________________________________________________

-*/

#include "basicmod.h"
#include "arrregsubsel.h"
#include "binid.h"


namespace Pos
{

/*!\brief data needed for regular subselection in an array of Pos:: indexes.

  The 3 parts represent:
  * A step in the available numbering (i.e. not in the idxpair number itself)
  * An offset from the Geometry 'origin', in steps in available numbering
  * The total size of the subselection

  For example, the input range is 300-700 step 3. Then to asking for 350-500
  step 7 will result in:
    - step 7/3 -> 2 idxes (resulting in actual rg step of 2*3 = 6)
    - offs (350-300)/6 = 8, corrected to 9 (leading to actual rg 354-498)
    - sz ((498-354)/6 + 1 = 25
    resulting in selected range of 354-498 step 6.

  Note that the requested output range will be adapted to something supported
  by the input range.

 */

mExpClass(Basic) IdxSubSelData : public ArrRegSubSelData
{
public:

    mUseType( IdxPair,			pos_type );
    typedef StepInterval<pos_type>	pos_steprg_type;

		IdxSubSelData(const pos_steprg_type&);
    bool	operator ==(const IdxSubSelData&) const;
		mImplSimpleIneqOper(IdxSubSelData)

    bool	isAll() const	{ return outputPosRange() == inpposrg_; }
    bool	hasFullRange() const;

    idx_type	idx4Pos(pos_type) const;
    pos_type	pos4Idx(idx_type) const;

    pos_type	posStart() const;
    pos_type	posStop() const;
    pos_type	posStep() const;
    bool	includes(pos_type) const;

    pos_steprg_type inputPosRange() const { return inpposrg_; }
    pos_steprg_type outputPosRange() const
		{ return pos_steprg_type( posStart(), posStop(), posStep() ); }

    void	setInputPosRange(const pos_steprg_type&);
    void	setOutputPosRange(pos_type start,pos_type stop,pos_type stp);
    inline void	setOutputPosRange( const pos_steprg_type& rg )
		{ setOutputPosRange( rg.start, rg.stop, rg.step ); }
    void	setOutputStep(pos_type step,pos_type existpos=mUdf(pos_type));

    void	limitTo(const IdxSubSelData&);
    void	widenTo(const IdxSubSelData&);

protected:

    pos_steprg_type	inpposrg_;

    void		ensureSizeOK();

};


/*!\brief Pos:: subindexing in a single dimension */

mExpClass(Basic) IdxSubSel1D : public ArrRegSubSel1D
{
public:

    mUseType( IdxSubSelData,	pos_type );
    mUseType( IdxSubSelData,	pos_steprg_type );

		IdxSubSel1D( const pos_steprg_type& rg )
		    : data_(rg)					{}
		mImplSimpleEqOpers1Memb(IdxSubSel1D,data_)

    const IdxSubSelData& posData() const	{ return data_; }
    IdxSubSelData&	posData()		{ return data_; }

    // for convenience, we duplicate the IdxSubSelData interface

    pos_type	posStart() const	{ return data_.posStart(); }
    pos_type	posStop() const		{ return data_.posStop(); }
    pos_type	posStep() const		{ return data_.posStep(); }
    pos_steprg_type inputPosRange() const { return data_.inputPosRange(); }
    pos_steprg_type outputPosRange() const { return data_.outputPosRange(); }

    bool	includes( pos_type pos ) const { return data_.includes(pos); }

    bool	isAll() const		{ return data_.isAll(); }
    bool	hasFullRange() const	{ return data_.hasFullRange(); }

    idx_type	idx4Pos( pos_type pos ) const { return data_.idx4Pos( pos ); }
    pos_type	pos4Idx( idx_type idx ) const { return data_.pos4Idx( idx ); }

    void	setInputPosRange( const pos_steprg_type& rg )
		{ data_.setInputPosRange( rg ); }
    void	setOutputPosRange( const pos_steprg_type& rg )
		{ data_.setOutputPosRange( rg ); }
    void	setOutputPosRange( pos_type start, pos_type stop, pos_type stp )
		{ data_.setOutputPosRange( start, stop, stp ); }

protected:

    IdxSubSelData	data_;

    Data&		gtData( idx_type ) const override
			{ return mSelf().data_; }

};


/*!\brief Pos:: subindexing in two dimensions (usually inline and crossline) */

mExpClass(Basic) IdxSubSel2D : public ArrRegSubSel2D
{
public:

    mUseType( IdxSubSelData,	pos_type );
    mUseType( IdxSubSelData,	pos_steprg_type );

		IdxSubSel2D( const pos_steprg_type& rg0,
			     const pos_steprg_type& rg1 )
		    : data0_(rg0), data1_(rg1)				{}
		IdxSubSel2D( const IdxSubSel1D& ss0,
			     const IdxSubSel1D& ss1 )
		    : data0_(ss0.posData()), data1_(ss1.posData())	{}
		mImplSimpleEqOpers2Memb(IdxSubSel2D,data0_,data1_)

    IdxSubSelData&	posData( idx_type idim )
			{ return idim ? data1_ : data0_; }
    const IdxSubSelData& posData( idx_type idim ) const
			{ return idim ? data1_ : data0_; }

    inline bool	isAll() const
		{ return data0_.isAll() && data1_.isAll(); }
    inline bool	hasFullRange() const
		{ return data0_.hasFullRange() && data1_.hasFullRange(); }

		// convenience fns usable when idxs represent inl/crl
    BinID	binID( idx_type iinl, idx_type icrl ) const
		{ return BinID( data0_.pos4Idx(iinl), data1_.pos4Idx(icrl) ); }
    RowCol	rowCol( pos_type inl, pos_type crl ) const
		{ return RowCol( data0_.idx4Pos(inl), data1_.idx4Pos(crl) ); }
    bool	includes( const BinID& bid ) const
		{ return data0_.includes(bid.inl())
		      && data1_.includes(bid.crl()); }
    pos_steprg_type inlRange() const { return data0_.outputPosRange(); }
    pos_steprg_type crlRange() const { return data1_.outputPosRange(); }

protected:

    IdxSubSelData	data0_;
    IdxSubSelData	data1_;

    Data&		gtData( idx_type idim ) const override
			{ return const_cast<IdxSubSelData&>( idim ? data1_
								  : data0_ ); }

};


} // namespace Pos

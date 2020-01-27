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

  Thus, note that the requested output range will be adapted to something
  supported by the input range.

 */

mExpClass(Basic) IdxSubSelData : public ArrRegSubSelData
{
public:

    mUseType( IdxPair,		pos_type );
    typedef Pos::steprg_type	pos_steprg_type;

		IdxSubSelData(const pos_steprg_type&);
    bool	operator ==(const IdxSubSelData&) const;
		mImplSimpleIneqOper(IdxSubSelData)
    bool	includes(const IdxSubSelData&) const;
    bool	includes(pos_type) const;

    bool	isAll() const	{ return outputPosRange() == inpposrg_; }
    bool	sameOutputPosRange(const IdxSubSelData&) const;
    bool	hasFullRange() const;

    idx_type	idx4Pos(pos_type) const;
    pos_type	pos4Idx(idx_type) const;

    pos_type	posStart() const;
    pos_type	posStop() const;
    pos_type	posStep() const;

    pos_steprg_type inputPosRange() const { return inpposrg_; }
    pos_steprg_type outputPosRange() const
		{ return pos_steprg_type( posStart(), posStop(), posStep() ); }
    size_type	inputSize() const	    { return inpposrg_.nrSteps()+1; }
    size_type	outputSize() const	    { return size(); }

    void	setInputPosRange(const pos_steprg_type&);
    void	setOutputPosRange(pos_type start,pos_type stop,pos_type stp);
    inline void	setOutputPosRange( const pos_steprg_type& rg )
		{ setOutputPosRange( rg.start, rg.stop, rg.step ); }
    void	setOutputStep(pos_type step,pos_type existpos=mUdf(pos_type));

    void	clearSubSel()	{ ArrRegSubSelData::clearSubSel(inputSize()); }
    void	limitTo(const IdxSubSelData&);
    void	widenTo(const IdxSubSelData&);
    void	addStepout(idx_type);

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
		    : ssdata_(rg)		{}
		mImplSimpleEqOpers1Memb(IdxSubSel1D,ssdata_)
    bool	includes( const IdxSubSel1D& oth ) const
		{ return ssdata_.includes( oth.ssdata_ ); }

    const IdxSubSelData& posData() const	{ return ssdata_; }
    IdxSubSelData&	posData()		{ return ssdata_; }

    // for convenience, we duplicate the IdxSubSelData interface

    pos_type	posStart() const	{ return ssdata_.posStart(); }
    pos_type	posStop() const		{ return ssdata_.posStop(); }
    pos_type	posStep() const		{ return ssdata_.posStep(); }
    pos_steprg_type inputPosRange() const { return ssdata_.inputPosRange(); }
    pos_steprg_type outputPosRange() const { return ssdata_.outputPosRange(); }

    bool	includes( pos_type pos ) const { return ssdata_.includes(pos); }

    bool	isAll() const		{ return ssdata_.isAll(); }
    bool	hasFullRange() const	{ return ssdata_.hasFullRange(); }

    idx_type	idx4Pos( pos_type pos ) const { return ssdata_.idx4Pos( pos ); }
    pos_type	pos4Idx( idx_type idx ) const { return ssdata_.pos4Idx( idx ); }

    void	setInputPosRange( const pos_steprg_type& rg )
		{ ssdata_.setInputPosRange( rg ); }
    void	setOutputPosRange( const pos_steprg_type& rg )
		{ ssdata_.setOutputPosRange( rg ); }
    void	setOutputPosRange( pos_type start, pos_type stop, pos_type stp )
		{ ssdata_.setOutputPosRange( start, stop, stp ); }

    void	clearSubSel()		{ ssdata_.clearSubSel(); }

protected:

    IdxSubSelData	ssdata_;

    SSData&		gtSSData( idx_type ) const override
			{ return mSelf().ssdata_; }

};


/*!\brief Pos:: subindexing in two dimensions (usually inline and crossline) */

mExpClass(Basic) IdxSubSel2D : public ArrRegSubSel2D
{
public:

    mUseType( IdxSubSelData,	pos_type );
    mUseType( IdxSubSelData,	pos_steprg_type );

		IdxSubSel2D( const pos_steprg_type& rg0,
			     const pos_steprg_type& rg1 )
		    : ssdata0_(rg0), ssdata1_(rg1)			{}
		IdxSubSel2D( const IdxSubSel1D& ss0,
			     const IdxSubSel1D& ss1 )
		    : ssdata0_(ss0.posData()), ssdata1_(ss1.posData())	{}
		mImplSimpleEqOpers2Memb(IdxSubSel2D,ssdata0_,ssdata1_)
    bool	includes( const IdxSubSel2D& oth ) const
		{ return ssdata0_.includes( oth.ssdata0_ )
		      && ssdata1_.includes( oth.ssdata1_ ); }

    IdxSubSelData&	posData( idx_type idim )
			{ return idim ? ssdata1_ : ssdata0_; }
    const IdxSubSelData& posData( idx_type idim ) const
			{ return idim ? ssdata1_ : ssdata0_; }

    inline bool	isAll() const
		{ return ssdata0_.isAll() && ssdata1_.isAll(); }
    inline bool	hasFullRange() const
		{ return ssdata0_.hasFullRange() && ssdata1_.hasFullRange(); }

		// convenience fns usable when idxs represent inl/crl
    BinID	binID( idx_type iinl, idx_type icrl ) const
		{ return BinID( ssdata0_.pos4Idx(iinl),
				ssdata1_.pos4Idx(icrl) ); }
    RowCol	rowCol( pos_type inl, pos_type crl ) const
		{ return RowCol( ssdata0_.idx4Pos(inl),
				 ssdata1_.idx4Pos(crl) ); }
    bool	includes( const BinID& bid ) const
		{ return ssdata0_.includes(bid.inl())
		      && ssdata1_.includes(bid.crl()); }
    pos_steprg_type inlRange() const { return ssdata0_.outputPosRange(); }
    pos_steprg_type crlRange() const { return ssdata1_.outputPosRange(); }

    void	clearSubSel()
		{ ssdata0_.clearSubSel(); ssdata1_.clearSubSel(); }

protected:

    IdxSubSelData	ssdata0_;
    IdxSubSelData	ssdata1_;

    SSData&		gtSSData( idx_type idim ) const override
			{ return const_cast<IdxSubSelData&>( idim ? ssdata1_
								  : ssdata0_ );}

};


} // namespace Pos

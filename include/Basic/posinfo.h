#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		2005 / Mar 2008
________________________________________________________________________

-*/

#include "basicmod.h"
#include "manobjectset.h"
#include "typeset.h"
#include "indexinfo.h"
#include "binid.h"
#include "bin2d.h"
#include "od_iosfwd.h"
class CubeHorSubSel;
namespace Survey { class HorSubSel; class FullSubSel; }


/*!\brief Position info, segmented

In sets of lines (including 'cubes') containing gaps and other irregularities
in a fully regular setup, a complete description of the positions present can
be done by provding a list of the regular segments per line. No sorting of
lines is required.

The crossline segments are assumed to be sorted, i.e.:
[1-3,1] [5-9,2] : OK
[9-5,-1] [3-1,-1] : OK
[5-9,2] [1-3,1] : Not OK

Note that the LineData class is also interesting for 2D lines with trace
numbers.

*/

namespace PosInfo
{


/*!\brief Index Position in a LineData. */

mExpClass(Basic) LinePos
{
public:

    typedef Index_Type	idx_type;

		LinePos( idx_type iseg=0, idx_type sidx=-1 )
		    : segnr_(iseg), sidx_(sidx)		    {}

    idx_type	segnr_;
    idx_type	sidx_;

    void	toPreStart()	{ segnr_ = 0; sidx_ = -1; }
    void	toStart()	{ segnr_ = sidx_ = 0; }
    bool	isValid() const	{ return segnr_>=0 && sidx_>=0; }

};

/*!\brief Position info for a line (3D: inlines, 2D: lines (with GeomID)).
  Kept as number segments with a regular numbering each. */

mExpClass(Basic) LineData
{
public:

    mUseType( LinePos,		idx_type );
    mUseType( Pos,		GeomID );
    mUseType( Pos,		IdxPair );
    typedef Pos::Index_Type	pos_type;
    typedef Pos::rg_type	pos_rg_type;
    typedef Pos::steprg_type	pos_steprg_type;
    typedef idx_type		size_type;
    typedef pos_steprg_type	Segment;
    typedef TypeSet<Segment>	SegmentSet;

			LineData( pos_type lnr=0 )
			    : linenr_(lnr)	{}
    bool		operator ==(const LineData&) const;
			mImplSimpleIneqOper(LineData)

    pos_type		linenr_;
    SegmentSet		segments_;

    size_type		size() const;
    bool		isEmpty() const	{ return segments_.isEmpty(); }
    idx_type		segmentOf(pos_type) const;
    inline bool		includes( pos_type nr ) const
						{ return segmentOf(nr) >= 0; }
    pos_rg_type		range() const;
    pos_type		minStep() const;
    void		merge(const LineData&,bool incl);
				//!< incl=union, !incl=intersection

    bool		isValid(const LinePos&) const;
    bool		toNext(LinePos&) const;
    bool		toPrev(LinePos&) const;
    pos_type		pos( const LinePos& ldp ) const
			{ return segments_[ldp.segnr_].atIndex(ldp.sidx_); }
    IdxPair		idxPair( const LinePos& ldp ) const
			{ return IdxPair( linenr_, pos(ldp) ); }
    BinID		binID( const LinePos& ldp ) const
			{ return BinID( linenr_, pos(ldp) ); }
    Bin2D		bin2D( const LinePos& ldp ) const
			{ return Bin2D( GeomID(linenr_), pos(ldp) ); }
    IdxPair		first() const
			{ return IdxPair(linenr_,segments_.first().start); }
    BinID		firstBinID() const
			{ return BinID(first()); }
    Bin2D		firstBin2D() const
			{ const auto frst = first();
			  return Bin2D( GeomID(frst.lineNr()), frst.trcNr() ); }

    pos_type		centerNumber() const;  //!< not exact
    pos_type		nearestNumber(pos_type) const;
    idx_type		nearestSegment(double) const;

};


/*!\brief Position in LineCollData. */

mExpClass(Basic) LineCollPos
{
public:

    mUseType( LinePos,	idx_type );

		LineCollPos()				   { toPreStart(); }
		LineCollPos( idx_type iln, idx_type isn=0, idx_type sidx=-1 )
		    : lidx_(iln), segnr_(isn), sidx_(sidx) {}

    idx_type	lidx_;
    idx_type	segnr_;
    idx_type	sidx_;

    void	toPreStart()	{ lidx_ = segnr_ = 0; sidx_ = -1; }
    void	toStart()	{ lidx_ = segnr_ = sidx_ = 0; }
    bool	isValid() const	{ return lidx_>=0 && segnr_>=0 && sidx_>=0; }

};


/*!\brief Position info for a collection of Line objects */

mExpClass(Basic) LineCollData : public ManagedObjectSet<LineData>
{
public:

    mUseType( LineData,	idx_type );
    mUseType( LineData,	size_type );
    mUseType( LineData,	pos_type );
    mUseType( LineData,	pos_rg_type );
    mUseType( LineData,	pos_steprg_type );
    mUseType( Survey,	FullSubSel );
    mUseType( Pos,	GeomID );
    mUseType( Pos,	IdxPair );
    typedef od_int64	glob_idx_type;
    typedef od_int64	glob_size_type;

			LineCollData()		{}
			LineCollData( const LineCollData& oth )
			    : ManagedObjectSet<LineData>()
						{ *this = oth; }
    LineCollData&	operator =( const LineCollData& oth )
				{ copyContents(oth); return *this; }
    bool		operator ==(const LineCollData&) const;
			mImplSimpleIneqOper(LineCollData)

    glob_size_type	totalSize() const;
    glob_size_type	totalNrSegments() const;

    virtual idx_type	lineIndexOf(pos_type lnr,idx_type* newidx=0) const;
    bool		includes( pos_type lnr ) const
			{ return lineIndexOf(lnr) >= 0; }
    bool		includes(pos_type lnr,pos_type trcnr) const;
    bool		includes(const BinID&) const;
    bool		includes(const Bin2D&) const;

    bool		isValid(const LineCollPos&) const;
    bool		toNext(LineCollPos&) const;
    bool		toPrev(LineCollPos&) const;
    bool		toNextLine(LineCollPos&) const;
    BinID		binID(const LineCollPos&) const;
    Bin2D		bin2D(const LineCollPos&) const;
    pos_type		trcNr(const LineCollPos&) const;
    LineCollPos		lineCollPos(const BinID&) const;
    LineCollPos		lineCollPos(const Bin2D&) const;

    void		limitTo(const Survey::HorSubSel&);
    void		merge(const LineCollData&,bool incl);
				//!< incl=union, !incl=intersection
    void		getFullSubSel(FullSubSel&,bool is2d) const;

    bool		read(od_istream&,bool asc);
    bool		write(od_ostream&,bool asc) const;

protected:

    void		copyContents(const LineCollData&);

};


typedef LineCollPos CubeDataPos;

/*!\brief Position info for an entire 3D cube. The LineData's are not
  automatically sorted. */

mExpClass(Basic) CubeData : public LineCollData
{
public:

			CubeData()		{}
			CubeData( BinID start, BinID stop, BinID step )
						{ generate(start,stop,step); }
			CubeData( const CubeData& oth )
						{ copyContents(oth); }
			CubeData( const LineCollData& oth )
						{ copyContents(oth); }
    CubeData&		operator =( const CubeData& oth )
			{ copyContents(oth); return *this; }
    CubeData&		operator =( const LineCollData& lcd )
			{ copyContents(lcd); return *this; }
    bool		operator ==( const CubeData& oth ) const
			{ return LineCollData::operator ==( oth ); }
    bool		operator ==( const LineCollData& oth ) const
			{ return LineCollData::operator ==( oth ); }
			mImplSimpleIneqOper(CubeData)
			mImplSimpleIneqOper(LineCollData)

    glob_size_type	totalSizeInside(const CubeHorSubSel&) const;
			/*!<Only take positions that are inside hrg. */

    void		getRanges(pos_rg_type& inl,pos_rg_type& crl) const;
    bool		getInlRange(pos_steprg_type&,bool sorted=true) const;
			    //!< Returns whether fully regular.
    bool		getCrlRange(pos_steprg_type&,bool sorted=true) const;
			    //!< Returns whether fully regular.
    bool		getCubeHorSubSel(CubeHorSubSel&) const;
			    //!< Returns whether fully regular.
    void		getCubeSubSel( FullSubSel& fss ) const
			{ return getFullSubSel( fss, false ); }

    BinID		minStep() const;
    BinID		nearestBinID(const BinID&) const;
    BinID		centerPos() const;  //!< not exact
    bool		hasPosition(const CubeHorSubSel&,glob_idx_type) const;
    CubeDataPos		cubeDataPos( const BinID& bid ) const
						{ return lineCollPos(bid); }

    bool		haveInlStepInfo() const	{ return size() > 1; }
    bool		haveCrlStepInfo() const;
    bool		isFullyRectAndReg() const;
    bool		isAll(const CubeHorSubSel&) const;
    bool		isCrlReversed() const;

    void		generate(BinID start,BinID stop,BinID step,
				 bool allowreversed=false);
    void		fillBySI(OD::SurvLimitType slt=OD::FullSurvey);

};


/*!\brief Position info for an entire 3D cube. The LineData's added are
  automatically sorted. */

mExpClass(Basic) SortedCubeData : public CubeData
{
public:
			SortedCubeData()				{}
			SortedCubeData( const BinID& start, const BinID& stop,
					const BinID& step )
						{ generate(start,stop,step); }
			SortedCubeData( const SortedCubeData& oth )
						{ copyContents( oth ); }
			SortedCubeData(const LineCollData&) = delete;
    SortedCubeData&	operator =( const SortedCubeData& scd )
			{ copyContents(scd); return *this; }
    SortedCubeData&	operator =( const LineCollData& lcd )
			{ copyContents(lcd); return *this; }

    virtual idx_type	lineIndexOf(pos_type inl,idx_type* newidx=0) const;

    SortedCubeData&	add(LineData*);

protected:

    virtual SortedCubeData& doAdd(LineData*) override;

};


/*!\brief Iterates through LineCollData */

mExpClass(Basic) LineCollDataIterator
{
public:

    mUseType( LineCollData, pos_type );

			LineCollDataIterator( const LineCollData& lcd )
			    : lcd_(lcd)		{}

    inline bool		next()
			{ return lcd_.toNext( lcp_ ); }
    inline bool		next( BinID& bid )
			{
			    if ( !next() )
				return false;
			    bid = binID();
			    return true;
			}
    inline bool		next( Bin2D& b2d )
			{
			    if ( !next() )
				return false;
			    b2d = bin2D();
			    return true;
			}

    inline void		reset()		{ lcp_.toPreStart(); }
    inline BinID	binID() const	{ return lcd_.binID( lcp_ ); }
    inline Bin2D	bin2D() const	{ return lcd_.bin2D( lcp_ ); }
    inline pos_type	trcNr() const	{ return lcd_.trcNr( lcp_ ); }

    const LineCollData&	lcd_;
    LineCollPos		lcp_;

};

typedef LineCollDataIterator CubeDataIterator;


/*!\brief Fills LineData. Requires feed of sorted trcnrs  */

mExpClass(Basic) LineDataFiller
{
public:

    mUseType( LineData,	pos_type );
    mUseType( LineData,	size_type );
    mUseType( LineData,	idx_type );

			LineDataFiller(LineData&);
			~LineDataFiller()	{ if ( !finished_ ) finish(); }
    void		reset();

    LineDataFiller&	add(pos_type);
    LineDataFiller&	add(const pos_type*,size_type);
    LineDataFiller&	add( const TypeSet<pos_type>& ps )
			{ return add( ps.arr(), ps.size() ); }

    bool		finish();		//!< true if any valid nr added

    LineData&		lineData()		{ return ld_; }
    const LineData&	lineData() const	{ return ld_; }
    pos_type		prevNr() const		{ return prevnr_; }

protected:

    LineData&		ld_;
    LineData::Segment	seg_;
    pos_type		prevnr_;
    bool		finished_;

};


/*!\brief Fills LineCollData. Requires per-line feed with trcnrs sorted  */

mExpClass(Basic) LineCollDataFiller
{
public:

    mUseType( LineCollData,	IdxPair );

			LineCollDataFiller(LineCollData&);
			~LineCollDataFiller()	{ finish(); }
    void		reset();

    LineCollDataFiller&	add( const Bin2D& b2d )	{ return doAdd(b2d.idxPair()); }
    LineCollDataFiller&	add( const BinID& bid )	{ return doAdd(bid); }

    void		finish();

protected:

    LineCollData&	lcd_;
    LineData*		ld_		    = nullptr;
    LineDataFiller*	ldf_		    = nullptr;

    LineCollDataFiller&	doAdd(const IdxPair&);
    void		finishLine();

};

} // namespace PosInfo

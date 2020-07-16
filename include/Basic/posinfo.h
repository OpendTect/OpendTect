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
class TrcKey;
namespace Survey { class HorSubSel; class FullHorSubSel; }


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

mExpClass(Basic) LineDataPos
{
public:

    typedef Index_Type	idx_type;

		LineDataPos( idx_type iseg=0, idx_type sidx=-1 )
		    : segnr_(iseg), sidx_(sidx)		{}
    virtual	~LineDataPos()				{}

    idx_type	segnr_;
    idx_type	sidx_;

    virtual void	toPreStart()	{ segnr_ = 0; sidx_ = -1; }
    virtual void	toStart()	{ segnr_ = sidx_ = 0; }
    virtual bool	isValid() const	{ return segnr_>=0 && sidx_>=0; }

};

/*!\brief Position info for a line (3D: inlines, 2D: lines (with GeomID)).
  Kept as number segments with a regular numbering each. */

mExpClass(Basic) LineData
{
public:

    mUseType( LineDataPos,	idx_type );
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

    bool		isValid(const LineDataPos&) const;
    bool		toNext(LineDataPos&) const;
    bool		toPrev(LineDataPos&) const;
    pos_type		pos( const LineDataPos& ldp ) const
			{ return segments_[ldp.segnr_].atIndex(ldp.sidx_); }
    IdxPair		idxPair( const LineDataPos& ldp ) const
			{ return IdxPair( linenr_, pos(ldp) ); }
    BinID		binID( const LineDataPos& ldp ) const
			{ return BinID( linenr_, pos(ldp) ); }
    Bin2D		bin2D( const LineDataPos& ldp ) const
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

mExpClass(Basic) LineCollDataPos : public LineDataPos
{
public:

		LineCollDataPos()			{ toPreStart(); }
		LineCollDataPos( idx_type iln, idx_type isn=0,
				 idx_type sidx=-1 )
		    : LineDataPos(isn,sidx), lidx_(iln)	{}

    idx_type	lidx_;

    void	toPreStart() override
		{ LineDataPos::toPreStart(); lidx_ = 0; }
    void	toStart() override
		{ LineDataPos::toStart(); lidx_ = 0; }
    bool	isValid() const override
		{ return LineDataPos::isValid() && lidx_>=0; }

};


class CubeData;
class LinesData;

/*!\brief Position info for a collection of Line objects */

mExpClass(Basic) LineCollData : public ManagedObjectSet<LineData>
{
public:

    mUseType( LineData,	idx_type );
    mUseType( LineData,	size_type );
    mUseType( LineData,	pos_type );
    mUseType( LineData,	pos_rg_type );
    mUseType( LineData,	pos_steprg_type );
    mUseType( Survey,	FullHorSubSel );
    mUseType( Pos,	IdxPair );
    typedef od_int64	glob_idx_type;
    typedef od_int64	glob_size_type;

    virtual LineCollData* clone() const		= 0;
    static LineCollData* create(const FullHorSubSel&);

    virtual bool	isLineSorted() const; //!< checks ascending only
    LineCollData&	operator =( const LineCollData& oth )
				{ copyContents(oth); return *this; }
    bool		operator ==(const LineCollData&) const;
			mImplSimpleIneqOper(LineCollData)

    virtual bool	isCubeData() const		= 0;
    bool		isLinesData() const		{ return !isCubeData();}
    CubeData*		asCubeData();
    const CubeData*	asCubeData() const;
    LinesData*		asLinesData();
    const LinesData*	asLinesData() const;

    virtual bool	isFullyRegular() const		= 0;
    glob_size_type	totalSize() const;
    glob_size_type	totalNrSegments() const;
    glob_size_type      totalSizeInside(const Survey::HorSubSel&) const;
                                /*!<Only count positions that are inside */

    virtual idx_type	lineIndexOf(pos_type lnr,idx_type* newidx=0) const;
    bool		includesLine( pos_type lnr ) const
			{ return lineIndexOf(lnr) >= 0; }
    bool		includes(pos_type lnr,pos_type trcnr) const;
    bool		includes(const BinID&) const;
    bool		includes(const Bin2D&) const;
    bool		includes(const TrcKey&) const;

    bool		isValid(const LineCollDataPos&) const;
    bool		hasPosition(const Survey::HorSubSel&,
				    glob_size_type) const;
    bool		toNext(LineCollDataPos&) const;
    bool		toPrev(LineCollDataPos&) const;
    bool		toNextLine(LineCollDataPos&) const;
    BinID		binID(const LineCollDataPos&) const;
    Bin2D		bin2D(const LineCollDataPos&) const;
    pos_type		lineNr(const LineCollDataPos&) const;
    pos_type		trcNr(const LineCollDataPos&) const;
    LineCollDataPos	lineCollPos(pos_type lnr,pos_type trcnr) const;
    LineCollDataPos	lineCollPos(const BinID&) const;
    LineCollDataPos	lineCollPos(const Bin2D&) const;
    LineCollDataPos	lineCollPos(const TrcKey&) const;

    void		limitTo(const Survey::HorSubSel&);
    void		merge(const LineCollData&,bool incl);
				//!< incl=union, !incl=intersection
    void		getFullHorSubSel(FullHorSubSel&,bool is2d) const;

    bool		read(od_istream&,bool asc);
    bool		write(od_ostream&,bool asc) const;

protected:

			LineCollData()	{}
			LineCollData( const LineCollData& oth )
			    : ManagedObjectSet<LineData>()
					{ *this = oth; }

    void		copyContents(const LineCollData&);

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
    LineCollDataPos	lcp_;

};


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

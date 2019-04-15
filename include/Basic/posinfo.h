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
#include "od_iosfwd.h"
class CubeHorSubSel;
namespace Survey { class SubGeometry3D; }


/*!\brief Position info, often segmented

In data cubes with gaps and other irregulaities, a complete description
of the positions present can be done by describing the regular segments
per inline. No sorting of inlines is required.

The crossline segments are assumed to be sorted, i.e.:
[1-3,1] [5-9,2] : OK
[9-5,-1] [3-1,-1] : OK
[5-9,2] [1-3,1] : Not OK

Note that the LineData class is also interesting for 2D lines with trace
numbers.

*/

namespace PosInfo
{


/*!\brief Position in a LineData. */

mExpClass(Basic) LineDataPos
{
public:

    typedef Index_Type	idx_type;

		LineDataPos( idx_type isn=0, idx_type sidx=-1 )
		    : segnr_(isn), sidx_(sidx)		    {}

    idx_type	segnr_;
    idx_type	sidx_;

    void	toPreStart()	{ segnr_ = 0; sidx_ = -1; }
    void	toStart()	{ segnr_ = sidx_ = 0; }
    bool	isValid() const	{ return segnr_>=0 && sidx_>=0; }

};

/*!\brief Position info for a line - in a 3D cube, that would be an inline.
  Stored as (crossline-)number segments. */

mExpClass(Basic) LineData
{
public:

    mUseType( LineDataPos,	idx_type );
    typedef Pos::Index_Type	pos_type;
    typedef Pos::rg_type	pos_rg_type;
    typedef Pos::steprg_type	pos_steprg_type;
    typedef idx_type		size_type;
    typedef pos_steprg_type	Segment;
    typedef TypeSet<Segment>	SegmentSet;

			LineData( pos_type i ) : linenr_(i)	{}
    bool		operator ==(const LineData&) const;
			mImplSimpleIneqOper(LineData)

    const pos_type	linenr_;
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
    BinID		binID( const LineDataPos& ldp ) const
			{ return BinID( linenr_, pos(ldp) ); }

    pos_type		centerNumber() const;  //!< not exact
    pos_type		nearestNumber(pos_type) const;
    idx_type		nearestSegment(double) const;

};


/*!\brief Position in a CubeData. */

mExpClass(Basic) CubeDataPos
{
public:

    mUseType( LineDataPos,	idx_type );

		CubeDataPos()				   { toPreStart(); }
		CubeDataPos( idx_type iln, idx_type isn=0, idx_type sidx=-1 )
		    : lidx_(iln), segnr_(isn), sidx_(sidx) {}

    idx_type	lidx_;
    idx_type	segnr_;
    idx_type	sidx_;

    void	toPreStart()	{ lidx_ = segnr_ = 0; sidx_ = -1; }
    void	toStart()	{ lidx_ = segnr_ = sidx_ = 0; }
    bool	isValid() const	{ return lidx_>=0 && segnr_>=0 && sidx_>=0; }

};


/*!\brief Position info for an entire 3D cube. The LineData's are not
  automatically sorted. */

mExpClass(Basic) CubeData : public ManagedObjectSet<LineData>
{
public:

    mUseType( LineData,		idx_type );
    mUseType( LineData,		size_type );
    mUseType( LineData,		pos_type );
    mUseType( LineData,		pos_rg_type );
    mUseType( LineData,		pos_steprg_type );

			CubeData()		{}
			CubeData( BinID start, BinID stop, BinID step )
						{ generate(start,stop,step); }
			CubeData( const CubeData& cd )
			    : ManagedObjectSet<LineData>()
						{ *this = cd; }
    CubeData&		operator =( const CubeData& cd )
			{ copyContents(cd); return *this; }

    size_type		totalSize() const;
    size_type		totalSizeInside(const CubeHorSubSel&) const;
			/*!<Only take positions that are inside hrg. */
    size_type		totalNrSegments() const;

    virtual idx_type	indexOf(pos_type inl,idx_type* newidx=0) const;
			//!< newidx only filled if not null and -1 is returned
    bool		includes(pos_type inl,pos_type crl) const;
    bool		includes(const BinID&) const;
    void		getRanges(pos_rg_type& inl,pos_rg_type& crl) const;
    bool		getInlRange(pos_steprg_type&,bool sorted=true) const;
			    //!< Returns whether fully regular.
    bool		getCrlRange(pos_steprg_type&,bool sorted=true) const;
			    //!< Returns whether fully regular.

    bool		isValid(const CubeDataPos&) const;
    bool		isValid(const BinID&) const;
    bool		isValid(od_int64 globidx,const CubeHorSubSel&) const;
    BinID		minStep() const;
    BinID		nearestBinID(const BinID&) const;
    BinID		centerPos() const;  //!< not exact

    bool		toNext(CubeDataPos&) const;
    bool		toPrev(CubeDataPos&) const;
    bool		toNextLine(CubeDataPos&) const;
    BinID		binID(const CubeDataPos&) const;
    CubeDataPos		cubeDataPos(const BinID&) const;

    bool		haveInlStepInfo() const		{ return size() > 1; }
    bool		haveCrlStepInfo() const;
    bool		isFullyRectAndReg() const;
    bool		isAll(const CubeHorSubSel&) const;
    bool		isCrlReversed() const;

    void		limitTo(const CubeHorSubSel&);
    void		merge(const CubeData&,bool incl);
				//!< incl=union, !incl=intersection
    void		generate(BinID start,BinID stop,BinID step,
				 bool allowreversed=false);
    void		fillBySI(OD::SurvLimitType slt=OD::FullSurvey);

    bool		read(od_istream&,bool asc);
    bool		write(od_ostream&,bool asc) const;

    virtual idx_type	indexOf( const LineData* l ) const
			{ return ObjectSet<LineData>::indexOf( l ); }

protected:

    void		copyContents(const CubeData&);

};


/*!\brief Position info for an entire 3D cube. The LineData's add are
  automatically sorted. */

mExpClass(Basic) SortedCubeData : public CubeData
{
public:
			SortedCubeData()				{}
			SortedCubeData( const BinID& start, const BinID& stop,
				  const BinID& step )
			    : CubeData(start,stop,step)		{}
			SortedCubeData( const SortedCubeData& cd )
			    : CubeData( cd )
								{ *this = cd; }
			SortedCubeData( const CubeData& cd )	{ *this = cd; }
    SortedCubeData&	operator =( const SortedCubeData& scd )
			{ copyContents(scd); return *this; }
    SortedCubeData&	operator =( const CubeData& cd )
			{ copyContents(cd); return *this; }

    virtual idx_type	indexOf(pos_type inl,idx_type* newidx=0) const;
			//!< newidx only filled if not null and -1 is returned

    SortedCubeData&	add(LineData*);

    virtual idx_type	indexOf( const LineData* l ) const
			{ return CubeData::indexOf( l ); }

protected:

    virtual CubeData&	doAdd(LineData*);

};


/*!\brief Fills CubeData object. Requires inline- and crossline-sorting.  */

mExpClass(Basic) CubeDataFiller
{
public:

    mUseType( CubeData,	pos_type );

			CubeDataFiller(CubeData&);
			~CubeDataFiller();

    void		add(const BinID&);
    void		finish(); // automatically called on delete

protected:

    CubeData&		cd_;
    LineData*		ld_;
    LineData::Segment	seg_;
    pos_type		prevcrl;

    void		initLine();
    void		finishLine();
    LineData*		findLine(pos_type);

};


/*!\brief Iterates through CubeData */

mExpClass(Basic) CubeDataIterator
{
public:

			CubeDataIterator( const CubeData& cd )
			    : cd_(cd)	{}

    inline bool		next( BinID& bid )
			{
			    const bool rv = cd_.toNext( cdp_ );
			    bid = binID(); return rv;
			}
    inline void		reset()		{ cdp_.toPreStart(); }
    inline BinID	binID() const	{ return cd_.binID( cdp_ ); }

    const CubeData&	cd_;
    CubeDataPos		cdp_;

};


} // namespace PosInfo

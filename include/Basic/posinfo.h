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
#include "trckeyzsampling.h"
#include "indexinfo.h"
#include "binid.h"
#include "od_iosfwd.h"


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

/*!
\brief Position info for a line - in a 3D cube, that would be an inline.
Stored as (crossline-)number segments.
*/

mExpClass(Basic) LineData
{
public:
    typedef StepInterval<int>	Segment;

				LineData( int i ) : linenr_(i)	{}

    const int			linenr_;
    TypeSet<Segment>		segments_;

    int				size() const;
    int				segmentOf(int) const;
    Interval<int>		range() const;
    void			merge(const LineData&,bool incl);
				//!< incl=union, !incl=intersection

    int				nearestSegment(double) const;

};


/*!
\brief Position in a CubeData.
*/

mExpClass(Basic) CubeDataPos
{
public:
		CubeDataPos( int iln=0, int isn=0, int sidx=-1 )
		    : lidx_(iln), segnr_(isn), sidx_(sidx)	{}

    int		lidx_;
    int		segnr_;
    int		sidx_;

    void	toPreStart()	{ lidx_ = segnr_ = 0; sidx_ = -1; }
    void	toStart()	{ lidx_ = segnr_ = sidx_ = 0; }
    bool	isValid() const	{ return lidx_>=0 && segnr_>=0 && sidx_>=0; }

};


/*!
\brief Position info for an entire 3D cube.
The LineData's are not sorted.
*/

mExpClass(Basic) CubeData : public ManagedObjectSet<LineData>
{
public:

			CubeData()		{}
			CubeData( BinID start, BinID stop, BinID step )
						{ generate(start,stop,step); }
			CubeData( const CubeData& cd )
			    : ManagedObjectSet<LineData>()
						{ *this = cd; }
    CubeData&		operator =( const CubeData& cd )
			{ copyContents(cd); return *this; }

    int			totalSize() const;
    int			totalSizeInside(const TrcKeySampling& hrg) const;
			/*!<Only take positions that are inside hrg. */

    virtual int		indexOf(int inl,int* newidx=0) const;
			//!< newidx only filled if not null and -1 is returned
    bool		includes(const BinID&) const;
    bool		includes(int inl,int crl) const;
    void		getRanges(Interval<int>& inl,Interval<int>& crl) const;
    bool		getInlRange(StepInterval<int>&,bool sorted=true) const;
			//!< Returns whether fully regular.
    bool		getCrlRange(StepInterval<int>&,bool sorted=true) const;
			//!< Returns whether fully regular.

    bool		isValid(const CubeDataPos&) const;
    bool		isValid(const BinID&) const;
    bool		isValid(od_int64 globalidx,const TrcKeySampling&) const;

    bool		toNext(CubeDataPos&) const;
    BinID		binID(const CubeDataPos&) const;
    CubeDataPos		cubeDataPos(const BinID&) const;

    bool		haveInlStepInfo() const		{ return size() > 1; }
    bool		haveCrlStepInfo() const;
    bool		isFullyRectAndReg() const;
    bool		isCrlReversed() const;

    void		limitTo(const TrcKeySampling&);
    void		merge(const CubeData&,bool incl);
				//!< incl=union, !incl=intersection
    void		generate(BinID start,BinID stop,BinID step,
				 bool allowreversed=false);

    bool		read(od_istream&,bool asc);
    bool		write(od_ostream&,bool asc) const;

    int			indexOf( const LineData* l ) const override
			{ return ObjectSet<LineData>::indexOf( l ); }

protected:

    void		copyContents(const CubeData&);

};


/*!
\brief Position info for an entire 3D cube.
The LineData's are sorted.
*/

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

    int			indexOf(int inl,int* newidx=0) const override;
			//!< newidx only filled if not null and -1 is returned

    SortedCubeData&	add(LineData*);

    int			indexOf( const LineData* l ) const override
			{ return CubeData::indexOf( l ); }

protected:

    CubeData&		doAdd(LineData*) override;

};


/*!
\brief Fills CubeData object. Requires inline- and crossline-sorting.
*/

mExpClass(Basic) CubeDataFiller
{
public:
			CubeDataFiller(CubeData&);
			~CubeDataFiller();

    void		add(const BinID&);
    void		finish(); // automatically called on delete

protected:

    CubeData&		cd_;
    LineData*		ld_;
    LineData::Segment	seg_;
    int			prevcrl;

    void		initLine();
    void		finishLine();
    LineData*		findLine(int);

};


/*!
\brief Iterates through CubeData.
*/

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


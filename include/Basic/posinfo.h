#ifndef posinfo_h
#define posinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		2005 / Mar 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "manobjectset.h"
#include "typeset.h"
#include "cubesampling.h"
#include "indexinfo.h"
#include "position.h"
class BinID;


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

/*!\brief Position info for a line - in a 3D cube, that would be an inline.
	  Stored as (crossline-)number segments. */

mClass(Basic) LineData
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
    IndexInfo			getIndexInfo(double) const;

};


/*!\brief Position in a CubeData */

mClass(Basic) CubeDataPos
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


/*!\brief Position info for an entire 3D cube.

  The LineData's are not sorted.
 */

mClass(Basic) CubeData : public ManagedObjectSet<LineData>
{
public:

    			CubeData()
			    : ManagedObjectSet<LineData>(false)	{}
    			CubeData( BinID start, BinID stop, BinID step )
			    : ManagedObjectSet<LineData>(false)
						{ generate(start,stop,step); }
    			CubeData( const CubeData& cd )
			    : ManagedObjectSet<LineData>(false)	{ *this = cd; }
    CubeData&		operator =( const CubeData& cd )
			{ copyContents(cd); return *this; }

    int			totalSize() const;
    int			totalSizeInside(const HorSampling& hrg) const;
    			/*!<Only take positions that are inside hrg. */

    virtual int		indexOf(int inl,int* newidx=0) const;
    			//!< newidx only filled if not null and -1 is returned
    bool		includes(int inl,int crl) const;
    bool		getInlRange(StepInterval<int>&) const;
    			//!< Returns whether fully regular.
    bool		getCrlRange(StepInterval<int>&) const;
    			//!< Returns whether fully regular.

    bool		isValid(const CubeDataPos&) const;
    bool		toNext(CubeDataPos&) const;
    BinID		binID(const CubeDataPos&) const;
    CubeDataPos		cubeDataPos(const BinID&) const;

    bool		haveInlStepInfo() const		{ return size() > 1; }
    bool		haveCrlStepInfo() const;
    bool		isFullyRectAndReg() const;

    void		limitTo(const HorSampling&);
    void		merge(const CubeData&,bool incl);
    				//!< incl=union, !incl=intersection
    void		generate(BinID start,BinID stop,BinID step);

    bool		read(std::istream&,bool asc);
    bool		write(std::ostream&,bool asc) const;

    virtual int		indexOf( const LineData* l ) const
    			{ return ObjectSet<LineData>::indexOf( l ); }

protected:

    void		copyContents(const CubeData&);

};


/*!\brief Position info for an entire 3D cube.

  The LineData's are sorted.
 */

mClass(Basic) SortedCubeData : public CubeData
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

    virtual int		indexOf(int inl,int* newidx=0) const;
    			//!< newidx only filled if not null and -1 is returned

    virtual CubeData&	operator +=( LineData* ld )	{ return add( ld ); }
    SortedCubeData&	add(LineData*);

    virtual int		indexOf( const LineData* l ) const
    			{ return CubeData::indexOf( l ); }

};


/*!\brief Fills CubeData object. Requires inline- and crossline-sorting. */

mClass(Basic) CubeDataFiller
{
public:
    			CubeDataFiller(CubeData&);
    			~CubeDataFiller();

    void		add(const BinID&);
    void		finish();

protected:

    CubeData&		cd_;
    LineData*		ld_;
    LineData::Segment	seg_;
    int			prevcrl;

    void		initLine();
    void		finishLine();
    LineData*		findLine(int);

};


mClass(Basic) CubeDataIterator
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

#endif


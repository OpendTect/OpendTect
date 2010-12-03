#ifndef posinfo_h
#define posinfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		2005 / Mar 2008
 RCS:		$Id: posinfo.h,v 1.20 2010-12-03 20:41:27 cvskris Exp $
________________________________________________________________________

-*/

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

mClass LineData
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


/*!\brief Position info for an entire 3D cube. */

mClass CubeData : public ManagedObjectSet<LineData>
{
public:
    			CubeData()
			    : ManagedObjectSet<LineData>(false)	{}
    			CubeData(const BinID& start,const BinID& stop,
				 const BinID& step);
    			CubeData( const CubeData& cd )
			    : ManagedObjectSet<LineData>(false)
								{ *this = cd; }
    CubeData&		operator =(const CubeData&);

    int			totalSize() const;
    int			totalSizeInside(const HorSampling& hrg) const;
    			/*!<Only take positions that are inside hrg. */

    int			indexOf(int inl) const;
    bool		includes(int inl,int crl) const;
    bool		getInlRange(StepInterval<int>&) const;
    			//!< Returns whether fully regular.
    bool		getCrlRange(StepInterval<int>&) const;
    			//!< Returns whether fully regular.

    bool		haveInlStepInfo() const	{ return size() > 1; }
    bool		haveCrlStepInfo() const;
    bool		isFullyRectAndReg() const;

    CubeData&		add( LineData* ld )	{ *this += ld; return *this; }

    void		limitTo(const HorSampling&);
    void		merge(const CubeData&,bool incl);
    				//!< incl=union, !incl=intersection
    void		sort();

    bool		read(std::istream&,bool asc);
    bool		write(std::ostream&,bool asc) const;
};


/*!\brief Fills CubeData object. Requires inline- and crossline-sorting. */

mClass CubeDataFiller
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

};


/*!\brief Iterator for Cube Data*/

mClass CubeDataIterator
{
public:
    			CubeDataIterator(const CubeData& cd)
			    : cubedata_(cd)
			    , firstpos_(true)	{}

    bool		next(BinID&);
    void		reset()			{ firstpos_ = true; }

protected:

    bool		firstpos_;
    CubeData		cubedata_;
};

} // namespace PosInfo

#endif

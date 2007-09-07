#ifndef coltabindex_h
#define coltabindex_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltabindex.h,v 1.1 2007-09-07 11:21:01 cvsbert Exp $
________________________________________________________________________

-*/

#include "coltab.h"


namespace ColTab
{
class Mapper;
class Sequence;

/*!\brief Color table scaling pars: automatic with clipping or explicit */

class IndexedLookUpTable
{
public:

			IndexedLookUpTable(const Sequence&,const Mapper&,
					   int nrcols=0,bool segmentised=false);
			~IndexedLookUpTable();
    void		update();
    			//!< If sequence, mapper, nr cols, or segmenting changed

    Color		color(float) const;

    void		setNrCols( int n )		{ nrcols_ = n; }
    void		setSegmentised( bool yn )	{ segmentised_ = yn; }

protected:

    const Sequence&	seq_;
    const Mapper&	mapper_;

    int			nrcols_;
    bool		segmentised_;

};

} // namespace ColTab

#endif

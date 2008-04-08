#ifndef coltabindex_h
#define coltabindex_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id: coltabindex.h,v 1.4 2008-04-08 03:27:42 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "coltab.h"
#include "sets.h"


namespace ColTab
{
class Mapper;
class Sequence;

/*!\brief Looks up color for certain value. Keeps a pre-calc list of colors.
 
  Note that sequence and mapper need to stay alive; no copy is made.
 
 */

class IndexedLookUpTable
{
public:

			IndexedLookUpTable(const Sequence&,int nrcols=0,
					   const Mapper* m=0);

    void		update();
    			//!< Call when sequence, mapper, or nr cols changed

    inline Color	color( float v ) const
			{ return colorForIndex( indexForValue(v) ); }
    int			indexForValue(float) const;
    inline Color	colorForIndex( int idx ) const	{ return cols_[idx]; }

    void		setMapper( const Mapper* m )	{ mapper_ = m; }
    void		setNrCols( int n )		{ nrcols_ = n; }
    int			nrCols()			{ return nrcols_; }

protected:

    const Sequence&	seq_;
    const Mapper*	mapper_;
    int			nrcols_;
    TypeSet<Color>	cols_;

    friend class	Indexer;

};

} // namespace ColTab

#endif

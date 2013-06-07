#ifndef coltabindex_h
#define coltabindex_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "coltab.h"
#include "typeset.h"


namespace ColTab
{
class Mapper;
class Sequence;

/*!\brief Looks up color for certain value. Keeps a pre-calc list of colors.
 
  Note that sequence and mapper need to stay alive; no copy is made.
 
 */

mClass IndexedLookUpTable
{
public:

			IndexedLookUpTable(const Sequence&,int nrcols=0,
					   const Mapper* m=0);

    void		update();
    			//!< Call when sequence, mapper, or nr cols changed

    inline Color	color( float v ) const
			{ return colorForIndex( indexForValue(v) ); }
    int			indexForValue(float) const;
    Color		colorForIndex(int) const;

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

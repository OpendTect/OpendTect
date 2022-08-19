#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "coltab.h"
#include "typeset.h"
#include "uistring.h"


namespace ColTab
{
class Mapper;
class Sequence;

/*!\brief Looks up color for certain value. Keeps a pre-calc list of colors.
 
  Note that sequence and mapper need to stay alive; no copy is made.
 
 */

mExpClass(General) IndexedLookUpTable
{ mODTextTranslationClass(IndexedLookUpTable);
public:

			IndexedLookUpTable(const Sequence&,int nrcols=0,
					   const Mapper* m=0);

    void		update();
			//!< Call when sequence, mapper, or nr cols changed

    inline OD::Color	color( float v ) const
			{ return colorForIndex( indexForValue(v) ); }
    int			indexForValue(float) const;
    OD::Color		colorForIndex(int) const;

    void		setMapper( const Mapper* m )	{ mapper_ = m; }
    void		setNrCols( int n )		{ nrcols_ = n; }
    int			nrCols()			{ return nrcols_; }

protected:

    const Sequence&	seq_;
    const Mapper*	mapper_;
    int			nrcols_;
    TypeSet<OD::Color>	cols_;

    friend class	Indexer;

};

} // namespace ColTab

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		April 2006
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emsurface.h"
#include "ranges.h"

typedef od_int64 GeomPosID;

namespace Geometry { class RowColSurface; }

namespace EM
{

/*!
\brief RowCol iterator
*/

mExpClass(EarthModel) RowColIterator : public EMObjectIterator
{
public:
    			RowColIterator(const Surface&,const SectionID&,
				       const TrcKeyZSampling* =0);
    			RowColIterator(const Surface&,const SectionID&,
				       const StepInterval<int> rowbnd,
				       const StepInterval<int> colbnd);
			~RowColIterator();

    PosID		next() override;
    PosID		fromIndex( int idx ) const;
    int			maxIndex() const;
    int			maximumSize() const override;
    int			maximumSize(const SectionID&) const;

protected:
    bool		initSection();
    bool		nextSection();
    void		fillPosIDs();

    RowCol				rc_;
    SectionID				sid_;
    const Geometry::RowColSurface*	cursection_;
    StepInterval<int>			rowrg_;
    StepInterval<int>			colrg_;
    bool				allsids_;
    const Surface&			surf_;

    const TrcKeyZSampling*			csbound_;
    const StepInterval<int>		rowbound_;
    const StepInterval<int>		colbound_;
    const bool				rowcolbounded_;
    Coord3				pos_;
    BinID				bid_;

    ObjectSet<TypeSet<GeomPosID>>	posids_;
};

} // namespace EM



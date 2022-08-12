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
			RowColIterator(const Surface&,
					const TrcKeyZSampling* =nullptr);
			RowColIterator(const Surface&,
				       const StepInterval<int> rowbnd,
				       const StepInterval<int> colbnd);
    mDeprecated("Use without SectionID")
			RowColIterator(const Surface& s,const SectionID&,
				       const TrcKeyZSampling* t=0)
			    : RowColIterator(s,t)		{}
    mDeprecated("Use without SectionID")
			RowColIterator(const Surface& s,const SectionID&,
				       const StepInterval<int> rowbnd,
				       const StepInterval<int> colbnd)
			    : RowColIterator(s,rowbnd,colbnd)		{}
			~RowColIterator();

    PosID		next() override;
    PosID		fromIndex( int idx ) const;
    int			maxIndex() const;
    int			maximumSize() const override;

    mDeprecated("Use without SectionID")
    int			maximumSize(const SectionID&) const
			{ return maximumSize(); }

protected:
    bool		initSection();
    void		fillPosIDs();

    const Surface&			surf_;
    RowCol				rc_;
    const Geometry::RowColSurface*	cursection_	= nullptr;
    StepInterval<int>			rowrg_;
    StepInterval<int>			colrg_;

    const TrcKeyZSampling*		csbound_	= nullptr;
    const StepInterval<int>		rowbound_;
    const StepInterval<int>		colbound_;
    const bool				rowcolbounded_;
    Coord3				pos_;
    BinID				bid_;

    ObjectSet<TypeSet<GeomPosID>>	posids_;
};

} // namespace EM

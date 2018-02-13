#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		April 2006
________________________________________________________________________


-*/

#include "emsurface.h"
#include "ranges.h"

namespace Geometry { class RowColSurface; }

namespace EM
{

/*!
\brief RowCol iterator
*/

mExpClass(EarthModel) RowColIterator : public ObjectIterator
{
public:
			RowColIterator(const Surface&,
				       const TrcKeyZSampling* =0);
			RowColIterator(const Surface&,
				       const StepInterval<int> rowbnd,
				       const StepInterval<int> colbnd);
    PosID		next();
    int			maximumSize() const;

protected:

    bool		init();

    RowCol				rc_;
    const Geometry::RowColSurface*	surfgeom_;
    StepInterval<int>			rowrg_;
    StepInterval<int>			colrg_;
    const Surface&			surf_;

    const TrcKeyZSampling*		csbound_;
    const StepInterval<int>		rowbound_;
    const StepInterval<int>		colbound_;
    const bool				rowcolbounded_;
    Coord3				pos_;
    BinID				bid_;
};

} // namespace EM

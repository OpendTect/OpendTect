#ifndef emrowcoliterator_h
#define emrowcoliterator_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id$
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "emsurface.h"
#include "ranges.h"

namespace Geometry { class RowColSurface; }

namespace EM
{

/*!
\ingroup EarthModel
\brief RowCol iterator
*/

mClass(EarthModel) RowColIterator : public EMObjectIterator
{
public:
    			RowColIterator(const Surface&,const SectionID&,
				       const CubeSampling* =0);
    			RowColIterator(const Surface&,const SectionID&,
				       const StepInterval<int> rowbnd,
				       const StepInterval<int> colbnd);
    PosID		next();
    int			maximumSize() const;
    int			maximumSize(const SectionID&) const;

protected:
    bool		initSection();
    bool		nextSection();

    RowCol				rc_;
    SectionID				sid_;
    const Geometry::RowColSurface*	cursection_;
    StepInterval<int>			rowrg_;
    StepInterval<int>			colrg_;
    bool				allsids_;
    const Surface&			surf_;
    
    const CubeSampling*			csbound_;
    const StepInterval<int>		rowbound_;
    const StepInterval<int>		colbound_;
    const bool				rowcolbounded_;
    Coord3				pos_;
    BinID				bid_;
};

}; // Namespace


#endif


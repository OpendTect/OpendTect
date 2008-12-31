#ifndef emrowcoliterator_h
#define emrowcoliterator_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id: emrowcoliterator.h,v 1.3 2008-12-31 09:08:40 cvsranojay Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "ranges.h"

namespace Geometry { class RowColSurface; }

namespace EM
{

mClass RowColIterator : public EMObjectIterator
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

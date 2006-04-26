#ifndef emrowcoliterator_h
#define emrowcoliterator_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		April 2006
 RCS:		$Id: emrowcoliterator.h,v 1.1 2006-04-26 21:15:40 cvskris Exp $
________________________________________________________________________


-*/

#include "emsurface.h"
#include "ranges.h"

namespace Geometry { class RowColSurface; }

namespace EM
{

class RowColIterator : public EMObjectIterator
{
public:
    			RowColIterator(const Surface&,const SectionID&);
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
};




}; // Namespace


#endif

#ifndef sectionselector_h
#define sectionselector_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionselector.h,v 1.2 2005-01-18 12:57:10 kristofer Exp $
________________________________________________________________________

-*/

#include "basictask.h"
#include "bufstring.h"
#include "emposid.h"
#include "sets.h"

namespace EM
{
    class EMObject;
};


namespace MPE
{

class TrackPlane;

class SectionSourceSelector : public BasicTask
{
public:
    			SectionSourceSelector( const EM::EMObject&,
					       const EM::SectionID& sid = -1 )
			    : sectionid( sid ) {}

    virtual void	reset() { selpos.erase(); }

    virtual void	setTrackPlane( const MPE::TrackPlane& ) {}

    int			nextStep() { return 0; }
    const char*		errMsg() const  { return errmsg[0] ? errmsg : 0; }

    const TypeSet<EM::SubID>&	selectedPositions() const { return selpos;}

protected:
    EM::SectionID	sectionid;
    TypeSet<EM::SubID>	selpos;
    BufferString	errmsg;
};

};

#endif


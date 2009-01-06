#ifndef sectionselector_h
#define sectionselector_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionselector.h,v 1.8 2009-01-06 10:48:18 cvsranojay Exp $
________________________________________________________________________

-*/

#include "task.h"
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

mClass SectionSourceSelector : public SequentialTask
{
public:
    				SectionSourceSelector(
					const EM::SectionID& sid = -1);

    EM::SectionID		sectionID() const;
    virtual void		reset();

    virtual void		setTrackPlane(const MPE::TrackPlane&);

    int				nextStep();
    const char*			errMsg() const;

    virtual void		fillPar(IOPar&) const {}
    virtual bool		usePar(const IOPar&) { return true; }

    const TypeSet<EM::SubID>&	selectedPositions() const;

protected:
    EM::SectionID		sectionid_;
    TypeSet<EM::SubID>		selpos_;
    BufferString		errmsg_;
};

};

#endif


#ifndef sectionselector_h
#define sectionselector_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 Contents:      Ranges
 RCS:           $Id: sectionselector.h,v 1.9 2009/07/22 16:01:16 cvsbert Exp $
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


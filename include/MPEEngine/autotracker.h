#ifndef autotracker_h
#define autotracker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: autotracker.h,v 1.2 2005-08-20 19:04:04 cvskris Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "executor.h"
#include "sets.h"

namespace EM { class EMObject; };
namespace Attrib { class SelSpec; }

namespace MPE
{

class SectionTracker;
class SectionAdjuster;
class SectionExtender;
class EMTracker;

class AutoTracker : public Executor
{
public:
				AutoTracker( EMTracker&, const EM::SectionID& );
    int				nextStep();
    int				nrDone() const { return nrdone; }
    int				totalNr() const { return totalnr; }

protected:
    int				nrdone;
    int				totalnr;

    const EM::SectionID		sid;
    TypeSet<EM::SubID>		blacklist;
    TypeSet<int>		blacklistscore;
    TypeSet<EM::SubID>		currentseeds;
    EMTracker&			emtracker;
    EM::EMObject&		emobject;
    SectionTracker*		sectiontracker;
    SectionExtender*		extender;
    SectionAdjuster*		adjuster;
};



}; // namespace MPE

#endif


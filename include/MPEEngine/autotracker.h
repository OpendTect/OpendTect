#ifndef autotracker_h
#define autotracker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: autotracker.h,v 1.1 2005-08-17 20:46:22 cvskris Exp $
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

protected:
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


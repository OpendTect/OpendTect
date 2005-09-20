#ifndef autotracker_h
#define autotracker_h
                                                                                
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: autotracker.h,v 1.3 2005-09-20 09:46:15 cvsduntao Exp $
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
    void			setNewSeeds( const TypeSet<EM::PosID>& );
    int				nextStep();
    int				nrDone() const { return nrdone; }
    int				totalNr() const { return totalnr; }

protected:
    bool			addSeed( const EM::PosID& );
    int				nrdone;
    int				totalnr;

    const EM::SectionID		sectionid;
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


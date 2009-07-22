#ifndef autotracker_h
#define autotracker_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id: autotracker.h,v 1.9 2009-07-22 16:01:16 cvsbert Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "executor.h"
#include "sets.h"
#include "cubesampling.h"

namespace EM { class EMObject; };
namespace Attrib { class SelSpec; }
namespace Geometry { class Element; }

namespace MPE
{

class SectionTracker;
class SectionAdjuster;
class SectionExtender;
class EMTracker;

mClass AutoTracker : public Executor
{
public:
				AutoTracker(EMTracker&,const EM::SectionID&);
				~AutoTracker();
    void			setNewSeeds(const TypeSet<EM::PosID>&);
    int				nextStep();
    void			setTrackBoundary(const CubeSampling&);
    void			unsetTrackBoundary();
    od_int64			nrDone() const		{ return nrdone_; }
    od_int64			totalNr() const		{ return totalnr_; }

protected:
    bool			addSeed(const EM::PosID&);
    void			manageCBbuffer(bool block);
    void			reCalculateTtalNr();
    int				nrdone_;
    int				totalnr_;
    int				nrflushes_;
    int				flushcntr_;
    int 			stepcount_;

    const EM::SectionID		sectionid_;
    TypeSet<EM::SubID>		blacklist_;
    TypeSet<int>		blacklistscore_;
    TypeSet<EM::SubID>		currentseeds_;
    EM::EMObject&		emobject_;
    SectionTracker*		sectiontracker_;
    SectionExtender*		extender_;
    SectionAdjuster*		adjuster_;
    Geometry::Element*          geomelem_;
};



}; // namespace MPE

#endif


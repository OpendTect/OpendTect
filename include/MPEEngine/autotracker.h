#ifndef autotracker_h
#define autotracker_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          23-10-1996
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emposid.h"
#include "executor.h"
#include "sets.h"
#include "sortedtable.h"
#include "thread.h"
#include "trckeyzsampling.h"

namespace EM { class EMObject; }
namespace Geometry { class Element; }
namespace Threads { class WorkManager; }
template <class T> class Array2D;
class TrcKeyValue;

/*!\brief %MPE stands for Model, Predict, Edit. Contains tracking and editing
	  functions.*/

namespace MPE
{

class SectionTracker;
class SectionAdjuster;
class SectionExtender;
class EMTracker;

/*!
\brief Executor to auto track.
*/


mExpClass(MPEEngine) HorizonTrackerMgr
{
friend class HorizonTracker;
public:
			HorizonTrackerMgr(EMTracker&);
			~HorizonTrackerMgr();

    void		setSeeds(const TypeSet<TrcKey>&);
    void		startFromSeeds();

protected:
    void		addTask(const TrcKeyValue&,const TrcKeyValue&);

    int				queueid_;

    EMTracker&			tracker_;
    TypeSet<TrcKey>		seeds_;
    Threads::WorkManager&	twm_;

    int				nrdone_;
    Threads::Lock		addlock_;
};



mExpClass(MPEEngine) AutoTracker : public Executor
{ mODTextTranslationClass(AutoTracker)
public:
				AutoTracker(EMTracker&,const EM::SectionID&);
				~AutoTracker();

    void			setNewSeeds(const TypeSet<EM::PosID>&);
    int				nextStep();
    void			setTrackBoundary(const TrcKeyZSampling&);
    void			unsetTrackBoundary();
    od_int64			nrDone() const		{ return nrdone_; }
    od_int64			totalNr() const		{ return totalnr_; }

    virtual uiString		uiMessage() const;

protected:
    bool			addSeed(const EM::PosID&);
    void			manageCBbuffer(bool block);
    void			reCalculateTotalNr();
    int				nrdone_;
    int				totalnr_;
    int				nrflushes_;
    int				flushcntr_;
    int 			stepcntallowedvar_;
    int				stepcntapmtthesld_;

    bool			burstalertactive_;

    const EM::SectionID		sectionid_;
    SortedTable<EM::SubID,char>	blacklist_;
    TypeSet<EM::SubID>		currentseeds_;
    EM::EMObject&		emobject_;
    SectionTracker*		sectiontracker_;
    SectionExtender*		extender_;
    SectionAdjuster*		adjuster_;
    Geometry::Element*		geomelem_;

    Array2D<float>*		horizon3dundoinfo_;
    RowCol			horizon3dundoorigin_;

    uiString			execmsg_;
};

} // namespace MPE

#endif


#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
class EMTracker;

/*!
\brief Executor to auto track.
*/


mExpClass(MPEEngine) HorizonTrackerMgr : public CallBacker
{
friend class TrackerTask;
public:
			HorizonTrackerMgr(EMTracker&);
			~HorizonTrackerMgr();

    void		setSeeds(const TypeSet<TrcKey>&);
    void		startFromSeeds();
    void		stop();
    bool		hasTasks() const;

    SectionTracker*	getFreeSectionTracker();
    void		freeSectionTracker(const SectionTracker*);

    Notifier<HorizonTrackerMgr>		finished;

protected:
    void		addTask(const TrcKeyValue&,const TrcKeyValue&,
				int seedid);

    mDeprecatedDef
    void		addTask( const TrcKeyValue& seed,
				 const TrcKeyValue& source )
			{ addTask( seed, source, 1 ); }

    void		taskFinished(CallBacker*);
    void		updateCB(CallBacker*);
    int			queueid_;

    EMTracker&			tracker_;
    ObjectSet<SectionTracker>	sectiontrackers_;
    BoolTypeSet			trackerinuse_;
    Array2D<float>*		horizon3dundoinfo_;
    RowCol			horizon3dundoorigin_;
    void			addUndoEvent();

    TypeSet<TrcKey>		seeds_;
    Threads::WorkManager&	twm_;

    Threads::Atomic<int>	nrdone_;
    Threads::Atomic<int>	nrtodo_;
    Threads::Atomic<int>	tasknr_;

    Threads::Lock		addlock_;
    Threads::Lock		finishlock_;
    Threads::Lock		getfreestlock_;
};

} // namespace MPE

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          23-10-1996
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "executor.h"
#include "rowcol.h"
#include "sets.h"
#include "sortedtable.h"
#include "thread.h"

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
				const int seedid);
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

    Threads::Lock		tasklock_;
    Threads::Lock		getfreestlock_;
};

} // namespace MPE

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "mpeenginemod.h"

#include "callback.h"
#include "rowcol.h"
#include "trckeyzsampling.h"

class TrcKeyValue;
namespace Threads { class WorkManager; }
template <class T> class Array2D;

/*!\brief %MPE stands for Model, Predict, Edit. Contains tracking and editing
	  functions.*/

namespace MPE
{

class SectionTracker;
class EMTracker;

/*!
\brief Executor to auto track.
*/


mClass(MPEEngine) HorizonTrackerMgr : public CallBacker
{
friend class TrackerTask;
public:
				HorizonTrackerMgr(EMTracker&);
				~HorizonTrackerMgr();

    void			init();
    void			startFromSeeds(const TypeSet<TrcKey>&);
    void			updateFlatCubesContainer(const TrcKeyZSampling&,
							 bool addremove);
				/*!< add = true, remove = false. */
    void			stop();
    bool			hasTasks() const;

    ConstRefMan<EMTracker>	getTracker() const;
    RefMan<EMTracker>		getTracker();

    SectionTracker*		getFreeSectionTracker();
    void			freeSectionTracker(const SectionTracker*);

    Notifier<HorizonTrackerMgr> finished;

protected:
    void			addTask(const TrcKeyValue&,const TrcKeyValue&,
					int seedid);
    mDeprecatedDef
    void			addTask( const TrcKeyValue& seed,
					 const TrcKeyValue& source )
				{ addTask( seed, source, 1 ); }

    void			taskFinished(CallBacker*);
    void			updateCB(CallBacker*);

    mStruct(MPEEngine) FlatCubeInfo
    {
				FlatCubeInfo()
				{
				    flatcs_.setEmpty();
				}

	TrcKeyZSampling		flatcs_;
	int			nrseeds_	= 1;
    };

    WeakPtr<EMTracker>		tracker_;
    ObjectSet<FlatCubeInfo>*	flatcubes_		= nullptr;
    ObjectSet<SectionTracker>	sectiontrackers_;
    BoolTypeSet			trackerinuse_;
    Array2D<float>*		horizon3dundoinfo_	= nullptr;
    RowCol			horizon3dundoorigin_;
    void			addUndoEvent();

    TypeSet<TrcKey>		seeds_;
    Threads::WorkManager&	twm_;
    int				queueid_;

    Threads::Atomic<int>	nrdone_			= 0;
    Threads::Atomic<int>	nrtodo_			= 0;
    Threads::Atomic<int>	tasknr_			= 0;

    Threads::Lock		addlock_;
    Threads::Lock		finishlock_;
    Threads::Lock		getfreestlock_;
};

} // namespace MPE

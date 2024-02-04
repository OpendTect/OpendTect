#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"

#include "callback.h"
#include "typeset.h"

#include <QHash>
#include <QString>

class Timer;

namespace Threads { class Lock; class Locker; }
namespace Well { class LoadReqs; }

mExpClass(Well) WellUpdateQueue : public CallBacker
{
public:
			~WellUpdateQueue();
			mOD_DisableCopy(WellUpdateQueue)

    static WellUpdateQueue&	WUQ();

    void		enqueue(const std::pair<MultiID,Well::LoadReqs>&);
    void		enqueueWellsToBeDeleted(const MultiID&);
    bool		dequeue(std::pair<MultiID,Well::LoadReqs>&);
    bool		dequeueWellsToBeDeleted(MultiID&);
    int			size() const;
    bool		isEmpty() const;
    bool		hasWellsToBeDeleted() const;

    Notifier<WellUpdateQueue>	canupdate;
private:

			WellUpdateQueue();
    void		addTimerCB(CallBacker*);
    void		checkUpdateStatus(CallBacker*);

    TypeSet<int>		updobjidqueue_;
    TypeSet<Well::LoadReqs>	updreqsqueue_;
    TypeSet<int>		delobjidqueue_;
    Timer*			timer_		    = nullptr;
    mutable Threads::Lock	lock_;
    const int			wellgrpid_;
};


mExpClass(Well) WellFileList : public CallBacker
{
public:
			WellFileList();
			~WellFileList();

    WellFileList&	operator =(const WellFileList&);
    void		addLoadedWells(const BufferString&,const MultiID&);
    void		removeFromLoadedWells(const BufferString&);
    int			nrFiles() const;
    int			nrWells() const;
    bool		hasLoadedWellInfo() const;

    void		catchChange() ;

    const QHash<const QString,QString>& loadedNameIDPairs() const;
    const QHash<const QString,QString>& allIdsNamePairs() const;
    const QHash<const QString,QString>& fileList() const;

    Notifier<WellFileList>  triggerupdate;

private:

    bool		catchChangedWells(const WellFileList&);
    bool		catchChangedFiles(const WellFileList&);
    bool		getDeletedWells(const WellFileList&);
    bool		getRenamedWells(const WellFileList&);
    bool		getDeletedFiles(const WellFileList&);
    bool		getAddedFiles(const WellFileList&);
    bool		getChangedFiles(const WellFileList&);
    void		updateWellQueue(const QString& fnm, bool reqall=false);

    QHash<const QString, QString>	filelist_;
    QHash<const QString, QString>	allidsnmpair_;
    QHash<const QString, QString>	loadednmidpair_;
    mutable Threads::Lock		lock_;
};

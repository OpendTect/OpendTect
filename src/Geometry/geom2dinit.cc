/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "geom2dinit.h"

#include "geom2dintersections.h"
#include "odruncontext.h"
#include "survgeom.h"
#include "survinfo.h"
#include "threadwork.h"


Geom2DInit::Geom2DInit()
    : queueid_(Threads::WorkManager::twm().addQueue(
			Threads::WorkManager::SingleThread,"Geom2DInit"))
    , lock_(false)
{
    mAttachCB( SurveyInfo::rootDirChanged(), Geom2DInit::rootDirChangedCB );
    mAttachCB( Survey::GM().closing, Geom2DInit::closeQueueCB );
}


void Geom2DInit::start()
{
    continue_ = true;
    Survey::GMAdmin().ensureSIPresent();
    Threads::Locker locker( lock_ );
    auto& twm = Threads::WorkManager::twm();
    if ( queueid_ >= 0 )
	twm.emptyQueue( queueid_, true );

    const CallBack cb = mCB(this,Geom2DInit,readGeomCB);
    CallBack cbdone = mCB(this,Geom2DInit,geomReadyCB);
    twm.addWork( Threads::Work(cb), &cbdone, queueid_, false, false, true );
}


Geom2DInit::~Geom2DInit()
{
    detachAllNotifiers();
}


Geom2DInit& Geom2DInit::getInstance()
{
    static PtrMan<Geom2DInit> mgr = new Geom2DInit;
    return *mgr;
}


void Geom2DInit::readGeomCB( CallBacker* )
{
    Survey::GMAdmin().fillGeometries( nullptr );
}


void Geom2DInit::geomReadyCB( CallBacker* retcb )
{
    auto& twm = Threads::WorkManager::twm();
    const bool res = twm.getWorkExitStatus( retcb );
    if ( !res || queueid_ < 0 || !continue_ )
	return;

    const CallBack cb = mCB(this,Geom2DInit,computeBendpointsCB);
    CallBack cbdone = mCB(this,Geom2DInit,bendPointsReadyCB);
    twm.addWork( Threads::Work(cb), &cbdone, queueid_, false, false, true );
}


void Geom2DInit::bendPointsReadyCB( CallBacker* retcb )
{
    auto& twm = Threads::WorkManager::twm();
    const bool res = twm.getWorkExitStatus( retcb );
    if ( !res || queueid_ < 0 || !continue_ )
	return;

    const CallBack cb = mCB(this,Geom2DInit,computeIntersectionsCB);
    twm.addWork( Threads::Work(cb), nullptr, queueid_, false, false, true );
}


void Geom2DInit::computeBendpointsCB( CallBacker* )
{
    auto& l2dim = Line2DIntersectionManager::instanceAdmin();
    l2dim.computeBendpoints();
}


void Geom2DInit::computeIntersectionsCB( CallBacker* )
{
    auto& l2dim = Line2DIntersectionManager::instanceAdmin();
    l2dim.compute();
}


void Geom2DInit::rootDirChangedCB( CallBacker* )
{
    Threads::Locker locker( lock_ );
    continue_ = false;
}


void Geom2DInit::closeQueueCB( CallBacker* )
{
    Threads::Locker locker( lock_ );
    const int queueid = queueid_;
    queueid_ = -1;
    if ( queueid >= 0 )
	Threads::WorkManager::twm().removeQueue( queueid, true );
}


void Geom2DInit::Start()
{
    getInstance().start();
}

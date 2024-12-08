/*+
________________________________________________________________________

 Copyright:	(C) 1995-2024 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "geom2dinit.h"

#include "survgeom.h"
#include "geom2dintersections.h"
#include "odruncontext.h"
#include "threadwork.h"


Geom2DInit::Geom2DInit()
    : queueid_(Threads::WorkManager::twm().addQueue(
			Threads::WorkManager::SingleThread,"Geom2DInit"))
    , lock_(false)
{
    mAttachCB( Survey::GM().closing, Geom2DInit::closeQueueCB );
}


void Geom2DInit::start()
{
    Survey::GMAdmin().ensureSIPresent();
    if ( OD::InTestProgRunContext() || OD::InBatchProgRunContext() )
    {
	//TODO fix when this class works within a SurveyChanger for loop
	Geom2DInit::readGeomCB( nullptr );
	Geom2DInit::computeBendpointsCB( nullptr );
	Geom2DInit::computeIntersectionsCB( nullptr );
	return;
    }

    Threads::Locker locker( lock_ );
    auto& twm = Threads::WorkManager::twm();
    if ( queueid_ >= 0 )
	twm.emptyQueue( queueid_, true );

    const CallBack cb1 = mCB(this,Geom2DInit,readGeomCB);
    const CallBack cb2 = mCB(this,Geom2DInit,computeBendpointsCB);
    const CallBack cb3 = mCB(this,Geom2DInit,computeIntersectionsCB);
    twm.addWork( Threads::Work(cb1), nullptr, queueid_, false, false, true );
    twm.addWork( Threads::Work(cb2), nullptr, queueid_, false, false, true );
    twm.addWork( Threads::Work(cb3), nullptr, queueid_, false, false, true );
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


void Geom2DInit::closeQueueCB( CallBacker* )
{
    Threads::WorkManager::twm().removeQueue( queueid_, false );
    queueid_ = -1;
}


void Geom2DInit::Start()
{
    getInstance().start();
}

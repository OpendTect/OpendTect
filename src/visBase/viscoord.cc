/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Dec 2002
___________________________________________________________________

-*/

static const char* rcsID = "$Id: viscoord.cc,v 1.1 2002-12-20 16:30:27 kristofer Exp $";

#include "viscoord.h"

#include "executor.h"
#include "geomposidholder.h"

#include "Inventor/nodes/SoCoordinate3.h"

namespace visBase
{

class CoordinatesBuilder : public Executor
{
public:
    				CoordinatesBuilder( Coordinates*,
						    Geometry::PosIdHolder&,
				       		    bool connect );

    int				nextStep();
    int				totalNr() const { return iterator->totalNr(); }
    int				nrDone() const { return nrdone; }
    
protected:
    Coordinates*			coords;
    Geometry::PosIdHolder&		posidholder;
    int					nrdone;
    bool				connect;
    Geometry::PosIdHolderIterator*	iterator;
};

};


visBase::CoordinatesBuilder::CoordinatesBuilder( Coordinates* c,
	Geometry::PosIdHolder& posidholder_, bool connect_ ) 
    : Executor("Transfering Coordinates")
    , coords( c )
    , posidholder( posidholder_ )
    , nrdone( 0 )
    , connect( connect_ )
    , iterator( posidholder_.createIter() )
{
    if ( connect )
    {
	coords->posidholder = &posidholder;
	coords->posids.erase();
    }
}


int visBase::CoordinatesBuilder::nextStep()
{
    if ( !iterator ) return ErrorOccurred;

    unsigned long pid;
    if ( !iterator->nextId(pid) ) return Finished;

    Coord3 pos = posidholder.getPos( pid );
    SbVec3f sbvec( pos.x, pos.y, pos.z );
    coords->coords->point.set1Value( nrdone++, sbvec );
    if ( connect ) coords->posids+=pid;
    return MoreToDo;
}


mCreateFactoryEntry( visBase::Coordinates );

visBase::Coordinates::Coordinates()
    : posidholder( 0 )
    , coords( new SoCoordinate3 )
    , notifier( this )
{
    coords->ref();
}


Executor* visBase::Coordinates::setPositions(
		    Geometry::PosIdHolder& posidholder, bool connect )
{
    return new CoordinatesBuilder( this, posidholder, connect );
}


visBase::Coordinates::~Coordinates()
{
    coords->unref();
}


SoNode* visBase::Coordinates::getData() { return coords; }


void  visBase::Coordinates::notify( const CallBack& cb )
{ notifier.notify( cb ); }


void visBase::Coordinates::removeNotification( const CallBack& cb )
{ notifier.remove( cb ); }


void visBase::Coordinates::hanldePosIdHolderCh( CallBacker* cb )
{
    mCBCapsuleUnpack(const Geometry::PosIdHolderNotifierData&,ev,cb);

    if ( ev.event==Geometry::PosIdHolderNotifierData::PosChanged )
    {
	const int ownnr = posids.indexOf( ev.oldpid );
	if ( ownnr==-1 ) return;

	const Coord3 newcoord = posidholder->getPos( ev.oldpid );
	SbVec3f newpos( newcoord.x, newcoord.y, newcoord.z );
	coords->point.set1Value( ownnr, newpos );

	CoordinateMessage message;
	message.coordnr = ownnr;
	message.event = visBase::CoordinateMessage::ChangedPos;
	CBCapsule<const visBase::CoordinateMessage&> caps( message, this );
	notifier.trigger( message, this );
    }
    else if ( ev.event==Geometry::PosIdHolderNotifierData::PosIdChanged )
    {
	const int ownnr = posids.indexOf( ev.oldpid );
	if ( ownnr==-1 ) return;

	posids[ownnr] = ev.newpid;
    }
    else if (  ev.event==Geometry::PosIdHolderNotifierData::SenderDown )
    {
	posids.erase();
	posidholder = 0;
    }
}

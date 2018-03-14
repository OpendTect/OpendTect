/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		March 2016
________________________________________________________________________

-*/

#include "uibaseobject.h"
#include "uibody.h"

#include <limits.h>

static CallBackSet& cmdrecorders_ = *new CallBackSet;

static ObjectSet<const uiBaseObject> cmdrecstopperstack_;
static ObjectSet<const uiBaseObject> cmdrecstopperlist_;
static ObjectSet<const CallBacker> cmdrecstrikeoutlist_;

CmdRecStopper::CmdRecStopper( const uiBaseObject* obj )
{
    cmdrecstopperstack_.setNullAllowed();
    cmdrecstopperstack_.push( obj );

    if ( !cmdrecorders_.isEmpty() && !cmdrecstopperlist_.isPresent(obj) )
	cmdrecstopperlist_ += obj;
}

CmdRecStopper::~CmdRecStopper()
{ cmdrecstopperstack_.pop(); }

void CmdRecStopper::clearStopperList( const CallBacker* cmdrec )
{
    cmdrecstrikeoutlist_ -= cmdrec;
    if ( cmdrecstrikeoutlist_.isEmpty() )
    {
	cmdrecstopperlist_.erase();
	for ( int idx=0; idx<cmdrecorders_.size(); idx++ )
	    cmdrecstrikeoutlist_ += cmdrecorders_[idx].cbObj();
    }
}

bool CmdRecStopper::isInStopperList( const uiBaseObject* obj )
{ return cmdrecstopperlist_.isPresent(obj); }


uiBaseObject::uiBaseObject( const char* nm, uiBody* b )
    : NamedCallBacker(nm)
    , finaliseStart_(this)
    , finaliseDone_(this)
    , cmdrecrefnr_(0)
    , body_(b)
{
}


uiBaseObject::~uiBaseObject()
{
    sendDelNotif();
}


void uiBaseObject::finalise()
{ if ( body() ) body()->finalise(); }

void uiBaseObject::clear()
{ if ( body() ) body()->clear(); }

bool uiBaseObject::finalised() const
{ return body() ? body()->finalised() : false; }


int uiBaseObject::beginCmdRecEvent( const char* msg )
{ return beginCmdRecEvent( (od_int64)0, msg ); }


int uiBaseObject::beginCmdRecEvent( od_int64 id, const char* msg )
{
    if ( cmdrecorders_.isEmpty() ||
	(!id && cmdrecstopperstack_.isPresent(this)) )
	return -1;

    cmdrecrefnr_ = cmdrecrefnr_==INT_MAX ? 1 : cmdrecrefnr_+1;

    BufferString actstr;
    if ( id )
	actstr += toString( id );

    actstr += " Begin "; actstr += cmdrecrefnr_;
    actstr += " "; actstr += msg;
    CBCapsule<const char*> caps( actstr, this );
    cmdrecorders_.doCall( &caps );
    return cmdrecrefnr_;
}


const QWidget* uiBaseObject::getConstWidget(int idx) const
{ return const_cast<uiBaseObject*>(this)->getWidget(idx); }


void uiBaseObject::endCmdRecEvent( int refnr, const char* msg )
{ endCmdRecEvent( (od_int64)0, refnr, msg ); }


void uiBaseObject::endCmdRecEvent( od_int64 id, int refnr, const char* msg )
{
    if ( cmdrecorders_.isEmpty() ||
	(!id && cmdrecstopperstack_.isPresent(this)) )
	return;

    BufferString actstr;
    if ( id )
	actstr += toString( id );

    actstr += " End "; actstr += refnr;
    actstr += " "; actstr += msg;
    CBCapsule<const char*> caps( actstr, this );
    cmdrecorders_.doCall( &caps );
}


void uiBaseObject::removeCmdRecorder( const CallBack& cb )
{
    cmdrecorders_ -= cb;
    CmdRecStopper::clearStopperList( cb.cbObj() );
}


void uiBaseObject::addCmdRecorder( const CallBack& cb )
{
    cmdrecorders_ += cb;
    cmdrecstrikeoutlist_ += cb.cbObj();
}


QWidget* uiBaseObject::getWidget( int )
{ return 0; }


RowCol uiBaseObject::getWidgetOrigin( int ) const
{ return RowCol(0,0); }


RowCol uiBaseObject::getWidgetSpan( int ) const
{ return RowCol(1,1); }


int uiBaseObject::getNrRows() const
{
    return getNrRowCols( true );
}


int uiBaseObject::getNrCols() const
{
    return getNrRowCols( false );
}


int uiBaseObject::getNrRowCols( bool row ) const
{
    int dim = row ? 0 : 1;

    int res = 0;
    for ( int idx=0; idx<getNrWidgets(); idx++ )
    {
	const RowCol origin = getWidgetOrigin( idx );
	const RowCol span = getWidgetSpan( idx );

	const int nrrows = origin[dim]+span[dim];
	if ( nrrows>res )
	    res = nrrows;
    }

    return res;
}

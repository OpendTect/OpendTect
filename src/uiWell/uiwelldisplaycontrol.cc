/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplaycontrol.cc,v 1.15 2010-08-20 15:02:27 cvsbruno Exp $";


#include "uiwelldisplaycontrol.h"

#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uiwelllogdisplay.h"

#include "mouseevent.h"
#include "keyboardevent.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "wellmarker.h"


uiWellDisplayControl::uiWellDisplayControl( uiWellLogDisplay& l ) 
    : CallBacker(CallBacker::CallBacker())
    , selmarker_(0)	
    , seldisp_(0)			
    , lastselmarker_(0)
    , ismousedown_(false)		       
    , posChanged(this)
    , mousePressed(this)
    , mouseReleased(this)
{
    addLogDisplay( l );
}    


void uiWellDisplayControl::addLogDisplay( uiWellLogDisplay& disp )
{
    logdisps_ += &disp;
    disp.scene().setMouseEventActive( true );
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    meh.movement.notify( mCB( this, uiWellDisplayControl, setSelLogDispCB ) );
    meh.movement.notify( mCB( this, uiWellDisplayControl, setSelMarkerCB ) );
    meh.movement.notify( mCB( this, uiWellDisplayControl, mouseMovedCB ) );
    meh.buttonReleased.notify( mCB(this,uiWellDisplayControl,mouseReleasedCB) );
    meh.buttonPressed.notify( mCB(this,uiWellDisplayControl,mousePressedCB) );
}


void uiWellDisplayControl::removeLogDisplay( uiWellLogDisplay& disp )
{
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    meh.movement.remove( mCB( this, uiWellDisplayControl, mouseMovedCB ) );
    meh.movement.remove( mCB( this, uiWellDisplayControl, setSelMarkerCB ) );
    meh.movement.remove( mCB( this, uiWellDisplayControl, setSelLogDispCB ) );
    meh.buttonPressed.remove( mCB(this,uiWellDisplayControl,mousePressedCB) );
    meh.buttonReleased.remove( mCB(this,uiWellDisplayControl,mouseReleasedCB) );
    logdisps_ -= &disp;
}


MouseEventHandler& uiWellDisplayControl::mouseEventHandler( int dispidx )
    { return logdisps_[dispidx]->scene().getMouseEventHandler(); }

KeyboardEventHandler& uiWellDisplayControl::keyboardEventHandler( int dispidx )
    { return logdisps_[dispidx]->getKeyboardEventHandler(); }

MouseEventHandler* uiWellDisplayControl::mouseEventHandler()
    { return seldisp_ ? &seldisp_->scene().getMouseEventHandler() : 0; }

KeyboardEventHandler* uiWellDisplayControl::keyboardEventHandler()
    { return seldisp_ ? &seldisp_->getKeyboardEventHandler() : 0; }


void uiWellDisplayControl::mouseMovedCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) 
	return;
    if ( !mevh->hasEvent() || mevh->isHandled() ) 
	return;

    BufferString info;
    getPosInfo( info );
    CBCapsule<BufferString> caps( info, this );
    posChanged.trigger( &caps );

    setSelMarkerCB( cb );
    if ( selmarker_ != lastselmarker_ )  
    {
	for ( int idx=0; idx<logdisps_.size(); idx++ )
	    logdisps_[idx]->highlightMarkerItem( selmarker_ );
	lastselmarker_ = selmarker_;
    }
}


float uiWellDisplayControl::mousePos() const
{
    return  seldisp_ ? seldisp_->mousePos() : 0;
}


void uiWellDisplayControl::getPosInfo( BufferString& info ) const
{
    info.setEmpty();
    if ( !seldisp_ ) return;
    const Well::Well2DDispData& data = seldisp_->data();
    float pos = mousePos();
    if ( data.markers_ )
    {
	for ( int idx=0; idx<data.markers_->size(); idx++ )
	{
	    const Well::Marker* marker = (*data.markers_)[idx];
	    if ( !marker ) continue;
		float markerpos = marker->dah();
	    if ( data.zistime_ && data.d2tm_ )
		markerpos = data.d2tm_->getTime( markerpos )*1000;
	    if ( !mIsEqual(markerpos,pos,5) )
		continue;
	    info += " Marker: ";
	    info += marker->name();
	    info += "  ";
	    break;
	}
    }
    float time = 0, dah = 0;
    if ( data.zistime_ )
    {
	time = pos;
	if ( data.d2tm_ && data.d2tm_->size() >= 1 )
	    dah = data.d2tm_->getDah( pos*0.001 );
    }
    else
    {
	dah = pos;
	if ( data.d2tm_ )
	    time = data.d2tm_->getTime( pos )*1000;
    }
    info += "  MD: ";
    info += toString( mNINT(dah) );
    info += "  Time: ";
    info += toString( mNINT(time) );

#define mGetLogPar( ld )\
    info += "   ";\
    info += ld.wl_->name();\
    info += ":";\
    info += toString( ld.wl_->getValue( dah ) );\
    info += ld.wl_->unitMeasLabel();\

    const uiWellLogDisplay::LogData& ldata1 = seldisp_->logData(true);
    const uiWellLogDisplay::LogData& ldata2 = seldisp_->logData(false);
    if ( ldata1.wl_ )
    { mGetLogPar( ldata1 ) }
    if ( ldata2.wl_ )
    { mGetLogPar( ldata2 ) }
}


void uiWellDisplayControl::setPosInfo( CallBacker* cb )
{
    info_.setEmpty();
    mCBCapsuleUnpack(BufferString,mesg,cb);
    if ( mesg.isEmpty() ) return;
    info_ += mesg;
}


void uiWellDisplayControl::mousePressedCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) 
	return;
    if ( !mevh->hasEvent() || mevh->isHandled() ) 
	return;

    ismousedown_ = true;
    mousePressed.trigger();
}


void uiWellDisplayControl::mouseReleasedCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) 
	return;
    if ( !mevh->hasEvent() || mevh->isHandled() ) 
	return;

    ismousedown_ = false;
    mouseReleased.trigger();
}


void uiWellDisplayControl::setSelLogDispCB( CallBacker* cb )
{
    seldisp_ = 0;
    if ( cb )
    {
	mDynamicCastGet(MouseEventHandler*,mevh,cb)
	for ( int idx=0; idx<logdisps_.size(); idx++)
	{
	    uiWellLogDisplay* ld = logdisps_[idx];
	    if ( &ld->getMouseEventHandler() == mevh ) 
	    {
		seldisp_ = ld;
		break;
	    }
	}
    }
}


void uiWellDisplayControl::setSelMarkerCB( CallBacker* ) 
{
    selmarker_ = 0;
    if ( !seldisp_ ) return;
    const MouseEvent& ev = seldisp_->getMouseEventHandler().event();
    int mousepos = ev.pos().y;
    for ( int idx=0; idx<seldisp_->markerItems().size(); idx++ )
    {
	if ( abs( seldisp_->markerItems()[idx]->itm_->getPos().y-mousepos )<2 )
	{
	    selmarker_ = &seldisp_->markerItems()[idx]->mrk_; 
	    break;
	}
    }
}




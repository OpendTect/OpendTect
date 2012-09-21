/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "uiwelldisplaycontrol.h"

#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uiwelllogdisplay.h"

#include "mouseevent.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "wellmarker.h"


uiWellDisplayControl::uiWellDisplayControl( uiWellDahDisplay& l ) 
    : selmarker_(0)	
    , seldisp_(0)			
    , lastselmarker_(0)
    , ismousedown_(false)		       
    , isctrlpressed_(false)
    , xpos_(0)			   
    , ypos_(0)			   
    , time_(0)					       
    , depth_(0)					       
    , posChanged(this)
    , mousePressed(this)
    , mouseReleased(this)
    , markerSel(this)			 
{
    addDahDisplay( l );
}    


uiWellDisplayControl::~uiWellDisplayControl()
{
    clear();
}


void uiWellDisplayControl::addDahDisplay( uiWellDahDisplay& disp )
{
    logdisps_ += &disp;
    disp.scene().setMouseEventActive( true );
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    meh.movement.notify( mCB( this, uiWellDisplayControl, setSelDahDisplay ) );
    meh.movement.notify( mCB( this, uiWellDisplayControl, setSelMarkerCB ) );
    meh.movement.notify( mCB( this, uiWellDisplayControl, mouseMovedCB ) );
    meh.buttonReleased.notify( mCB(this,uiWellDisplayControl,mouseReleasedCB) );
    meh.buttonPressed.notify( mCB(this,uiWellDisplayControl,mousePressedCB) );
}


void uiWellDisplayControl::removeDahDisplay( uiWellDahDisplay& disp )
{
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    meh.movement.remove( mCB( this, uiWellDisplayControl, mouseMovedCB ) );
    meh.movement.remove( mCB( this, uiWellDisplayControl, setSelMarkerCB ) );
    meh.movement.remove( mCB( this, uiWellDisplayControl, setSelDahDisplay ) );
    meh.buttonPressed.remove( mCB(this,uiWellDisplayControl,mousePressedCB) );
    meh.buttonReleased.remove( mCB(this,uiWellDisplayControl,mouseReleasedCB) );
    logdisps_ -= &disp;
}


void uiWellDisplayControl::clear()
{
    for ( int idx=logdisps_.size()-1; idx>=0; idx-- )
	removeDahDisplay( *logdisps_[idx] );

    seldisp_ = 0; 	
    lastselmarker_ = 0;
}


MouseEventHandler& uiWellDisplayControl::mouseEventHandler( int dispidx )
    { return logdisps_[dispidx]->scene().getMouseEventHandler(); }


MouseEventHandler* uiWellDisplayControl::mouseEventHandler()
    { return seldisp_ ? &seldisp_->scene().getMouseEventHandler() : 0; }


void uiWellDisplayControl::mouseMovedCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) 
	return;
    if ( !mevh->hasEvent() || mevh->isHandled() ) 
	return;

    if ( seldisp_ )
    {
	const uiWellDahDisplay::Data& zdata = seldisp_->zData();
	xpos_ = seldisp_->dahObjData(true).xax_.getVal(mevh->event().pos().x);
	ypos_ = seldisp_->dahObjData(true).yax_.getVal(mevh->event().pos().y);
	if ( zdata.zistime_ )
	{
	    time_ = ypos_;
	    const Well::D2TModel* d2t = zdata.d2T();
	    if ( d2t && d2t->size() >= 1 )
		depth_ = d2t->getDah( ypos_*0.001f );
	}
	else
	{
	    const Well::Track* tr = zdata.track(); 
	    depth_ = tr ? tr->getDahForTVD( ypos_ ) : mUdf(float);
	    time_ = ypos_;
	}
    }

    BufferString info;
    getPosInfo( info );
    CBCapsule<BufferString> caps( info, this );
    posChanged.trigger( &caps );
}


void uiWellDisplayControl::getPosInfo( BufferString& info ) const
{
    info.setEmpty();
    if ( !seldisp_ ) return;
    if ( selmarker_ )
    {
	info += " Marker: ";
	info += selmarker_->name();
	info += "  ";
    }
    info += "  MD: ";
    bool zinft = seldisp_->zData().dispzinft_ && seldisp_->zData().zistime_;
    float dispdepth = zinft ? mToFeetFactorF*depth_ : depth_;
    info += toString( mNINT32(dispdepth) );
    info += seldisp_->zData().zistime_ ? " Time: " : " Depth: ";
    info += toString( mNINT32(time_) );

#define mGetLogPar( ld )\
    info += "   ";\
    info += ld.log()->name();\
    info += ":";\
    info += toString( ld.log()->getValue( depth_ ) );\
    info += ld.log()->unitMeasLabel();\

    const uiWellDahDisplay::DahObjData& data1 = seldisp_->dahObjData(true);
    const uiWellDahDisplay::DahObjData& data2 = seldisp_->dahObjData(false);

    info += "  ";
    data1.getInfoForDah( depth_, info );
    info += "  ";
    data2.getInfoForDah( depth_, info );
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
    isctrlpressed_ = seldisp_ ? seldisp_->isCtrlPressed() : false;

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
    isctrlpressed_ = false;
    mouseReleased.trigger();
}


void uiWellDisplayControl::setSelDahDisplay( CallBacker* cb )
{
    seldisp_ = 0;
    if ( cb )
    {
	mDynamicCastGet(MouseEventHandler*,mevh,cb)
	for ( int idx=0; idx<logdisps_.size(); idx++)
	{
	    uiWellDahDisplay* ld = logdisps_[idx];
	    if ( &ld->getMouseEventHandler() == mevh ) 
	    {
		seldisp_ = ld;
		break;
	    }
	}
    }
}


void uiWellDisplayControl::setSelMarkerCB( CallBacker* cb ) 
{
    if ( !seldisp_ ) return;
    const MouseEvent& ev = seldisp_->getMouseEventHandler().event();
    int mousepos = ev.pos().y;
    Well::Marker* selmrk = 0;
    for ( int idx=0; idx<seldisp_->markerdraws_.size(); idx++ )
    {
	uiWellDahDisplay::MarkerDraw& markerdraw = *seldisp_->markerdraws_[idx];
	const Well::Marker& mrk = markerdraw.mrk_;
	uiLineItem& li = *markerdraw.lineitm_;

	if ( abs( li.getPos().y - mousepos )<2 ) 
	{
	    selmrk = const_cast<Well::Marker*>( &mrk );
	    break;
	}
    }
    bool markerchanged = ( lastselmarker_ != selmrk );
    setSelMarker( selmrk );
    if ( markerchanged )
	markerSel.trigger();
}


void uiWellDisplayControl::setSelMarker( const Well::Marker* mrk )
{
    if ( lastselmarker_ && ( lastselmarker_ != mrk ) )
	highlightMarker( *lastselmarker_, false );

    if ( mrk )
	highlightMarker( *mrk, true );

    selmarker_ = mrk;

    if ( seldisp_ )
	seldisp_->setToolTip( mrk ? mrk->name() : 0 );

    if ( lastselmarker_ != mrk )
	lastselmarker_ = mrk;
}


void uiWellDisplayControl::highlightMarker( const Well::Marker& mrk, bool yn )
{
    for ( int iddisp=0; iddisp<logdisps_.size(); iddisp++ )
    {
	uiWellDahDisplay& ld = *logdisps_[iddisp];
	uiWellDahDisplay::MarkerDraw* mrkdraw = ld.getMarkerDraw( mrk );
	if ( !mrkdraw ) continue;
	const LineStyle& ls = mrkdraw->ls_;
	uiLineItem& li = *mrkdraw->lineitm_;
	int width = yn ? ls.width_+2 : ls.width_;
	li.setPenStyle( LineStyle( ls.type_, width, mrk.color() ) ); 
    }
}


void uiWellDisplayControl::setCtrlPressed( bool yn )
{
    for ( int idx=0; idx<logdisps_.size(); idx++)
    {
	uiWellDahDisplay* ld = logdisps_[idx];
	ld->setCtrlPressed( yn );
    }
    isctrlpressed_ = yn;
}

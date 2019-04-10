/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2010
________________________________________________________________________

-*/


#include "uiwelldisplaycontrol.h"

#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uiwelllogdisplay.h"

#include "mouseevent.h"
#include "welld2tmodel.h"
#include "welllog.h"
#include "wellmarker.h"


uiWellDisplayControl::uiWellDisplayControl( uiWellDahDisplay& l )
    : selmarker_(*new Well::Marker("",mUdf(float)))
    , seldisp_(0)
    , lastselmarker_(*new Well::Marker("",mUdf(float)))
    , ismousedown_(false)
    , isctrlpressed_(false)
    , xpos_(0)
    , ypos_(0)
    , time_(0)
    , dah_(0)
    , posChanged(this)
    , mousePressed(this)
    , mouseReleased(this)
    , markerSel(this)
{
    addDahDisplay( l );
}


uiWellDisplayControl::~uiWellDisplayControl()
{
    detachAllNotifiers();
    clear();
}


void uiWellDisplayControl::addDahDisplay( uiWellDahDisplay& disp )
{
    logdisps_ += &disp;
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    mAttachCB( meh.movement, uiWellDisplayControl::setSelDahDisplay );
    mAttachCB( meh.movement, uiWellDisplayControl::setSelMarkerCB );
    mAttachCB( meh.movement, uiWellDisplayControl::mouseMovedCB );
    mAttachCB( meh.buttonReleased, uiWellDisplayControl::mouseReleasedCB );
    mAttachCB( meh.buttonPressed,uiWellDisplayControl::mousePressedCB );
}


void uiWellDisplayControl::removeDahDisplay( uiWellDahDisplay& disp )
{
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    mDetachCB( meh.movement, uiWellDisplayControl::mouseMovedCB );
    mDetachCB( meh.movement, uiWellDisplayControl::setSelMarkerCB );
    mDetachCB( meh.movement, uiWellDisplayControl::setSelDahDisplay );
    mDetachCB( meh.buttonPressed,uiWellDisplayControl::mousePressedCB );
    mDetachCB( meh.buttonReleased,uiWellDisplayControl::mouseReleasedCB );
    logdisps_ -= &disp;
}


void uiWellDisplayControl::clear()
{
    logdisps_.erase();
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
	xpos_ = seldisp_->dahObjData(0).xax_.getVal(mevh->event().pos().x_);
	ypos_ = seldisp_->dahObjData(0).yax_.getVal(mevh->event().pos().y_);
	const Well::Track* tr = zdata.track();
	if ( zdata.zistime_ )
	{
	    time_ = ypos_;
	    const Well::D2TModel* d2t = zdata.d2T();
	    if ( d2t && d2t->size()>1 && tr && tr->size()>1 )
		dah_ = d2t->getDah( ypos_*0.001f, *tr );
	}
	else
	{
	    dah_ = tr ? tr->getDahForTVD( ypos_ ) : mUdf(float);
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
    info.setEmpty(); if ( !seldisp_ ) return;
    const uiWellDahDisplay::DahObjData& data1 = seldisp_->dahObjData(0);
    const uiWellDahDisplay::DahObjData& data2 = seldisp_->dahObjData(1);
    if ( data1.hasData() ) { info += "  "; data1.getInfoForDah(dah_,info); }
    if ( data2.hasData() ) { info += "  "; data2.getInfoForDah(dah_,info); }
    if ( !selmarker_.isUdf() ){info += "  Marker:"; info += selmarker_.name(); }

    const uiWellDahDisplay::Data& zdata = seldisp_->zData();
    const bool zinft = zdata.dispzinft_ && zdata.zistime_;
    const FixedString depthunitstr = getDistUnitString( zinft, false );
    info.add( "   MD: " ).add( zinft ? mToFeetFactorF*dah_ : dah_ )
	.add( ' ' ).add( depthunitstr );

    const Well::Track* track = zdata.track();
    if ( track )
    {
	const float tvdss = mCast(float,track->getPos(dah_).z_);
	const float tvd = track->getKbElev() + tvdss;
	info.add( "  TVD: " ).add( zinft ? mToFeetFactorF*tvd : tvd )
	    .add( ' ' ).add( depthunitstr );
	info.add( "TVDSS: ").add( zinft ? mToFeetFactorF*tvdss : tvdss )
	    .add( ' ' ).add( depthunitstr );
    }

    if ( zdata.zistime_ )
    {
	info.add( "  TWT: " ).add( time_ );
	info.add( toString(SI().zDomain().unitStr()) );
    }
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
    Well::Marker selmrk = Well::Marker::udf();

    for ( int idx=0; idx<seldisp_->markerdraws_.size(); idx++ )
    {
	const uiWellDahDisplay::MarkerDraw& markerdraw =
						   *seldisp_->markerdraws_[idx];
	if ( markerdraw.contains(ev.pos()) )
	{
	    selmrk = markerdraw.mrkr_;
	    break;
	}
    }

    bool markerchanged = ( lastselmarker_ != selmrk );
    setSelMarker( selmrk );
    if ( markerchanged )
	markerSel.trigger();
}


void uiWellDisplayControl::setSelMarker( Well::Marker mrk )
{
   if ( lastselmarker_ != mrk )
	highlightMarker( lastselmarker_, false );

    if ( !mrk.isUdf() )
	highlightMarker( mrk, true );

    selmarker_ = mrk;

   if ( seldisp_ )
	seldisp_->setToolTip( selmarker_.isUdf() ? uiString::empty()
			    : toUiString(selmarker_.name() ) );
    if ( lastselmarker_ != mrk )
	lastselmarker_ = mrk;
}


void uiWellDisplayControl::highlightMarker( Well::Marker mrk, bool hlt )
{

    for ( int iddisp=0; iddisp<logdisps_.size(); iddisp++ )
    {
	uiWellDahDisplay& ld = *logdisps_[iddisp];
	uiWellDahDisplay::MarkerDraw* mrkdraw = ld.getMarkerDraw( mrk );
	if ( !mrkdraw ) continue;
	hlt ? mrkdraw->highlight() : mrkdraw->unHighlight();
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

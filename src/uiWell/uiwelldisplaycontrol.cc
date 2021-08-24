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
    : selmarker_(0)
    , seldisp_(0)
    , lastselmarker_(0)
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
	float zfac = 1.f;
	if ( !zdata.zistime_ && SI().depthsInFeet() )
	    zfac = mToFeetFactorF;

	xpos_ = seldisp_->dahObjData(true).xax_.getVal(mevh->event().pos().x);
	ypos_ = seldisp_->dahObjData(true).yax_.getVal(mevh->event().pos().y);
	const Well::Track* track = zdata.track();
	if ( zdata.zistime_ )
	{
	    time_ = ypos_;
	    const Well::D2TModel* d2t = zdata.d2T();
	    if ( d2t && d2t->size()>1 && track && track->size()>1 )
		dah_ = d2t->getDah( ypos_*0.001f, *track );
	}
	else
	{
	    dah_ = track ? track->getDahForTVD( ypos_/zfac ) : mUdf(float);
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
    if ( !seldisp_ || mIsUdf(dah_) )
	return;

    const uiWellDahDisplay::DahObjData& data1 = seldisp_->dahObjData(true);
    const uiWellDahDisplay::DahObjData& data2 = seldisp_->dahObjData(false);
    if ( data1.hasData() ) { info += "  "; data1.getInfoForDah(dah_,info); }
    if ( data2.hasData() ) { info += "  "; data2.getInfoForDah(dah_,info); }
    if ( selmarker_ ) { info += "  Marker:"; info += selmarker_->name(); }

    info += "  MD:";
    const uiWellDahDisplay::Data& zdata = seldisp_->zData();
    const bool zinft = SI().depthsInFeet();
    const FixedString depthunitstr = getDistUnitString(zinft,false);
    info += toString( zinft ? mToFeetFactorF*dah_ : dah_, 2 );
    info += depthunitstr;

    const Well::Track* track = zdata.track();
    if ( track )
    {
	info += "  TVD:";
	const float tvdss = mCast(float,track->getPos(dah_).z);
	const float tvd = track->getKbElev() + tvdss;
	info += toString( zinft ? mToFeetFactorF*tvd : tvd, 2 );
	info += depthunitstr;
	info += "  TVDSS:";
	info += toString( zinft ? mToFeetFactorF*tvdss : tvdss, 2 );
	info += depthunitstr;
    }

    if ( zdata.zistime_ )
    {
	info += "  TWT:";
	info += toString( time_, 2 );
	info += SI().zDomain().unitStr();
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
    int mousepos = ev.pos().y;
    Well::Marker* selmrk = 0;
    for ( int idx=0; idx<seldisp_->markerdraws_.size(); idx++ )
    {
	uiWellDahDisplay::MarkerDraw& markerdraw = *seldisp_->markerdraws_[idx];
	const Well::Marker& mrk = markerdraw.mrk_;
	uiLineItem& li = *markerdraw.lineitm_;

	if ( abs(li.lineRect().centre().y-mousepos) < 2 )
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
	seldisp_->setToolTip( mrk ? toUiString(mrk->name()) :
						    uiStrings::sEmptyString() );

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
	const OD::LineStyle& ls = mrkdraw->ls_;
	uiLineItem& li = *mrkdraw->lineitm_;
	int width = yn ? ls.width_+2 : ls.width_;
	li.setPenStyle( OD::LineStyle( ls.type_, width, mrk.color() ) );
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

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplaycontrol.cc,v 1.2 2010-04-08 13:13:11 cvsbruno Exp $";


#include "uiwelldisplaycontrol.h"
#include "mouseevent.h"
#include "uimenuhandler.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uicolor.h"
#include "welld2tmodel.h"



uiWellDisplayReshape::uiWellDisplayReshape( uiWellDisplay& disp )
    : CallBacker(CallBacker::CallBacker())
    , menu_(*new uiMenuHandler(&disp,-1))
    , addlogmnuitem_("Add Log Display...",0)      			 
    , remlogmnuitem_("Remove Log Display...",1)
    , addstratmnuitem_("Add Strat Display...",2)
    , remstratmnuitem_("Remove Strat Display...",3)
{
    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiWellDisplayReshape,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiWellDisplayReshape,handleMenuCB));
}


void uiWellDisplayReshape::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;

    mAddMenuItem( menu, &remlogmnuitem_, true, false );
    mAddMenuItem( menu, &addlogmnuitem_, true, false );
    mAddMenuItem( menu, &remstratmnuitem_, true, false );
    mAddMenuItem( menu, &addstratmnuitem_, true, false );
}


void uiWellDisplayReshape::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==addlogmnuitem_.id )
	addLogPanel();
    if ( mnuid==addlogmnuitem_.id )
	remLogPanel();
}


void uiWellDisplayReshape::addLogPanel()
{}
void uiWellDisplayReshape::remLogPanel()
{}



uiWellDisplayMarkerEdit::uiWellDisplayMarkerEdit( uiWellLogDisplay& disp, Well::Data& wd )
    : CallBacker(CallBacker::CallBacker())
    , wd_(wd)  
    , mousepressed_(false)			
    , selmarker_(0)		
    , curmarker_(0)		
    , lastmarker_(0)		
    , menu_(*new uiMenuHandler(disp.parent(),-1))
    , addmrkmnuitem_("Add marker...",1)      			 
    , remmrkmnuitem_("Remove marker...",0)      				
{
    addLogDisplay( disp );
    menu_.ref();
    menu_.createnotifier.notify(mCB(this,uiWellDisplayMarkerEdit,createMenuCB));
    menu_.handlenotifier.notify(mCB(this,uiWellDisplayMarkerEdit,handleMenuCB));
}


uiWellDisplayMarkerEdit::~uiWellDisplayMarkerEdit()
{
    menu_.unRef();
}


void uiWellDisplayMarkerEdit::addLogDisplay( uiWellLogDisplay& ld )
{
    logdisps_ += &ld;
    MouseEventHandler& meh = ld.getMouseEventHandler();
    meh.buttonReleased.notify( mCB(this,uiWellDisplayMarkerEdit,mouseRelease) );
    meh.buttonReleased.notify( mCB(this,uiWellDisplayMarkerEdit,usrClickCB) );
    meh.buttonPressed.notify( mCB(this,uiWellDisplayMarkerEdit,mousePressed) );
    meh.movement.notify( mCB(this,uiWellDisplayMarkerEdit,mouseMoved) );
    ld.setEditMarkers( true );
}


void uiWellDisplayMarkerEdit::mousePressed( CallBacker* cb )
{
    mousepressed_ = true;
    if ( mousepressed_ )
       selmarker_ = selectMarker( cb, false );	
}


void uiWellDisplayMarkerEdit::mouseMoved( CallBacker* cb )
{
    if ( mousepressed_ && selmarker_ )
	changeMarkerPos( selmarker_ );
    
    curmarker_ = selectMarker( cb, true );
    if ( curmarker_ != lastmarker_ )  
    {
	for ( int idx=0; idx<logdisps_.size(); idx++ )
	    logdisps_[idx]->highlightMarkerItem( curmarker_ );
	lastmarker_ = curmarker_;
    }
}


void uiWellDisplayMarkerEdit::mouseRelease( CallBacker* )
{
    mousepressed_ = false;
    selmarker_ = 0; 
}



void uiWellDisplayMarkerEdit::trigMarkersChanged()
{
    wd_.markerschanged.trigger();
}


void uiWellDisplayMarkerEdit::usrClickCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    if ( !mevh ) 
	return; 
    if ( !mevh->hasEvent() )
	return;
    if ( mevh->isHandled() )
	return;

    mevh->setHandled( handleUserClick(mevh->event()) );
}


bool uiWellDisplayMarkerEdit::handleUserClick( const MouseEvent& ev )
{
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	 !ev.altStatus() )
    {
	menu_.executeMenu(0);
	return true;
    }
    return false;
}


class uiWellDispAddMarkerDlg : public uiDialog
{
public : 
    uiWellDispAddMarkerDlg( uiParent* p, float dah )
	: uiDialog(p,uiDialog::Setup("Add marker",
				     "Specify properties",mNoHelpID))
	, marker_(0)							     
    {
	namefld_ = new uiGenInput( this, "Name", StringInpSpec("Marker") );
	depthfld_ = new uiGenInput( this, "Depth", FloatInpSpec(dah) );
	depthfld_->attach( alignedBelow, namefld_ );
	uiColorInput::Setup csu( Color::DgbColor() );
	csu.lbltxt( "Color" ).withalpha(false);
	colorfld_ = new uiColorInput( this, csu, "Color" );
	colorfld_->attach( alignedBelow, depthfld_ );
    }

    bool acceptOK( CallBacker* )
    {
	const char* nm = namefld_->text();
	float dpt = depthfld_->getfValue();
	marker_ = new Well::Marker( nm, dpt );
	marker_->setColor( colorfld_->color() );
	return true;
    }

    Well::Marker* marker() { return marker_; }

protected :

    Well::Marker*	marker_;
    uiGenInput*		namefld_;
    uiGenInput*		depthfld_;
    uiColorInput*	colorfld_;
};


void uiWellDisplayMarkerEdit::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    bool ishandled = true;
    if ( !logdisps_.isEmpty() ) return;
    if ( mnuid==addmrkmnuitem_.id )
    {
	uiWellDispAddMarkerDlg dlg( 0, mousePos() );
	if ( dlg.go() )
	{
	    Well::Marker* newmrk = dlg.marker();
	    if ( !newmrk ) return;
	    Well::MarkerSet& mrkset = wd_.markers();
	    for ( int idx=0; idx<mrkset.size(); idx++ )
	    {
		Well::Marker& mrk = *mrkset[idx]; 
		if ( newmrk->dah() > mrk.dah() )
		    continue;
		else 
		{ 
		    mrkset.insertAt( newmrk, idx );
		    trigMarkersChanged();
		    return;
		}
	    }
	}
    }
    else if ( mnuid==remmrkmnuitem_.id  && selectMarker( cb, true ) )
    {
	ObjectSet<Well::Marker>& mrkset = wd_.markers();
	delete mrkset.remove( mrkset.indexOf( selectMarker(cb,true) ), true);
	trigMarkersChanged();
    }
    else
	ishandled = false;

    menu->setIsHandled( ishandled );
}


void uiWellDisplayMarkerEdit::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;

    if ( logdisps_.size() && selectMarker(cb,true) ) 
	mAddMenuItem( menu, &remmrkmnuitem_, true, false );
    mAddMenuItem( menu, &addmrkmnuitem_, true, false );
}


float uiWellDisplayMarkerEdit::mousePos()  
{
    if ( !logdisps_.size() ) return 0;
    const MouseEvent& ev = logdisps_[0]->getMouseEventHandler().event();
    return logdisps_[0]->logData(0).yax_.getVal( ev.pos().y );
}


Well::Marker* uiWellDisplayMarkerEdit::selectMarker( CallBacker* cb, bool allowrightclk )
{
    mDynamicCastGet(MouseEventHandler*,mevh,cb)
    uiWellLogDisplay* seldisp = 0;
    for ( int idx=0; idx<logdisps_.size(); idx++)
    {
	if ( &logdisps_[idx]->getMouseEventHandler() == mevh )
	    seldisp = logdisps_[idx];
    }

    if ( !seldisp ) return 0;

    const MouseEvent& ev = seldisp->getMouseEventHandler().event();
    if ( (ev.buttonState() & OD::MidButton ) ) return 0;
    if ( !allowrightclk )
    {	
	 if ( !(ev.buttonState() & OD::LeftButton ) ||
	       (ev.buttonState() & OD::RightButton ) )
	return 0;
    }

    int mousepos = ev.pos().y;
    Well::Marker* mrk = 0;
    for ( int idx=0; idx<seldisp->markerItems().size(); idx++ )
    {
	if ( abs( seldisp->markerItems()[idx]->itm_->getPos().y-mousepos )<2 )
	    return ( &seldisp->markerItems()[idx]->mrk_ );
    }
    return 0;
}


#define mSetZVal(val)\
    if ( logdisps_[0]->zIsTime() && wd_.haveD2TModel() )\
	val = wd_.d2TModel()->getDepth( val )/1000;
void uiWellDisplayMarkerEdit::changeMarkerPos( Well::Marker* mrk )
{
    if ( selmarker_ )
    {
	float mousepos = mousePos();
	mSetZVal( mousepos );
	selmarker_->setDah( mousepos );
	trigMarkersChanged();
    }
}




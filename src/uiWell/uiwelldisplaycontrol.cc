/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplaycontrol.cc,v 1.9 2010-05-19 12:31:21 cvsbruno Exp $";


#include "uiwelldisplaycontrol.h"
#include "uicolor.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uimenuhandler.h"
#include "uiwelllogdisplay.h"

#include "mouseevent.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "wellmarker.h"


uiWellDisplayMarkerEdit::uiWellDisplayMarkerEdit( uiWellLogDisplay& disp, 
						  Well::Data* wd )
    : CallBacker(CallBacker::CallBacker())
    , wd_(wd)  
    , mousepressed_(false)			
    , selmarker_(0)		
    , curmarker_(0)		
    , lastmarker_(0)
    , menu_(0)	 
    , addmrkmnuitem_("Add marker...",1)      			 
    , remmrkmnuitem_("Remove marker...",0)
    , needsave_(false)
    , edit_(false)	      
{
    if ( disp.parent() )
	addMenu(new uiMenuHandler(disp.parent(),-1) );	 
    addLogDisplay( disp );
}


uiWellDisplayMarkerEdit::~uiWellDisplayMarkerEdit()
{
    if ( menu_ ) menu_->unRef();
}


void uiWellDisplayMarkerEdit::addMenu( uiMenuHandler* menu )
{
    if ( menu_ ) 
    { menu_->unRef(); }
    menu_ = menu;
    menu_->ref();
    menu_->createnotifier.notify(mCB(this,uiWellDisplayMarkerEdit,createMenuCB));
    menu_->handlenotifier.notify(mCB(this,uiWellDisplayMarkerEdit,handleMenuCB));
}


void uiWellDisplayMarkerEdit::addLogDisplay( uiWellLogDisplay& ld )
{
    logdisps_ += &ld;
    MouseEventHandler& meh = ld.getMouseEventHandler();
    meh.buttonPressed.notify( mCB(this,uiWellDisplayMarkerEdit,mousePressed) );
    meh.buttonPressed.notify( mCB(this,uiWellDisplayMarkerEdit,usrClickCB) );
    meh.movement.notify( mCB(this,uiWellDisplayMarkerEdit,mouseMoved) );
}


void uiWellDisplayMarkerEdit::mousePressed( CallBacker* cb )
{
    if ( !edit_ ) 
	return;

    mousepressed_ = !mousepressed_;
    selmarker_ = mousepressed_ ? selectMarker( cb, false ) : 0;
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
    if ( wd_ ) wd_->markerschanged.trigger();
    needsave_ = true;
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
	if ( menu_ ) menu_->executeMenu(0);
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
    if ( !wd_ ) return;
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    bool ishandled = true;
    if ( logdisps_.isEmpty() ) return;
    if ( mnuid==addmrkmnuitem_.id )
    {
	uiWellDispAddMarkerDlg mrkdlg( menu_->getParent(), mousePos() );
	if ( mrkdlg.go() )
	{
	    Well::Marker* newmrk = mrkdlg.marker();
	    if ( !newmrk ) return;
	    Well::MarkerSet& mrkset = wd_->markers();
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
    else if ( mnuid==remmrkmnuitem_.id  && selectMarker( 0, true ) )
    {
	ObjectSet<Well::Marker>& mrkset = wd_->markers();
	delete mrkset.remove( mrkset.indexOf( selectMarker(0,true) ), true);
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

    if ( logdisps_.size() && selectMarker(0,true) ) 
	mAddMenuItem( menu, &remmrkmnuitem_, true, false )
    mAddMenuItem( menu, &addmrkmnuitem_, true, false )
}


float uiWellDisplayMarkerEdit::mousePos() 
{
    if ( !logdisps_.size() ) return 0;
    return logdisps_[0]->mousePos();
}


Well::Marker* uiWellDisplayMarkerEdit::selectMarker( CallBacker* cb, bool allowrightclk )
{
    uiWellLogDisplay* seldisp = 0;
    if ( cb )
    {
	mDynamicCastGet(MouseEventHandler*,mevh,cb)
	for ( int idx=0; idx<logdisps_.size(); idx++)
	{
	    if ( &logdisps_[idx]->getMouseEventHandler() == mevh )
		seldisp = logdisps_[idx];
	}
    }
    else if ( logdisps_.size() ) seldisp = logdisps_[0];
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
    if ( logdisps_[0]->data().zistime_ && wd_ &&\
	    wd_->haveD2TModel() && wd_->d2TModel()->size() > 0 )\
	val = wd_->d2TModel()->getDepth( val/1000 );
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


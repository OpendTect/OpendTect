/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Apr 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplaycontrol.cc,v 1.14 2010-08-13 12:30:01 cvsbruno Exp $";


#include "uiwelldisplaycontrol.h"
#include "uicolor.h"
#include "uibutton.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiwelllogdisplay.h"
#include "welllog.h"

#include "mouseevent.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "wellmarker.h"
#include "wellstratman.h"


uiWellDisplayControl::uiWellDisplayControl( uiWellLogDisplay& l, Well::Data* w) 
    : CallBacker(CallBacker::CallBacker())
    , wd_(w)  
    , mousepressed_(false)			
    , selmarker_(0)		
    , curmarker_(0)		
    , lastmarker_(0)
    , menu_(0)	 
    , addmrkmnuitem_("Add marker...",1)      			 
    , remmrkmnuitem_("Remove marker...",0)
    , needsave_(false)
    , edit_(false)	      
    , infoChanged(this)       
{
    if ( l.parent() )
	addMenu(new uiMenuHandler(l.parent(),-1) );	
    addLogDisplay( l );
}    


uiWellDisplayControl::~uiWellDisplayControl()
{
    if ( menu_ ) menu_->unRef();
}



void uiWellDisplayControl::addLogDisplay( uiWellLogDisplay& disp )
{
    logdisps_ += &disp;
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    meh.movement.notify( mCB( this, uiWellDisplayControl, mouseMoved ) );
    meh.buttonPressed.notify( mCB(this,uiWellDisplayControl,mousePressed) );
    meh.buttonPressed.notify( mCB(this,uiWellDisplayControl,usrClicked) );
}


void uiWellDisplayControl::removeLogDisplay( uiWellLogDisplay& disp )
{
    MouseEventHandler& meh = mouseEventHandler( logdisps_.size()-1 );
    meh.movement.remove( mCB( this, uiWellDisplayControl, mouseMoved ) );
    meh.buttonPressed.remove( mCB(this,uiWellDisplayControl,mousePressed) );
    meh.buttonPressed.remove( mCB(this,uiWellDisplayControl,usrClicked) );
    logdisps_ -= &disp;
}



MouseEventHandler& uiWellDisplayControl::mouseEventHandler( int dispidx )
{
    return logdisps_[dispidx]->scene().getMouseEventHandler();
}


void uiWellDisplayControl::mouseMoved( CallBacker* cb )
{
    for ( int idx=0; idx<logdisps_.size(); idx++ )
    {
	if ( !mouseEventHandler( idx ).hasEvent() )
	                continue;
	const MouseEvent& ev = mouseEventHandler(idx).event();

	BufferString info;
	getPosInfo( idx, mousePos(idx), info );
	CBCapsule<BufferString> caps( info, this );
	infoChanged.trigger( &caps );
    }

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


float uiWellDisplayControl::mousePos( int dispidx ) 
{
    MouseEventHandler& meh = mouseEventHandler( dispidx );
    const MouseEvent& ev = meh.event();
    return logdisps_[dispidx]->logData(true).yax_.getVal( ev.pos().y );
}


void uiWellDisplayControl::getPosInfo( int dispidx, float pos, 
					BufferString& info ) const
{
    const Well::Well2DDispData& data = logdisps_[dispidx]->data();
    info.setEmpty();
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

    const uiWellLogDisplay::LogData& ldata1 =logdisps_[dispidx]->logData(true);
    const uiWellLogDisplay::LogData& ldata2 =logdisps_[dispidx]->logData(false);
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


void uiWellDisplayControl::addMenu( uiMenuHandler* menu )
{
    if ( menu_ ) 
    { menu_->unRef(); }
    menu_ = menu;
    menu_->ref();
    menu_->createnotifier.notify(mCB(this,uiWellDisplayControl,createMenuCB));
    menu_->handlenotifier.notify(mCB(this,uiWellDisplayControl,handleMenuCB));
}


void uiWellDisplayControl::mousePressed( CallBacker* cb )
{
    if ( !edit_ ) 
	return;

    mousepressed_ = !mousepressed_;
    selmarker_ = mousepressed_ ? selectMarker( cb, false ) : 0;
}


void uiWellDisplayControl::mouseRelease( CallBacker* )
{
    mousepressed_ = false;
    selmarker_ = 0; 
}



void uiWellDisplayControl::trigMarkersChanged()
{
    if ( wd_ ) wd_->markerschanged.trigger();
    needsave_ = true;
}


void uiWellDisplayControl::usrClicked( CallBacker* cb )
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


bool uiWellDisplayControl::handleUserClick( const MouseEvent& ev )
{
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	 !ev.altStatus() )
    {
	if ( menu_ ) menu_->executeMenu(0);
	return true;
    }
    return false;
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }
class uiWellDispAddMarkerDlg : public uiDialog
{
public : 
    uiWellDispAddMarkerDlg( uiParent* p, const Well::MarkerSet& ms, float dah )
	: uiDialog(p,uiDialog::Setup("Add marker",
				     "Specify properties",mNoHelpID))
	, ms_(ms)
    {
	namefld_ = new uiGenInput( this, "Name", StringInpSpec("Marker") );
	depthfld_ = new uiGenInput( this, "Depth", FloatInpSpec(dah) );
	depthfld_->attach( alignedBelow, namefld_ );
	uiColorInput::Setup csu( Color::DgbColor() );
	csu.lbltxt( "Color" ).withalpha(false);
	colorfld_ = new uiColorInput( this, csu, "Color" );
	colorfld_->attach( alignedBelow, depthfld_ );

	stratmrkfld_ = new uiCheckBox( this, "Set as stratigraphic markers" );
	stratmrkfld_->attach( alignedBelow, colorfld_ );
	stratmrkfld_->setChecked( true );
    }

    bool acceptOK( CallBacker* )
    {
	name_ = namefld_->text();
	if ( !strcmp( name_, "" ) )
	    mErrRet( "Please specify a marker name" )
	if ( ms_.isPresent( name_.buf() ) )
	    mErrRet("Marker name already existing" )
	dah_ = depthfld_->getfValue();
	if ( mIsUdf( dah_ ) )
	    mErrRet( "No valid position entered" )
	col_ = colorfld_->color();
	isstrat_ = stratmrkfld_->isChecked();
	return true;
    }

    float 		dah_;
    Color 		col_;
    BufferString 	name_;
    bool 		isstrat_;

protected :

    uiGenInput*		namefld_;
    uiGenInput*		depthfld_;
    uiColorInput*	colorfld_;
    uiCheckBox*		stratmrkfld_;

    const Well::MarkerSet& ms_;
};


#define mSetZVal(val)\
    if ( logdisps_[0]->data().zistime_ && wd_ &&\
	    wd_->haveD2TModel() && wd_->d2TModel()->size() > 0 )\
	val = wd_->d2TModel()->getDah( val/1000 );
void uiWellDisplayControl::handleMenuCB( CallBacker* cb )
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
	float mousepos = mousePos(0);
	mSetZVal( mousepos )
	Well::MarkerSet& mrkset = wd_->markers();
	uiWellDispAddMarkerDlg mrkdlg( menu_->getParent(), mrkset, mousepos );
	if ( mrkdlg.go() )
	{
	    BufferString nm( mrkdlg.name_ ); Color col = mrkdlg.col_;
	    Well::Marker* marker = new Well::Marker( nm, mrkdlg.dah_ );
	    marker->setColor( col );
	    if (  mrkdlg.isstrat_ )
		marker->setLevelID( Well::StratMGR().addLevel( nm, col ) );
	    mrkset.insertNew( marker );
	    trigMarkersChanged();
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


void uiWellDisplayControl::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;

    if ( logdisps_.isEmpty() && selectMarker(0,true) ) 
	mAddMenuItem( menu, &remmrkmnuitem_, true, false )
    mAddMenuItem( menu, &addmrkmnuitem_, true, false )
}


Well::Marker* uiWellDisplayControl::selectMarker( CallBacker* cb, bool allowrightclk )
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


void uiWellDisplayControl::changeMarkerPos( Well::Marker* mrk )
{
    if ( selmarker_ )
    {
	float mousepos = mousePos(0);
	mSetZVal( mousepos );
	selmarker_->setDah( mousepos );
	trigMarkersChanged();
    }
}


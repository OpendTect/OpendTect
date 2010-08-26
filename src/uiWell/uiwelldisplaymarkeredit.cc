/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Aug 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplaymarkeredit.cc,v 1.2 2010-08-26 14:37:28 cvsbruno Exp $";


#include "uiwelldisplaymarkeredit.h"

#include "uiwelldisplaycontrol.h"
#include "uicolor.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiwelllogdisplay.h"

#include "keyboardevent.h"
#include "mouseevent.h"
#include "randcolor.h"
#include "survinfo.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
#include "wellstratman.h"
#include "welldata.h"



void WellDispMarkerParams::getFromMarker( const Well::Marker& mrk )
{
    name_ = mrk.name();
    col_ = mrk.color();
    dah_ = mrk.dah();
    isstrat_ = ( mrk.levelID() > 0 );
}


void WellDispMarkerParams::putToMarker( Well::Marker& mrk )
{
    mrk.setName( name_ );
    mrk.setDah( dah_ );
    mrk.setColor( col_ );
    if ( isstrat_ )
	mrk.setLevelID( Well::StratMGR().addLevel( name_, col_ ) );
}



#define mErrRet(msg,act) { uiMSG().error( msg ); act; }
uiWellDispMarkerEditGrp::uiWellDispMarkerEditGrp( uiParent* p, 
						  WellDispMarkerParams& par )
	: uiGroup(p,"Edit Markers Group")
	, par_(par) 
	, istime_(SI().zIsTime())
	, dispparchg(this)	   
{
    namefld_ = new uiGenInput( this, "Name", StringInpSpec("Marker") );

    bool istime = SI().zIsTime();
    posfld_ = new uiGenInput( this, istime ? "Time" : "Depth", FloatInpSpec(0));
    posfld_->attach( alignedBelow, namefld_ );
    uiColorInput::Setup csu( getRandStdDrawColor() );
    csu.lbltxt( "Color" ).withalpha(false);
    colorfld_ = new uiColorInput( this, csu, "Color" );
    colorfld_->attach( alignedBelow, posfld_ );

    stratmrkfld_ = new uiCheckBox( this, "Set as stratigraphic marker" );
    stratmrkfld_->attach( alignedBelow, colorfld_ );
    stratmrkfld_->setChecked( true );

    CallBack parchgcb = mCB(this, uiWellDispMarkerEditGrp, getFromScreen );
    namefld_->valuechanged.notify( parchgcb );
    colorfld_->colorChanged.notify( parchgcb );
    posfld_->valuechanged.notify( parchgcb ); 
    stratmrkfld_->activated.notify( parchgcb );
}


void uiWellDispMarkerEditGrp::setDefault()
{
    par_.name_ = "Marker";
    par_.dah_ = 0;
    par_.time_= 0;
    par_.col_ = getRandStdDrawColor();
    par_.isstrat_ = true;

    putToScreen();
}


void uiWellDispMarkerEditGrp::getFromScreen( CallBacker* )
{
    par_.name_ = namefld_->text();
    par_.time_ = posfld_->getfValue();
    par_.col_ = colorfld_->color();
    par_.isstrat_ = stratmrkfld_->isChecked();
    dispparchg.trigger();
}


bool uiWellDispMarkerEditGrp::checkPars()
{
    if ( !strcmp( par_.name_, "" ) )
	mErrRet( "Please specify a marker name", return false )
    if ( mIsUdf( par_.dah_ ) )
	mErrRet( "No valid position entered", return false )
    return true;
}


void uiWellDispMarkerEditGrp::putToScreen()
{
    namefld_->setText( par_.name_ );
    colorfld_->setColor( par_.col_ );
    posfld_->setValue( par_.time_ );
    stratmrkfld_->setChecked( par_.isstrat_ );
}


void uiWellDispMarkerEditGrp::setPos( float time, float depth )
{
    par_.dah_ = depth;
    par_.time_ = time;
    posfld_->setValue( istime_ ? time : depth );
}


void uiWellDispMarkerEditGrp::setFldsSensitive( bool yn )
{
    namefld_->setSensitive( yn );
    colorfld_->setSensitive( yn );
    posfld_->setSensitive( yn );
    stratmrkfld_->setSensitive( yn);
}




uiWellDispEditMarkerDlg::uiWellDispEditMarkerDlg( uiParent* p,
					      WellDispMarkerParams& par )
	: uiDialog(p,uiDialog::Setup("Edit Markers Dialog",
				"Select editing mode",mTODOHelpID)
				.modal(false))
{
    setCtrlStyle( LeaveOnly );

    modefld_ = new uiGenInput( this, "Select mode", 
			    BoolInpSpec(true,"Add/Remove","Edit Selected") );
    modefld_->valuechanged.notify( mCB(this,uiWellDispEditMarkerDlg,modeChg) );

    uiSeparator* modesep = new uiSeparator( this, "Mode Sep" );
    modesep->attach( stretchedBelow, modefld_ );

    mrkgrp_ = new uiWellDispMarkerEditGrp( this, par );
    mrkgrp_->attach( ensureBelow, modefld_ );
    mrkgrp_->attach( ensureBelow, modesep );

    setMode( true );
}


void uiWellDispEditMarkerDlg::modeChg( CallBacker* )
{
    bool isaddremmode = isAddRemMode();
    mrkgrp_->setDefault();
    mrkgrp_->setFldsSensitive( isaddremmode );
}


void uiWellDispEditMarkerDlg::setMode( bool addmode )
{
    modefld_->setValue( addmode );
    modeChg( 0 );
}


bool uiWellDispEditMarkerDlg::isAddRemMode() const
{
    return modefld_->getBoolValue();
}



uiWellDispMarkerEditor::uiWellDispMarkerEditor( uiParent* p )
    : editdlg_(0)	
    , menu_(0)	
    , isediting_(false)  
    , startmnuitem_("Start Marker Editing...",0)      			 
    , curmrk_(0)
    , curctrl_(0)
    , curwd_(0)	 
    , lasteditwd_(0)
    , lasteditmrk_(0)	    
    , editFinished(this)			    
{
    menu_ = new uiMenuHandler( p, -1 );	
    menu_->ref();
    menu_->createnotifier.notify(mCB(this,uiWellDispMarkerEditor,createMenuCB));
    menu_->handlenotifier.notify(mCB(this,uiWellDispMarkerEditor,handleMenuCB));
}


uiWellDispMarkerEditor::~uiWellDispMarkerEditor()
{
    if ( menu_ ) menu_->unRef();
    for ( int idx=ctrls_.size()-1; idx>=0; idx-- )
	removeCtrl( *ctrls_[idx], *wds_[idx] );
}


void uiWellDispMarkerEditor::addCtrl( uiWellDisplayControl& ctrl, 
					  Well::Data& wd  )
{
    CallBack cbclk = mCB( this, uiWellDispMarkerEditor,handleUsrClickCB );
    CallBack cbchg = mCB( this, uiWellDispMarkerEditor,handleCtrlChangeCB );
    CallBack cbpos = mCB( this, uiWellDispMarkerEditor,posChgCB );
    ctrl.posChanged.notify( cbchg );
    ctrl.posChanged.notify( cbpos );
    ctrl.mousePressed.notify( cbclk ); 
    ctrls_ += &ctrl;
    wds_ += &wd;
}


void uiWellDispMarkerEditor::removeCtrl( uiWellDisplayControl& ctrl,
       						Well::Data& wd )
{
    CallBack cbclk = mCB( this, uiWellDispMarkerEditor,handleUsrClickCB );
    CallBack cbpos = mCB( this, uiWellDispMarkerEditor,posChgCB );
    CallBack cbchg = mCB( this, uiWellDispMarkerEditor,handleCtrlChangeCB );
    ctrl.mousePressed.remove( cbclk ); 
    ctrl.posChanged.remove( cbpos );
    ctrl.posChanged.remove( cbchg );
    ctrls_ -= &ctrl;
    wds_ -= &wd;
}


void uiWellDispMarkerEditor::handleCtrlChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(BufferString,mesg,caller,cb);
    mDynamicCastGet(uiWellDisplayControl*,ctrl,caller)
    if ( ctrl == curctrl_ )
	{ curmrk_ = curctrl_ ? curctrl_->selMarker() : 0; return; }
    curctrl_ = 0; curmrk_ = 0; curwd_ = 0;
    if ( !ctrl || !ctrl->mouseEventHandler() 
	    	|| !ctrl->mouseEventHandler()->hasEvent() ) return;
    curctrl_ = ctrl;
    int widx = ctrls_.indexOf( ctrl );
    curwd_ =  ( widx >=0 && widx<wds_.size() ) ? wds_[widx] : 0;
    curmrk_ = curctrl_ ? curctrl_->selMarker() : 0;
}


void uiWellDispMarkerEditor::handleUsrClickCB( CallBacker* cb )
{
    handleEditMarker();
    handleMenuMaker();
}


void uiWellDispMarkerEditor::posChgCB( CallBacker* cb )
{
    if ( !isediting_ ) return;
    MouseEventHandler* mevh = curctrl_->mouseEventHandler();
    if ( !mevh || !mevh->hasEvent()  ) return;

    if ( editdlg_ && curctrl_ && editdlg_->isAddRemMode()  )
	editdlg_->grp().setPos( curctrl_->time(), curctrl_->depth() );
}


void uiWellDispMarkerEditor::handleMenuMaker()
{
    if ( !curctrl_ || isediting_ ) return;
    MouseEventHandler* mevh = curctrl_->mouseEventHandler();
    if ( !mevh || !mevh->hasEvent() || mevh->isHandled() ) return;

    const MouseEvent& ev = mevh->event();
    if ( ev.rightButton() && !ev.ctrlStatus() && !ev.shiftStatus() &&
	 !ev.altStatus() )
    {
	if ( !menu_ ) return;
	menu_->executeMenu(0);
	mevh->setHandled( true );
    }
}


void uiWellDispMarkerEditor::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu ) return;
    mAddMenuItem( menu, &startmnuitem_, true, false )
}


void uiWellDispMarkerEditor::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    bool ishandled = true;

    if ( mnuid==startmnuitem_.id ) 
    {
	if ( !editdlg_ )
	    editdlg_ = new uiWellDispEditMarkerDlg( menu_->getParent(), par_ );
	editdlg_->go();
	editdlg_->grp().dispparchg.notify( 
		    mCB(this, uiWellDispMarkerEditor, editMarkerCB));
	editdlg_->windowClosed.notify( 
		    mCB(this, uiWellDispMarkerEditor, editDlgClosedCB));
	isediting_ = true;
    }
    else
	ishandled = false;

    menu->setIsHandled( ishandled );
}

 
void uiWellDispMarkerEditor::handleEditMarker()
{
    if ( !curctrl_ || !isediting_ || !curwd_ ) return;
    bool isaddremmode = editdlg_->isAddRemMode();
    bool ishandled = true;
    MouseEventHandler* mevh = curctrl_->mouseEventHandler();
    if ( !mevh || !mevh->hasEvent() || mevh->isHandled() ) return;
    if ( isaddremmode )
    {
	if ( curctrl_->isCtrlPressed() && curmrk_ )
	    removeMarker();
	else if ( !curctrl_->isCtrlPressed() ) 
	    addNewMarker();
    }
    else if ( curmrk_ )
    {
	lasteditwd_ = curwd_;
	lasteditmrk_ = curmrk_;
	par_.getFromMarker( *curmrk_ );
	editdlg_->grp().putToScreen();
	editdlg_->grp().setFldsSensitive( true );
    }
    else 
	ishandled = false;
    mevh->setHandled( ishandled );
    curwd_->markerschanged.trigger();
}


void uiWellDispMarkerEditor::editDlgClosedCB( CallBacker* )
{
    editFinished.trigger();
    isediting_ = false;
}


void uiWellDispMarkerEditor::addNewMarker()
{
    if ( curwd_->markers().isPresent( par_.name_.buf() ) )
	mErrRet("Marker name already exists", return )
    Well::Marker* mrk = new Well::Marker( par_.name_, par_.dah_ );
    par_.putToMarker( *mrk );
    curwd_->markers().insertNew( mrk );
}


void uiWellDispMarkerEditor::editMarkerCB( CallBacker* )
{
    if ( !isediting_ ) return;
    bool isaddremmode = editdlg_->isAddRemMode();
    if ( !isaddremmode && lasteditmrk_ )
    {
	par_.putToMarker( *lasteditmrk_ );
	if ( lasteditwd_ ) 
	    lasteditwd_->markerschanged.trigger();
    }
}


void uiWellDispMarkerEditor::removeMarker()
{
    if ( !curwd_ ) return;
    ObjectSet<Well::Marker>& mrkobjset = curwd_->markers();
    delete mrkobjset.remove( mrkobjset.indexOf(curmrk_),true );
}


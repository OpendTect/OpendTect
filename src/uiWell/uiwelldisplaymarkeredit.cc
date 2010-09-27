/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Aug 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplaymarkeredit.cc,v 1.6 2010-09-27 11:05:19 cvsbruno Exp $";


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
#include "stratlevel.h"
#include "welld2tmodel.h"
#include "wellmarker.h"
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
    {
	const Strat::Level* lvl = Strat::eLVLS().add( name_.buf(), col_ );
	if ( !lvl && Strat::LVLS().isPresent( name_.buf() ) )
	    lvl =  Strat::LVLS().get( name_.buf() );

	mrk.setLevelID( lvl ? lvl->id() : -1 ); 
    }
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




uiWellDispEditMarkerDlg::uiWellDispEditMarkerDlg( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Edit Markers Dialog",
				"Select editing mode",mTODOHelpID)
				.modal(false))
	, curmrk_(0)
	, curctrl_(0)
	, curwd_(0)	 
	, lasteditwd_(0)
	, lasteditmrk_(0)
	, hasedited_(false)
	, needsave_(false)			   
{
    setOkText( "Ok/Save" );

    modefld_ = new uiGenInput( this, "Select mode", 
			    BoolInpSpec(true,"Add/Remove","Edit Selected") );
    modefld_->valuechanged.notify( mCB(this,uiWellDispEditMarkerDlg,modeChg) );

    uiSeparator* modesep = new uiSeparator( this, "Mode Sep" );
    modesep->attach( stretchedBelow, modefld_ );

    mrkgrp_ = new uiWellDispMarkerEditGrp( this, par_ );
    mrkgrp_->attach( ensureBelow, modefld_ );
    mrkgrp_->attach( ensureBelow, modesep );
    grp().dispparchg.notify(mCB(this,uiWellDispEditMarkerDlg,editMarkerCB));

    windowClosed.notify( mCB(this,uiWellDispEditMarkerDlg,editDlgClosedCB) );
    setMode( true );
}


uiWellDispEditMarkerDlg::~uiWellDispEditMarkerDlg()
{
    deepErase( orgmarkerssets_ );
}


void uiWellDispEditMarkerDlg::editDlgClosedCB( CallBacker* )
{
    for ( int idx=0; idx<ctrls_.size(); idx++ )
	activateSensors( *ctrls_[idx], false );
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


void uiWellDispEditMarkerDlg::addWellCtrl( uiWellDisplayControl& ctrl, 
					      Well::Data& wd  )
{
    ctrls_ += &ctrl;
    wds_ += &wd;
    Well::MarkerSet* orgmrks = new Well::MarkerSet();
    orgmrks->copy( wd.markers() );
    orgmarkerssets_ += orgmrks;
    activateSensors( ctrl, true );
}


void uiWellDispEditMarkerDlg::activateSensors(uiWellDisplayControl& ctr,bool yn)
{
    CallBack cbclk = mCB( this, uiWellDispEditMarkerDlg,handleUsrClickCB );
    CallBack cbpos = mCB( this, uiWellDispEditMarkerDlg,posChgCB );
    CallBack cbchg = mCB( this, uiWellDispEditMarkerDlg,handleCtrlChangeCB );
#define mNotify(action)\
    ctr.posChanged.action( cbchg );\
    ctr.posChanged.action( cbpos );\
    ctr.mousePressed.action( cbclk );\

    if ( yn ) 
	{ mNotify( notify ) }
    else
	{ mNotify( remove ) }
}


void uiWellDispEditMarkerDlg::handleCtrlChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller(BufferString,mesg,caller,cb);
    mDynamicCastGet(uiWellDisplayControl*,ctrl,caller)
    curctrl_ = 0; curmrk_ = 0; curwd_ = 0;
    if ( !ctrl || !ctrl->mouseEventHandler() 
	    	|| !ctrl->mouseEventHandler()->hasEvent() ) return;
    curctrl_ = ctrl;
    int widx = ctrls_.indexOf( ctrl );
    curwd_ = ( widx >=0 && widx<wds_.size() ) ? wds_[widx] : 0;
    if ( curctrl_ && curwd_ )
    {
	ObjectSet<Well::Marker>& mrkset = curwd_->markers();
	int curmrkidx = mrkset.indexOf( curctrl_->selMarker() );
	if ( curmrkidx >=0 )
	    curmrk_ = mrkset[curmrkidx];
    }
}


void uiWellDispEditMarkerDlg::handleUsrClickCB( CallBacker* cb )
{
    handleEditMarker();
}


void uiWellDispEditMarkerDlg::posChgCB( CallBacker* cb )
{
    MouseEventHandler* mevh = curctrl_->mouseEventHandler();
    if ( !mevh || !mevh->hasEvent()  ) return;

    if ( curctrl_ && isAddRemMode()  )
	grp().setPos( curctrl_->time(), curctrl_->depth() );
}



void uiWellDispEditMarkerDlg::handleEditMarker()
{
    if ( !curctrl_ || !curwd_ ) return;
    bool isaddremmode = isAddRemMode();
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
	if ( curwd_->haveD2TModel() )
	    par_.time_ = 1000*curwd_->d2TModel()->getTime( par_.dah_ ); 
	grp().putToScreen();
	grp().setFldsSensitive( true );
    }
    else 
	ishandled = false;
    mevh->setHandled( ishandled );
    hasedited_ = true; 
    curwd_->markerschanged.trigger();
}


void uiWellDispEditMarkerDlg::addNewMarker()
{
    if ( curwd_->markers().isPresent( par_.name_.buf() ) )
	mErrRet("Marker name already exists", return )
    if ( curwd_ && curwd_->haveD2TModel() )
	par_.dah_ =  curwd_->d2TModel()->getDah( par_.time_*0.001 );
    Well::Marker* mrk = new Well::Marker( par_.name_, par_.dah_ );
    par_.putToMarker( *mrk );
    curwd_->markers().insertNew( mrk );
}


void uiWellDispEditMarkerDlg::editMarkerCB( CallBacker* )
{
    bool isaddremmode = isAddRemMode();
    if ( !isaddremmode && lasteditmrk_ )
    {
	if ( curwd_ && curwd_->haveD2TModel() )
	    par_.dah_ =  curwd_->d2TModel()->getDah( par_.time_*0.001 );
	par_.putToMarker( *lasteditmrk_ );
	if ( lasteditwd_ ) 
	    lasteditwd_->markerschanged.trigger();
    }
}


void uiWellDispEditMarkerDlg::removeMarker()
{
    if ( !curwd_ ) return;
    ObjectSet<Well::Marker>& mrkobjset = curwd_->markers();
    delete mrkobjset.remove( mrkobjset.indexOf(curmrk_),true );
}


bool uiWellDispEditMarkerDlg::acceptOK( CallBacker* )
{
    needsave_ = hasedited_;
    return true;
}


bool uiWellDispEditMarkerDlg::rejectOK( CallBacker* )
{
    needsave_ = false;
    if ( hasedited_ )
    {
	BufferString msg = "Some markers have been edited and will be lost. \n";
	msg += "Do you want to abort anyway or continue editing markers ? ";
	if ( !uiMSG().askContinue( msg ) )
	{
	    for ( int idx=0; idx<wds_.size(); idx++ )
	    {
		wds_[idx]->markers() = *orgmarkerssets_[idx];
		wds_[idx]->markerschanged.trigger();
	    }
	    return true;
	}
	else
	    return false;
    }
    return true;
}


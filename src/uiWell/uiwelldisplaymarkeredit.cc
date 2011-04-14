/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Aug 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplaymarkeredit.cc,v 1.22 2011-04-14 09:55:37 cvsbruno Exp $";


#include "uiwelldisplaymarkeredit.h"

#include "uiwelldisplaycontrol.h"
#include "uicolor.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimenuhandler.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiwelllogdisplay.h"

#include "keyboardevent.h"
#include "mouseevent.h"
#include "randcolor.h"
#include "survinfo.h"
#include "sorting.h"
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
	Strat::LevelSet& lvls = Strat::eLVLS();
	const bool ispresent = Strat::LVLS().isPresent( name_.buf() ); 
	const Strat::Level* lvl = ispresent ? lvls.get( name_.buf() )
					    : lvls.add( name_.buf(), col_ );
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
    posfld_->setSensitive( false );
    uiColorInput::Setup csu( getRandStdDrawColor() );
    csu.lbltxt( "Color" ).withdesc(false);
    colorfld_ = new uiColorInput( this, csu, "Color" );
    colorfld_->attach( alignedBelow, posfld_ );

    stratmrkfld_ = new uiCheckBox( this, "Set as regional marker" );
    stratmrkfld_->attach( rightOf, colorfld_ );
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
    stratmrkfld_->setSensitive( yn);
}




uiWellDispEditMarkerDlg::uiWellDispEditMarkerDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Edit Markers Dialog",
			    "Select editing mode","dgb:107.0.3")
			    .modal(false))
    , curmrk_(0)
    , curctrl_(0)
    , curwd_(0)	 
    , hasedited_(false)
    , needsave_(false)			   
{
    setOkText( "Ok/Save" );

    modefld_ = new uiGenInput( this, "Select mode", 
					BoolInpSpec(true,"Add","Remove") );
    modefld_->valuechanged.notify( mCB(this,uiWellDispEditMarkerDlg,modeChg) );

    uiSeparator* modesep = new uiSeparator( this, "Mode Sep" );
    modesep->attach( stretchedBelow, modefld_ );

    mrkgrp_ = new uiWellDispMarkerEditGrp( this, par_ );
    mrkgrp_->attach( ensureBelow, modefld_ );
    mrkgrp_->attach( ensureBelow, modesep );
    grp().dispparchg.notify(mCB(this,uiWellDispEditMarkerDlg,editMarkerCB));

    mrklist_ = new uiListBox( this, "Markers", false );
    mrklist_->attach( ensureBelow, mrkgrp_ );
    mrklist_->rightButtonClicked.notify( 
			    mCB(this,uiWellDispEditMarkerDlg,listRClickCB) );
    mrklist_->selectionChanged.notify( 
			    mCB(this,uiWellDispEditMarkerDlg,listLClickCB) );

    windowClosed.notify( mCB(this,uiWellDispEditMarkerDlg,editDlgClosedCB) );
    setMode( true );
}


uiWellDispEditMarkerDlg::~uiWellDispEditMarkerDlg()
{
    deepErase( tmplist_ );
    deepErase( orgmarkerssets_ );
}


void uiWellDispEditMarkerDlg::editDlgClosedCB( CallBacker* )
{
    for ( int idx=0; idx<ctrls_.size(); idx++ )
	activateSensors( *ctrls_[idx], *wds_[idx], false );
}


void uiWellDispEditMarkerDlg::modeChg( CallBacker* )
{
    bool isaddmode = isAddMode();
    mrkgrp_->setDefault();
    mrkgrp_->setFldsSensitive( isaddmode );

    if ( isaddmode )
    {
	mrklist_->setMultiSelect( false );
	if ( mrklist_->size() )
	    mrklist_->setCurrentItem( 0 );
    }
    else
    {
	mrklist_->clearSelection();
	mrklist_->setNotSelectable();
    }
}


void uiWellDispEditMarkerDlg::setMode( bool addmode )
{
    modefld_->setValue( addmode );
    modeChg( 0 );
}


bool uiWellDispEditMarkerDlg::isAddMode() const
{
    return modefld_->getBoolValue();
}


void uiWellDispEditMarkerDlg::addWellCtrl( uiWellDisplayControl& ctrl, 
					      Well::Data& wd  )
{
    ctrls_ += &ctrl;
    wds_ += &wd;
    Well::MarkerSet* orgmrks = new Well::MarkerSet();
    for ( int idx=0; idx<wd.markers().size(); idx++ )
    {
	(*orgmrks) += new Well::Marker( *wd.markers()[idx] );
    }
    orgmarkerssets_ += orgmrks;
    activateSensors( ctrl, wd, true );
    fillMarkerList( 0 );
}


void uiWellDispEditMarkerDlg::activateSensors(uiWellDisplayControl& ctr, 
						Well::Data& wd, bool yn)
{
    CallBack cbclk = mCB( this, uiWellDispEditMarkerDlg, handleUsrClickCB );
    CallBack cbpos = mCB( this, uiWellDispEditMarkerDlg, posChgCB );
    CallBack cbchg = mCB( this, uiWellDispEditMarkerDlg, handleCtrlChangeCB );
    CallBack cbbox = mCB( this, uiWellDispEditMarkerDlg, fillMarkerList );
#define mNotify(action)\
    ctr.posChanged.action( cbchg );\
    ctr.posChanged.action( cbpos );\
    ctr.mousePressed.action( cbclk );\
    wd.markerschanged.action( cbbox );

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


void uiWellDispEditMarkerDlg::posChgCB( CallBacker* cb )
{
    MouseEventHandler* mevh = curctrl_->mouseEventHandler();
    if ( !mevh || !mevh->hasEvent()  ) return;

    if ( curctrl_ && isAddMode()  )
	grp().setPos( curctrl_->time(), curctrl_->depth() );
}


void uiWellDispEditMarkerDlg::handleUsrClickCB( CallBacker* )
{
    if ( !curctrl_ || !curwd_ ) return;
    bool isaddmode = isAddMode();
    bool ishandled = true;
    MouseEventHandler* mevh = curctrl_->mouseEventHandler();
    if ( !mevh || !mevh->hasEvent() || mevh->isHandled() ) return;

    if ( isaddmode )
	addNewMarker();
    else if ( curmrk_ )
	removeMarker();

    mevh->setHandled( true);
    hasedited_ = true; 
    curwd_->markerschanged.trigger();
}


void uiWellDispEditMarkerDlg::addNewMarker()
{
    if ( curwd_ && curwd_->haveD2TModel() )
	par_.dah_ =  curwd_->d2TModel()->getDah( par_.time_*0.001 );

    const char* mrknm = par_.name_.buf();
    bool ispresent = curwd_->markers().isPresent( mrknm ); 
    Well::Marker* mrk = ispresent ? curwd_->markers().getByName( mrknm  )
				  : new Well::Marker( mrknm, par_.dah_ );
    if ( !mrk ) return;
    par_.putToMarker( *mrk );
    curwd_->markers().insertNew( mrk );

    for ( int idx=0; idx<tmplist_.size(); idx++ )
    {
	if ( !strcmp( mrknm, tmplist_[idx]->name() ) )
	    delete tmplist_.remove( idx );
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
    if ( hasedited_ )
    {
	for ( int idx=0; idx<wds_.size(); idx++ )
	{
	    wds_[idx]->markerschanged.trigger();
	}
    }

    needsave_ = hasedited_;
    return true;
}


bool uiWellDispEditMarkerDlg::rejectOK( CallBacker* )
{
    needsave_ = false;
    if ( hasedited_ )
    {
	BufferString msg = "Some markers have been edited. \n";
	msg += "Do you want to continue with marker editing ? ";
	if ( !uiMSG().askContinue( msg ) )
	{
	    for ( int idx=0; idx<wds_.size(); idx++ )
	    {
		deepErase( wds_[idx]->markers() );
		wds_[idx]->markers().copy( *orgmarkerssets_[idx] );
		wds_[idx]->markerschanged.trigger();
	    }
	    return true;
	}
	else
	    return false;
    }
    return true;
}


mClass addNewMrkDlg : public uiDialog
{
public:
    addNewMrkDlg( uiParent* p, Well::Marker& mrk )
	: uiDialog(this, uiDialog::Setup("Properties",0,mNoHelpID) )
	, marker_(mrk)
    {
	nmfld_ = new uiGenInput( this, "Name", StringInpSpec() );
	colfld_ = new uiColorInput( this,
			uiColorInput::Setup( mrk.color() ).lbltxt("Color") );
	colfld_->attach( alignedBelow, nmfld_ );
    }

    bool acceptOK( CallBacker* )
    {
	marker_.setName( nmfld_->text() );
	marker_.setColor( colfld_->color() );
	return true;
    }	

protected: 

    Well::Marker&	marker_;
    uiGenInput*         nmfld_;
    uiColorInput*       colfld_;
};


void uiWellDispEditMarkerDlg::listRClickCB( CallBacker* )
{
    uiPopupMenu mnu( this, "Action" );
    mnu.insertItem( new uiMenuItem("Add &New ..."), 0 );
    mnu.insertItem( new uiMenuItem("Remove ..."), 1 );
    const int mnuid = mnu.exec();
    if ( mnuid < 0 ) 
	return;
    else if ( mnuid == 0 )
    {
	Well::Marker* mrk = new Well::Marker( "Name", 0 ); 
	mrk->setColor( getRandStdDrawColor() );
	addNewMrkDlg dlg( this, *mrk );
	if ( dlg.go() )
	{
	    tmplist_ += mrk;
	    fillMarkerList(0);
	    if ( mrklist_->isPresent( mrk->name() ) )
		mrklist_->setSelected( mrklist_->indexOf( mrk->name() ) );
	}
	else
	    delete mrk;
    }
    else if ( mnuid == 1 )
    {
	BufferString mrknm = mrklist_->getText();
	BufferString msg = "This will remove "; 
		     msg += mrknm; 
		     msg += " from all the wells \n ";
		     msg += "Do you want to continue ? ";
	if ( uiMSG().askContinue( msg ) )
	{
	    for ( int idx=0; idx<wds_.size(); idx++ )
	    {
		Well::MarkerSet& mrkset = wds_[idx]->markers();
		if ( mrkset.isPresent( mrknm ) )
		{
		    delete mrkset.remove( mrkset.indexOf( mrknm ),true );
		    wds_[idx]->markerschanged.trigger();
		}
	    }
	    hasedited_ = true; 
	}
    }
}


void uiWellDispEditMarkerDlg::listLClickCB( CallBacker* )
{
    int curit = mrklist_->currentItem();
    if ( curit < 0 || curit >= colors_.size() ) 
	return;
    par_.name_ = mrklist_->getText(); 
    par_.col_ = colors_[curit];
    par_.dah_ = 0; 
    grp().putToScreen();
}


void uiWellDispEditMarkerDlg::fillMarkerList( CallBacker* )
{
    const char* selnm = mrklist_->nrSelected() ? mrklist_->getText() : 0;

    if ( mrklist_->size() ) mrklist_->setEmpty();
    BufferStringSet mrknms; TypeSet<Color> mrkcols; TypeSet<float> dahs;

#define mAddMrkToList(mrk)\
    if ( mrknms.addIfNew( mrk.name() ) )\
    {\
	mrkcols += mrk.color();\
	dahs += mrk.dah();\
    }

    for ( int idwd=0; idwd<wds_.size(); idwd++ )
    {
	const Well::Data& wd = *wds_[idwd];
	for ( int idmrk=0; idmrk<wd.markers().size(); idmrk++ )
	{
	    const Well::Marker& mrk = *wd.markers()[idmrk];
	    mAddMrkToList( mrk )
	}
    }
    for ( int idmrk=0; idmrk<tmplist_.size(); idmrk++ )
    {
	const Well::Marker& mrk = *tmplist_[idmrk];
	mAddMrkToList( mrk )
    }
    TypeSet<int> idxs;
    for ( int idx=0; idx<dahs.size(); idx++)
	idxs += idx;
    sort_coupled( dahs.arr(), idxs.arr(), dahs.size() );

    for ( int idx=0; idx<mrknms.size(); idx++ )
    {
	mrklist_->addItem( mrknms.get( idxs[idx] ), mrkcols[idxs[idx]] );
	colors_ += mrkcols[idxs[idx]]; 
    }

    if ( mrklist_->isEmpty() )
    {
	Well::Marker* mrk = new Well::Marker( "Name", 0 ); 
	mrk->setColor( getRandStdDrawColor() );
	    tmplist_ += mrk;
    }
    if ( isAddMode() )
    {
	const int selidx = mrklist_->indexOf( selnm );
	if ( selidx < mrklist_->size() && selidx >= 0 )
	    mrklist_->setCurrentItem( selidx );
    }
}


void uiWellDispEditMarkerDlg::editMarkerCB( CallBacker* )
{
    BufferString mrknm = mrklist_->getText();
    for ( int idwd=0; idwd<wds_.size(); idwd++ )
    {
	Well::MarkerSet& mrkset = wds_[idwd]->markers();
	Well::Marker* mrk = mrkset.getByName( mrknm );
	if ( mrk )
	{
	    mrk->setName( par_.name_ );
	    mrk->setColor( par_.col_ );
	    wds_[idwd]->markerschanged.trigger();
	    mrklist_->setCurrentItem( par_.name_ );
	}
    }
}


/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Aug 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisplaymarkeredit.cc,v 1.23 2012-04-20 16:00:12 cvsbruno Exp $";


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
#include "uitoolbutton.h"
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

#define mErrRet(msg,act) { uiMSG().error( msg ); act; }
uiAddEditMrkrDlg::uiAddEditMrkrDlg( uiParent* p, Well::Marker& mrk )
    : uiDialog(p,uiDialog::Setup("Edit Markers",mNoDlgTitle,mNoHelpID))
    , marker_(mrk) 
{
    namefld_ = new uiGenInput( this, "Name", StringInpSpec("Marker") );

    bool istime = SI().zIsTime();
    uiColorInput::Setup csu( getRandStdDrawColor() );
    csu.lbltxt( "Color" ).withdesc(false);
    colorfld_ = new uiColorInput( this, csu, "Color" );
    colorfld_->attach( alignedBelow, namefld_ );

    stratmrkfld_ = new uiCheckBox( this, "Set as regional marker" );
    stratmrkfld_->attach( rightOf, colorfld_ );
    stratmrkfld_->setChecked( true );

    putToScreen();
}


bool uiAddEditMrkrDlg::acceptOK( CallBacker* )
{
    BufferString nm = namefld_->text();
    if ( nm.isEmpty() )
	mErrRet( "Please specify a marker name", return false );

    marker_.setName( nm );
    marker_.setColor( colorfld_->color() );

    if ( stratmrkfld_->isChecked() )
    {
	Strat::LevelSet& lvls = Strat::eLVLS();
	const bool ispresent = Strat::LVLS().isPresent( nm.buf() ); 
	const Strat::Level* lvl = ispresent ? 
	    lvls.get( nm.buf() ) : lvls.add( nm.buf(), colorfld_->color() );
	marker_.setLevelID( lvl ? lvl->id() : -1 ); 
    }

    return true;
}


void uiAddEditMrkrDlg::putToScreen()
{
    namefld_->setText( marker_.name() );
    colorfld_->setColor( marker_.color() );
    stratmrkfld_->setChecked( marker_.levelID() > 0 );
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

    modefld_ = new uiGenInput( this, "Select picking mode", 
			BoolInpSpec(true,"Add/Move","Remove") );
    modefld_->valuechanged.notify( mCB(this,uiWellDispEditMarkerDlg,modeChg) );

    uiSeparator* modesep = new uiSeparator( this, "Mode Sep" );
    modesep->attach( stretchedBelow, modefld_ );

    mrklist_ = new uiListBox( this, "Markers", false );
    mrklist_->attach( ensureBelow, modefld_ );
    mrklist_->attach( ensureBelow, modesep );
    mrklist_->rightButtonClicked.notify( 
			    mCB(this,uiWellDispEditMarkerDlg,listRClickCB) );

    CallBack butcb( mCB(this,uiWellDispEditMarkerDlg,buttonPushedCB) );
    addbut_ = new uiToolButton( this, "plus.png", "Add Marker", butcb );
    addbut_->attach( rightOf, mrklist_ );
    editbut_ = new uiToolButton( this, "edit.png", "Edit Marker", butcb );
    editbut_->attach( alignedBelow, addbut_ );
    rembut_ = new uiToolButton(this, "trashcan.png", "Remove Marker", butcb);
    rembut_->attach( alignedBelow, editbut_ );

    windowClosed.notify( mCB(this,uiWellDispEditMarkerDlg,editDlgClosedCB) );
    setMode( true );
}


uiWellDispEditMarkerDlg::~uiWellDispEditMarkerDlg()
{
    deepErase( tobeadded_ );
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
}


void uiWellDispEditMarkerDlg::handleUsrClickCB( CallBacker* )
{
    if ( !curctrl_ || !curwd_ ) return;
    bool isaddmode = isAddMode();
    bool ishandled = true;
    MouseEventHandler* mevh = curctrl_->mouseEventHandler();
    if ( !mevh || !mevh->hasEvent() || mevh->isHandled() ) return;

    if ( isaddmode )
	addNewMrkr();
    else if ( curmrk_ )
	removeMrkr();

    mevh->setHandled( true);
    hasedited_ = true; 
    curwd_->markerschanged.trigger();
}


void uiWellDispEditMarkerDlg::addMoveMarker()
{
    if ( curwd_ && curwd_->haveD2TModel() )
	dah_ =  curwd_->d2TModel()->getDah( time_*0.001 );

    const char* mrknm = mrklist_->getText();
    bool ispresent = curwd_->markers().isPresent( mrknm ); 
    Well::Marker* mrk = ispresent ? curwd_->markers().getByName( mrknm  )
				  : new Well::Marker( mrknm, dah_ );
    if ( !mrk ) return;
    curwd_->markers().insertNew( mrk );

    for ( int idx=0; idx<tobeadded_.size(); idx++ )
    {
	if ( !strcmp( mrknm, tobeadded_[idx]->name() ) )
	    delete tobeadded_.remove( idx );
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


void uiWellDispEditMarkerDlg::buttonPushedCB( CallBacker* cb )
{
    mDynamicCastGet(uiToolButton*,tb,cb);
    if ( tb == addbut_ )
	addNewMrkr();
    else if ( tb == editbut_ )
	editMrkr();
    else if ( tb == rembut_ ) 
	removeMrkr();
}


void uiWellDispEditMarkerDlg::addNewMrkr()
{
    Well::Marker* mrk = new Well::Marker( "Name", 0 ); 
    mrk->setColor( getRandStdDrawColor() );
    uiAddEditMrkrDlg dlg( this, *mrk );
    if ( dlg.go() )
    {
	tobeadded_ += mrk;
	fillMarkerList(0);
	if ( mrklist_->isPresent( mrk->name() ) )
	    mrklist_->setSelected( mrklist_->indexOf( mrk->name() ) );
    }
    else
	delete mrk;
}


void uiWellDispEditMarkerDlg::editMrkr()
{
    const int selidx = mrklist_->currentItem();
    if ( selidx < 0 ) 
	return;

    Well::Marker* mrk = new Well::Marker( mrklist_->getText(), 0 ); 
    mrk->setColor( mrklist_->getColor( selidx ) );
    const Strat::Level* lvl = Strat::LVLS().get( mrklist_->getText() );
    mrk->setLevelID( lvl ? lvl->id() : -1 );

    uiAddEditMrkrDlg dlg( this, *mrk );

    if ( !dlg.go() )
	return;

    BufferString mrknm = mrk->name();
    Well::Marker* wdmrk = 0;

#define mSetMrkFromMrk()\
    wdmrk->setName( mrk->name() );\
    wdmrk->setColor( mrk->color() );\
    wdmrk->setLevelID( mrk->levelID() );
    for ( int idwd=0; idwd<wds_.size(); idwd++ )
    {
	Well::MarkerSet& mrkset = wds_[idwd]->markers();
	wdmrk = mrkset.getByName( mrknm );
	if ( wdmrk )
	{
	    mSetMrkFromMrk();
	    wds_[idwd]->markerschanged.trigger();
	}
    }
    for ( int idx=0; idx<tobeadded_.size(); idx++ )
    {
	if ( !strcmp( mrknm, tobeadded_[idx]->name() ) )
	{
	    mSetMrkFromMrk();
	}
    }
    if ( wdmrk )
    {	
	mrklist_->setCurrentItem( wdmrk->name() );
	hasedited_ = true; 
    }
    delete mrk;
}


void uiWellDispEditMarkerDlg::removeMrkr()
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


void uiWellDispEditMarkerDlg::listRClickCB( CallBacker* )
{
    uiPopupMenu mnu( this, "Action" );

    const bool nomrkr = mrklist_->isEmpty();

    mnu.insertItem( new uiMenuItem("Add &New ..."), 0 );
    if ( !nomrkr )
    {
	mnu.insertItem( new uiMenuItem("&Edit ..."), 1 );
	mnu.insertItem( new uiMenuItem("Remove ..."), 2 );
    }
    const int mnuid = mnu.exec();
    if ( mnuid < 0 ) 
	return;
    else if ( mnuid == 0 )
    {
	addNewMrkr();
    }
    else if ( mnuid == 1 )
    {
	editMrkr();
    }
    else if ( mnuid ==2 )
    {
	removeMrkr();
    }
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
    for ( int idmrk=0; idmrk<tobeadded_.size(); idmrk++ )
    {
	const Well::Marker& mrk = *tobeadded_[idmrk];
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
	    tobeadded_ += mrk;
    }
    if ( isAddMode() )
    {
	const int selidx = mrklist_->indexOf( selnm );
	if ( selidx < mrklist_->size() && selidx >= 0 )
	    mrklist_->setCurrentItem( selidx );
    }
}



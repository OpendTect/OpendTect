/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Nov 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisppropdlg.cc,v 1.25 2010-03-05 10:13:35 cvsbruno Exp $";

#include "uiwelldisppropdlg.h"

#include "uiwelldispprop.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uitabstack.h"
#include "uiseparator.h"

#include "welldata.h"
#include "welldisp.h"

#include "keystrs.h"


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data* d )
	: uiDialog(p,uiDialog::Setup("Well display properties",
	   "","107.2.0").savetext("Save as default").savebutton(true)
					.savechecked(false)
				       	.modal(false))
	, wd_(d)
	, props_(d->displayProperties())
	, applyAllReq(this)
	, savedefault_(false)
{
    setCtrlStyle( LeaveOnly );

    wd_->dispparschanged.notify( mCB(this,uiWellDispPropDlg,wdChg) );

    ts_ = new uiTabStack( this, "Well display porperties tab stack" );
    ObjectSet<uiGroup> tgs;
    tgs += new uiGroup( ts_->tabGroup(), "Left log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Right Log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Track properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Marker properties" );

    propflds_ += new uiWellLogDispProperties( tgs[0],
		    uiWellDispProperties::Setup( "Line thickness", "Line color")		    ,props_.left_, &(wd_->logs()) );
    propflds_ += new uiWellLogDispProperties( tgs[1],
		    uiWellDispProperties::Setup( "Line thickness", "Line color")		    ,props_.right_, &(wd_->logs()) );
    bool foundlog = false;
    propflds_ += new uiWellTrackDispProperties( tgs[2],
		    uiWellDispProperties::Setup(), props_.track_ );
    propflds_ += new uiWellMarkersDispProperties( tgs[3],
		    uiWellDispProperties::Setup( "Marker size", "Marker color" )		    ,props_.markers_ );
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	propflds_[idx]->propChanged.notify(
					mCB(this,uiWellDispPropDlg,propChg) );
	if ( !strcmp( sKey::Log, propflds_[idx]->props().subjectName() ) )
	{
	    ts_->addTab( tgs[idx], foundlog ? "Right Log" : "Left Log" );
	    foundlog = true;
	}
	else
	    ts_->addTab( tgs[idx], propflds_[idx]->props().subjectName() );
    }
    uiPushButton* applbut = new uiPushButton( this, "&Apply to all wells",
			mCB(this,uiWellDispPropDlg,applyAllPush), true );
    applbut->attach( centeredBelow, ts_ );
    wd_->tobedeleted.notify( mCB(this,uiWellDispPropDlg,welldataDelNotify) );
}


uiWellDispPropDlg::~uiWellDispPropDlg()
{
}


void uiWellDispPropDlg::getFromScreen()
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->getFromScreen();
}


void uiWellDispPropDlg::putToScreen()
{
    for ( int idx=0; idx<propflds_.size(); idx++ )
	propflds_[idx]->putToScreen();
}


void uiWellDispPropDlg::wdChg( CallBacker* )
{
    NotifyStopper ns( wd_->dispparschanged );
    putToScreen();
}


void uiWellDispPropDlg::propChg( CallBacker* )
{
    getFromScreen();
    wd_->dispparschanged.trigger();
}


void uiWellDispPropDlg::applyAllPush( CallBacker* )
{
    getFromScreen();
    applyAllReq.trigger();
}


void uiWellDispPropDlg::welldataDelNotify( CallBacker* )
{
    wd_->tobedeleted.remove( mCB(this,uiWellDispPropDlg,welldataDelNotify) );
    close();
}


bool uiWellDispPropDlg::rejectOK( CallBacker* )
{
    if ( saveButtonChecked() )
	savedefault_ = true;
    else 
	savedefault_ = false;
    return true;
}




uiMultiWellDispPropDlg::uiMultiWellDispPropDlg( uiParent* p, 
					        ObjectSet<Well::Data> d )
	: uiWellDispPropDlg(p,d[0])
	, wds_(d)
	, wdChged(this)	 
	, wellselfld_(0)	 
{
    uiLabeledComboBox* lcb = 0;
    if ( wds_.size()>1 )
    {
	BufferStringSet wellnames;
	for ( int idx=0; idx< wds_.size(); idx++ )
	    wellnames.addIfNew( wds_[idx]->name() );
	
	lcb = new uiLabeledComboBox( this, "Select Well" );
	wellselfld_ = lcb->box();
	wellselfld_->addItems( wellnames );
	wellselfld_->selectionChanged.notify(
		    mCB(this,uiMultiWellDispPropDlg,wellSelChg) );
	wellselfld_ = lcb->box();
	lcb->attach( centeredAbove,ts_ );

	uiSeparator* sep = new uiSeparator( this, "Well Sel/Log Sel Sep" );
	sep->attach( stretchedBelow, lcb );
	setVSpacing( 25 );
    }
}


void uiMultiWellDispPropDlg::wellSelChg( CallBacker* )
{
    const int selidx = wellselfld_ ? wellselfld_->currentItem() : 0;
    wd_ = wds_[selidx];
    if ( !wd_ ) return;
    wd_->dispparschanged.notify( mCB(this,uiMultiWellDispPropDlg,wdChg) );
    wd_->tobedeleted.notify( mCB(this,uiMultiWellDispPropDlg,welldataDelNotify) );
    bool foundleftlog = false;
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet(Well::DisplayProperties::Log*,logpp,
						&propflds_[idx]->props() );
	mDynamicCastGet(Well::DisplayProperties::Track*,trcpp,
						&propflds_[idx]->props() );
	mDynamicCastGet(Well::DisplayProperties::Markers*,mrkpp,
						&propflds_[idx]->props() );
	if ( logpp )
	{
	    propflds_[idx]->resetProps( foundleftlog ? 
		    			wd_->displayProperties().right_ :
		    			wd_->displayProperties().left_ );
	    foundleftlog = true;
	}
	else if ( trcpp )
	    propflds_[idx]->resetProps( wd_->displayProperties().track_ );
	else if ( mrkpp )
	    propflds_[idx]->resetProps( wd_->displayProperties().markers_ );
    }
}



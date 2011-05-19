/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Nov 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisppropdlg.cc,v 1.30 2011-05-19 15:02:05 cvsbruno Exp $";

#include "uiwelldisppropdlg.h"

#include "uiwelldispprop.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uitabstack.h"
#include "uiseparator.h"

#include "welldata.h"
#include "welldisp.h"

#include "keystrs.h"

#define mDispNot (is2ddisplay_? wd_->disp2dparschanged : wd_->disp3dparschanged)
uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data* d, bool is2ddisp)
	: uiDialog(p,uiDialog::Setup("Well display properties",
	   "","107.2.0").savetext("Save as default").savebutton(true)
					.savechecked(false)
				       	.modal(false))
	, wd_(d)
	, applyAllReq(this)
	, savedefault_(false)
	, is2ddisplay_(is2ddisp)		     
{
    setCtrlStyle( LeaveOnly );

    Well::DisplayProperties& props = d->displayProperties( is2ddisplay_ );
    mDispNot.notify( mCB(this,uiWellDispPropDlg,wdChg) );

    ts_ = new uiTabStack( this, "Well display porperties tab stack" );
    ObjectSet<uiGroup> tgs;
    tgs += new uiGroup( ts_->tabGroup(), "Left log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Right Log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Track properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Marker properties" );

    propflds_ += new uiWellLogDispProperties( tgs[0],
		    uiWellDispProperties::Setup( "Line thickness", "Line color")		    ,props.logs_[0]->left_, &(wd_->logs()) );
    propflds_ += new uiWellLogDispProperties( tgs[1],
		    uiWellDispProperties::Setup( "Line thickness", "Line color")		    ,props.logs_[0]->right_, &(wd_->logs()) );
    bool foundlog = false;
    propflds_ += new uiWellTrackDispProperties( tgs[2],
		    uiWellDispProperties::Setup(), props.track_ );
    propflds_ += new uiWellMarkersDispProperties( tgs[3],
		    uiWellDispProperties::Setup( "Marker size", "Marker color" )		    ,props.markers_ );
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
    NotifyStopper ns( mDispNot );
    putToScreen();
}


void uiWellDispPropDlg::propChg( CallBacker* )
{
    getFromScreen();
    mDispNot.trigger();
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
					        ObjectSet<Well::Data>& wds,
       						bool is2ddisplay )
	: uiWellDispPropDlg(p,wds[0],is2ddisplay)
	, wds_(wds)
	, wdChged(this)	 
	, wellselfld_(0)	 
{
    if ( wds_.size()>1 )
    {
	BufferStringSet wellnames;
	for ( int idx=0; idx< wds_.size(); idx++ )
	    wellnames.addIfNew( wds_[idx]->name() );
	
	wellselfld_ = new uiLabeledComboBox( this, "Select Well" );
	wellselfld_->box()->addItems( wellnames );
	wellselfld_->box()->selectionChanged.notify(
		    mCB(this,uiMultiWellDispPropDlg,wellSelChg) );
	wellselfld_->attach( hCentered );
	ts_->attach( ensureBelow, wellselfld_ );
    }
}


uiMultiWellDispPropDlg::~uiMultiWellDispPropDlg()
{
    mDispNot.remove( mCB(this,uiMultiWellDispPropDlg,wdChg) );
    wd_->tobedeleted.remove(mCB(this,uiMultiWellDispPropDlg,welldataDelNotify));
}


void uiMultiWellDispPropDlg::resetProps( Well::DisplayProperties& prop,
       					 const Well::LogSet* wls )
{
    bool foundleftlog = false;
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	mDynamicCastGet( uiWellTrackDispProperties*,trckfld,propflds_[idx] );
	mDynamicCastGet( uiWellMarkersDispProperties*,mrkfld,propflds_[idx] );
	mDynamicCastGet( uiWellLogDispProperties*,logfld,propflds_[idx] );
	if ( logfld )
	{
	    logfld->setLogSet( wls );
	    logfld->resetProps( 
		    foundleftlog? prop.logs_[0]->right_: prop.logs_[0]->left_);
	    foundleftlog = true;
	}
	else if ( trckfld )
	    trckfld->resetProps( prop.track_ );
	else if ( mrkfld )
	    mrkfld->resetProps( prop.markers_ );
    }
}


void uiMultiWellDispPropDlg::wellSelChg( CallBacker* )
{
    const int selidx = wellselfld_ ? wellselfld_->box()->currentItem() : 0;
    wd_ = wds_[selidx];
    if ( !wd_ ) return;
    mDispNot.notify( mCB(this,uiMultiWellDispPropDlg,wdChg) );
    wd_->tobedeleted.notify(mCB(this,uiMultiWellDispPropDlg,welldataDelNotify));
    resetProps( wd_->displayProperties(), &wd_->logs() );
}



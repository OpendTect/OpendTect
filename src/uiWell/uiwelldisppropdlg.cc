/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Nov 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisppropdlg.cc,v 1.16 2009-06-16 10:39:55 cvsbert Exp $";

#include "uiwelldisppropdlg.h"

#include "uiwelldispprop.h"
#include "uibutton.h"
#include "uitabstack.h"

#include "welldata.h"
#include "welldisp.h"

#include "keystrs.h"


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data& d )
	: uiDialog(p,uiDialog::Setup("Well display properties",
	   "",mTODOHelpID).savetext("Save as default").savebutton(true)
					.savechecked(false)
				       	.modal(false))
	, wd_(d)
	, props_(d.displayProperties())
	, applyAllReq(this)
	, savedefault_(false)		   
{
    setCtrlStyle( LeaveOnly );
    wd_.dispparschanged.notify( mCB(this,uiWellDispPropDlg,wdChg) );

    ts_ = new uiTabStack( this, "Well display porperties tab stack" );
    ObjectSet<uiGroup> tgs;
    tgs += new uiGroup( ts_->tabGroup(), "Track properties" );
    tgs +=  new uiGroup( ts_->tabGroup(), "Marker properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Left log properties" );
    tgs +=  new uiGroup( ts_->tabGroup(), "Right Log properties" );

    propflds_ += new uiWellTrackDispProperties( tgs[0],
		    uiWellDispProperties::Setup(), props_.track_ );
    propflds_ += new uiWellMarkersDispProperties( tgs[1],
		    uiWellDispProperties::Setup( "Marker size", "Marker color" )		    ,props_.markers_ );
    propflds_ += new uiWellLogDispProperties( tgs[2],
		    uiWellDispProperties::Setup( "Line thickness", "Line color")		    ,props_.left_, &(wd_.logs()) );
    propflds_ += new uiWellLogDispProperties( tgs[3],
		    uiWellDispProperties::Setup( "Line thickness", "Line color")		    ,props_.right_, &(wd_.logs()) );
  
    bool foundlog = false;
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

}


uiWellDispPropDlg::~uiWellDispPropDlg()
{
    wd_.dispparschanged.remove( mCB(this,uiWellDispPropDlg,wdChg) );
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
    NotifyStopper ns( wd_.dispparschanged );
    putToScreen();
}


void uiWellDispPropDlg::propChg( CallBacker* )
{
    getFromScreen();
    wd_.dispparschanged.trigger();
}


void uiWellDispPropDlg::applyAllPush( CallBacker* )
{
    getFromScreen();
    applyAllReq.trigger();
}

//TODO remove function
bool uiWellDispPropDlg::rejectOK( CallBacker* )
{
    if ( saveButtonChecked() )
	savedefault_ = true;
    else 
	savedefault_ = false;
    return true;
}




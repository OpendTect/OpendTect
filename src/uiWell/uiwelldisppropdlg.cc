/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Nov 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisppropdlg.cc,v 1.3 2008-12-10 10:03:21 cvsbruno Exp $";

#include "uiwelldisppropdlg.h"

#include "uiwelldispprop.h"
#include "uibutton.h"
#include "uitabstack.h"

#include "welldata.h"
#include "welldisp.h"

#include "keystrs.h"


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data& d )
	: uiDialog(p,uiDialog::Setup("Well display properties",
				     "",mTODOHelpID))
	, wd_(d)
	, props_(d.displayProperties())
    	, orgprops_(new Well::DisplayProperties(d.displayProperties()))
	, applyAllReq(this)
{
    wd_.dispparschanged.notify( mCB(this,uiWellDispPropDlg,wdChg) );

    ts_ = new uiTabStack( this, "Well display porperties tab stack" );
    ObjectSet<uiGroup> tgs;
    tgs += new uiGroup( ts_->tabGroup(), "Left log properties" );
    tgs +=  new uiGroup( ts_->tabGroup(), "Right Log properties" );
    tgs += new uiGroup( ts_->tabGroup(), "Track properties" );
    tgs +=  new uiGroup( ts_->tabGroup(), "Marker properties" );

    propflds_ += new uiWellLogDispProperties( tgs[0],
		    uiWellDispProperties::Setup( "Line thickness", "Line Color" ),
		    props_.left_ );
    propflds_ += new uiWellLogDispProperties( tgs[1],
		    uiWellDispProperties::Setup( "Line thickness", "Line Color" ),
		    props_.right_ );
    propflds_ += new uiWellTrackDispProperties( tgs[2],
		    uiWellDispProperties::Setup(), props_.track_ );
    propflds_ += new uiWellMarkersDispProperties( tgs[3],
		    uiWellDispProperties::Setup( "Marker size", "Marker color" ),
		    props_.markers_ );

    bool foundlog = false;
    for ( int idx=0; idx<propflds_.size(); idx++ )
    {
	propflds_[idx]->propChanged.notify( mCB(this,uiWellDispPropDlg,propChg) );
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
    delete orgprops_;
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
    *orgprops_ = props_;
}


bool uiWellDispPropDlg::rejectOK( CallBacker* )
{
    props_ = *orgprops_;
    wd_.dispparschanged.trigger();
    return true;
}


bool uiWellDispPropDlg::acceptOK( CallBacker* )
{
    getFromScreen();
    wd_.dispparschanged.trigger();
    return true;
}

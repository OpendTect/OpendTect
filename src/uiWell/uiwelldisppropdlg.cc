/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Nov 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelldisppropdlg.cc,v 1.2 2008-12-08 12:51:59 cvsbert Exp $";

#include "uiwelldisppropdlg.h"

#include "uiwelldispprop.h"
#include "uibutton.h"
#include "uitabstack.h"

#include "welldata.h"
#include "welldisp.h"


uiWellDispPropDlg::uiWellDispPropDlg( uiParent* p, Well::Data& d )
	: uiDialog(p,uiDialog::Setup("Well display properties",
				     "",mTODOHelpID))
	, wd_(d)
	, props_(d.displayProperties())
    	, orgprops_(new Well::DisplayProperties(d.displayProperties()))
{
    ts_ = new uiTabStack( this, "Well display porperties tab stack" );
    uiGroup* tg = ts_->tabGroup();
    propflds_ += new uiWellTrackDispProperties( tg,
		    uiWellDispProperties::Setup(), props_.track_ );
    propflds_ += new uiWellMarkersDispProperties( tg,
		    uiWellDispProperties::Setup("Marker size","Marker color"),
		    props_.markers_ );

    for ( int idx=0; idx<propflds_.size(); idx++ )
	ts_->addTab( propflds_[idx], propflds_[idx]->props().subjectName() );

    uiPushButton* applbut = new uiPushButton( this, "&Apply",
			mCB(this,uiWellDispPropDlg,applyPush), true );
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


void uiWellDispPropDlg::applyPush( CallBacker* )
{
    getFromScreen();
    wd_.dispparschanged.trigger();
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

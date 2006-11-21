/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2006
 RCS:           $Id: uidlggroup.cc,v 1.1 2006-11-21 20:21:47 cvskris Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

#include "uimsg.h"
#include "uitabstack.h"

uiTabStackDlg::uiTabStackDlg( uiParent* p, const uiDialog::Setup& setup )
    : uiDialog( p, setup )
    , canrevert_( true )
{
    tabstack_ = new uiTabStack( this, "TabStack" );
}


uiParent* uiTabStackDlg::tabParent()
{ return tabstack_->tabGroup(); }


void uiTabStackDlg::addGroup( uiDlgGroup* grp )
{
    groups_ += grp;
    tabstack_->addTab( grp );
}


bool uiTabStackDlg::acceptOK(CallBacker*)
{
    for ( int idx=groups_.size()-1; idx>=0; idx-- )
    {
	if ( !groups_[idx]->acceptOK() )
	{
	    const char* errmsg = groups_[idx]->errMsg();

	    for ( idx++; idx<groups_.size(); idx++ )
		groups_[idx]->revertChanges();

	    if ( errmsg )
		 uiMSG().error( errmsg );

	    return false;
	}
    }

    return true;
}


bool uiTabStackDlg::rejectOK(CallBacker*)
{
    for ( int idx=groups_.size()-1; idx>=0; idx-- )
    {
	if ( !groups_[idx]->rejectOK() )
	{
	    const char* errmsg = groups_[idx]->errMsg();
	    if ( errmsg )
		 uiMSG().error( errmsg );

	    return false;
	}
    }

    return true;
}

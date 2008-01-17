/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          November 2006
 RCS:           $Id: uidlggroup.cc,v 1.4 2008-01-17 16:05:00 cvskris Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

#include "uimsg.h"
#include "uitabstack.h"

uiTabStackDlg::uiTabStackDlg( uiParent* p, const uiDialog::Setup& dlgsetup )
    : uiDialog( p, dlgsetup )
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


void uiTabStackDlg::showGroup( int idx )
{
    tabstack_->setCurrentPage( groups_[idx] );
}


bool uiTabStackDlg::acceptOK(CallBacker*)
{
    for ( int idx=groups_.size()-1; idx>=0; idx-- )
    {
	tabstack_->setCurrentPage( groups_[idx] );
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

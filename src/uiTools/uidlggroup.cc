/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2006
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
    tabstack_->selChange().notify( mCB(this,uiTabStackDlg,selChange));

    postFinalise().notify( mCB( this,uiTabStackDlg,selChange));
}



uiTabStackDlg::~uiTabStackDlg()
{
}


void uiTabStackDlg::selChange( CallBacker* )
{
    const HelpKey& helpkey = helpKey();
    setButtonSensitive( uiDialog::HELP, !helpkey.isEmpty() );
}


HelpKey uiTabStackDlg::helpKey() const
{
    mDynamicCastGet(const uiDlgGroup*,grp,tabstack_->currentPage())
    return grp && !grp->helpKey().isEmpty()
		? grp->helpKey() : uiDialog::helpKey();
}


uiParent* uiTabStackDlg::tabParent()
{ return tabstack_->tabGroup(); }


void uiTabStackDlg::addGroup( uiDlgGroup* grp )
{
    groups_ += grp;
    tabstack_->addTab( grp, grp->getCaption() );
}


void uiTabStackDlg::showGroup( int idx )
{
    if ( groups_.validIdx(idx) )
	tabstack_->setCurrentPage( groups_[idx] );
}


bool uiTabStackDlg::acceptOK(CallBacker*)
{
    const int curpage = currentGroupID();
    for ( int idx=groups_.size()-1; idx>=0; idx-- )
    {
	tabstack_->setCurrentPage( groups_[idx] );
	if ( !groups_[idx]->acceptOK() )
	{
	    const char* errmsg = groups_[idx]->errMsg();

	    for ( idx++; idx<groups_.size(); idx++ )
		groups_[idx]->revertChanges();

	    if ( errmsg )
		 uiMSG().error( mToUiStringTodo(errmsg) );

	    return false;
	}
    }
    showGroup( curpage );

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
		 uiMSG().error( mToUiStringTodo(errmsg) );

	    return false;
	}
    }

    return true;
}

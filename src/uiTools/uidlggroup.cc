/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

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
    const char* helpid = helpID();
    setButtonSensitive( uiDialog::HELP, helpid && *helpid );
}


const char* uiTabStackDlg::helpID() const
{
    const char* helpid = uiDialog::helpID();
    if ( helpid && *helpid )
	return helpid;

    bool hassomehelp = false;
    for ( int idx=0; idx<tabstack_->size(); idx++ )
    {
	mDynamicCastGet( const uiDlgGroup*, grp, tabstack_->page(idx) );
	if ( grp->helpID() )
	{
	    hassomehelp = true;
	    break;
	}
    }

    mDynamicCastGet( const uiDlgGroup*, grp, tabstack_->currentPage() );
    if ( grp && grp->helpID() )
	return grp->helpID();

    return hassomehelp ? "" : 0;
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
		 uiMSG().error( errmsg );

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
		 uiMSG().error( errmsg );

	    return false;
	}
    }

    return true;
}

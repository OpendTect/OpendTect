/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidlggroup.h"

#include "uimsg.h"
#include "uitabstack.h"


// uiDlgGroup
uiDlgGroup::uiDlgGroup(uiParent* p,const uiString& caption )
    : uiGroup( p, caption.getFullString() )
    , caption_( caption )
{}


uiDlgGroup::~uiDlgGroup()
{}



// uiSingleGroupDlg
uiSingleGroupDlg::uiSingleGroupDlg( uiParent* p, const uiDialog::Setup& su )
    : uiDialog(p,su)
{}


uiSingleGroupDlg::~uiSingleGroupDlg()
{}


HelpKey uiSingleGroupDlg::helpKey() const
{
    if ( !grp_->helpKey().isEmpty() )
	return grp_->helpKey();

    return uiDialog::helpKey();
}



// uiTabStackDlg
uiTabStackDlg::uiTabStackDlg( uiParent* p, const uiDialog::Setup& dlgsetup )
    : uiDialog( p, dlgsetup )
    , canrevert_( true )
{
    tabstack_ = new uiTabStack( this, "TabStack" );
    tabstack_->selChange().notify( mCB(this,uiTabStackDlg,selChange));

    postFinalize().notify( mCB( this,uiTabStackDlg,selChange));
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
	    const uiString errmsg = groups_[idx]->errMsg();

	    for ( idx++; idx<groups_.size(); idx++ )
		groups_[idx]->revertChanges();

	    if ( !errmsg.isEmpty() )
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
	    const uiString errmsg = groups_[idx]->errMsg();
	    if ( !errmsg.isEmpty() )
		 uiMSG().error( errmsg );

	    return false;
	}
    }

    return true;
}

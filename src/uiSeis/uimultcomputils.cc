/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          August 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimultcomputils.cc,v 1.5 2009-03-18 11:04:03 cvshelene Exp $";

#include "uimultcomputils.h"
#include "bufstringset.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "uigeninput.h"
#include "uilistbox.h"


uiMultCompDlg::uiMultCompDlg( uiParent* p, LineKey lkey )
    	: uiDialog(p,uiDialog::Setup("Component dialog",
		    		     "Choose the component to display","") )
	, needdisplay_(true)
	, compfld_(0)
{
    BufferStringSet complist;
    SeisIOObjInfo::getCompNames( lkey, complist );

    if ( complist.size()>1 )
	compfld_ = new uiListBox( this, complist, "Component nr" );
    else
	needdisplay_ = false;
}


int uiMultCompDlg::getCompNr() const
{
    return compfld_->indexOf( compfld_->getText() );
}


uiMultCompSel::uiMultCompSel( uiParent* p )
    : uiCompoundParSel(p,"Components subselection")
    , dlg_(0)
{
}


uiMultCompSel::~uiMultCompSel()
{
    if ( dlg_ ) delete dlg_;
}


void uiMultCompSel::setUpList( LineKey lkey )
{
    compnms_.erase();
    SeisIOObjInfo::getCompNames( lkey, compnms_ );
    butPush.notify( mCB(this,uiMultCompSel,doDlg) );
    prepareDlg();
}


void uiMultCompSel::setUpList( const BufferStringSet& bfsset )
{
    compnms_ = bfsset;
    butPush.notify( mCB(this,uiMultCompSel,doDlg) );
    prepareDlg();
}


void uiMultCompSel::prepareDlg()
{
    if ( dlg_ )
    {
	dlg_->outlistfld_->box()->empty();
	dlg_->outlistfld_->box()->addItems( compnms_ );
    }
    else
	dlg_ = new MCompDlg( this, compnms_ );
}


void uiMultCompSel::doDlg( CallBacker* )
{
    if ( !dlg_ ) return;
    dlg_->selChg(0);
    dlg_->go();
}


BufferString uiMultCompSel::getSummary() const
{
    BufferString ret;
    if ( !allowChoice() || !dlg_ || dlg_->useallfld_->getBoolValue()
	|| dlg_->outlistfld_->box()->nrChecked() == compnms_.size() )
	ret = "-- All components --";
    else
    {
	for ( int idx=0; idx<compnms_.size(); idx++ )
	    if ( dlg_->outlistfld_->box()->isItemChecked( idx) )
	    {
		if ( ret.size() ) ret += ", ";
		ret += compnms_.get(idx);
	    }
    }

    return ret;
}


uiMultCompSel::MCompDlg::MCompDlg( uiParent* p, const BufferStringSet& names )
    : uiDialog( p, uiDialog::Setup("Components selection dialog",
				   "",mTODOHelpID) )
{
    useallfld_ = new uiGenInput( this, "Components to use:",
	    			 BoolInpSpec( true, "All", "Subselection" ) );
    useallfld_->valuechanged.notify( mCB(this,uiMultCompSel::MCompDlg,selChg) );

    outlistfld_ = new uiLabeledListBox( this, names, "Available components",
				       false, uiLabeledListBox::AboveMid );
    outlistfld_->attach( ensureBelow, useallfld_ );
    outlistfld_->box()->setItemsCheckable( true );
    outlistfld_->box()->setNotSelectable();
}


void uiMultCompSel::MCompDlg::selChg( CallBacker* )
{
    outlistfld_->display( !useallfld_->getBoolValue() );
}



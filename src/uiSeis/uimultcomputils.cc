/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Helene Huck
 Date:          August 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uimultcomputils.h"
#include "bufstringset.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "uigeninput.h"
#include "uilistbox.h"


uiMultCompDlg::uiMultCompDlg( uiParent* p, const BufferStringSet& complist )
    	: uiDialog(p,uiDialog::Setup("Data component selection","","") )
	, compfld_(0)
{
    BufferString instructions = "After loading multiple components,\n";
    instructions += "use 'Page Up and 'Page Down' buttons to scroll.\n";
    instructions += "Make sure the attribute treeitem is selected\n";
    instructions += "and that the mouse pointer is in the scene.";
    setTitleText( instructions );

    compfld_ = new uiListBox( this, complist, "" );
    compfld_->setMultiSelect();
    compfld_->doubleClicked.notify( mCB(this,uiMultCompDlg,accept) );
}


void uiMultCompDlg::getCompNrs( TypeSet<int>& selitems ) const
{
    compfld_->getSelectedItems( selitems );
}


const char* uiMultCompDlg::getCompName( int idx ) const
{
    return compfld_->textOfItem( idx );
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
	dlg_->outlistfld_->box()->setEmpty();
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
				   "",mNoHelpID) )
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



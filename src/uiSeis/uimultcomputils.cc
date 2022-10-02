/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimultcomputils.h"
#include "bufstringset.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "survgeom.h"


uiMultCompDlg::uiMultCompDlg( uiParent* p, const BufferStringSet& complist )
	: uiDialog(p,uiDialog::Setup(tr("Multi-Attribute selection"),
                                     uiStrings::sEmptyString(), mNoHelpKey) )
	, compfld_(0)
{
    uiString instructions( tr("Workflow :-\n"
	"1) Select multiple attributes and press \"OK\".\n"
	"2) Wait until the attributes are loaded and displayed\n"
	"3) Make sure the attribute tree-item is still selected\n"
	"4) Press the PageUp / PageDown key to scroll through"
	    " the individual attributes") );
    setTitleText( instructions );

    compfld_ = new uiListBox( this, "", OD::ChooseAtLeastOne );
    compfld_->addItems( complist );
    compfld_->doubleClicked.notify( mCB(this,uiMultCompDlg,accept) );
}


uiMultCompDlg::~uiMultCompDlg()
{}


void uiMultCompDlg::getCompNrs( TypeSet<int>& selitems ) const
{
    compfld_->getChosen( selitems );
}


const char* uiMultCompDlg::getCompName( int idx ) const
{
    return compfld_->textOfItem( idx );
}


uiMultCompSel::uiMultCompSel( uiParent* p )
    : uiCompoundParSel(p,tr("Components subselection"))
    , dlg_(0)
{
}


uiMultCompSel::~uiMultCompSel()
{
    delete dlg_;
}


void uiMultCompSel::setUpList( const MultiID& mid )
{
    compnms_.erase();
    SeisIOObjInfo::getCompNames( mid, compnms_ );
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
	dlg_->outlistfld_->setEmpty();
	dlg_->outlistfld_->addItems( compnms_ );
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
	|| dlg_->outlistfld_->nrChosen() == compnms_.size() )
	ret = "-- All components --";
    else
    {
	BufferStringSet selnms;
	for ( int idx=0; idx<compnms_.size(); idx++ )
	    if ( dlg_->outlistfld_->isChosen( idx) )
		selnms.add( compnms_.get(idx) );
	ret = selnms.getDispString();
    }

    return ret;
}


uiMultCompSel::MCompDlg::MCompDlg( uiParent* p, const BufferStringSet& names )
    : uiDialog( p, uiDialog::Setup(tr("Components selection dialog"),
				   uiString::emptyString(),mNoHelpKey) )
{
    useallfld_ = new uiGenInput( this, tr("Components to use:"),
				 BoolInpSpec( true, uiStrings::sAll(),
                                 tr("Subselection") ) );
    useallfld_->valuechanged.notify( mCB(this,uiMultCompSel::MCompDlg,selChg));

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Available components"),
			 uiListBox::AboveMid );
    outlistfld_ = new uiListBox( this, su );
    outlistfld_->addItems( names );
    outlistfld_->attach( ensureBelow, useallfld_ );
}


uiMultCompSel::MCompDlg::~MCompDlg()
{}


void uiMultCompSel::MCompDlg::selChg( CallBacker* )
{
    outlistfld_->display( !useallfld_->getBoolValue() );
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uilayseqattribed.cc,v 1.1 2011-01-14 14:44:09 cvsbert Exp $";

#include "uilayseqattribed.h"
#include "stratlayseqattrib.h"
#include "propertyref.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uimsg.h"


uiLaySeqAttribEd::uiLaySeqAttribEd( uiParent* p, Strat::LaySeqAttrib& lsa,
				   const Strat::RefTree& rt,  bool isnew )
    : uiDialog(p,Setup(isnew ? "Add attribute" : "Edit attribute",
		    gtDlgTitle(lsa,isnew),mTODOHelpID))
    , attr_(lsa)
    , nmchgd_(false)
    , anychg_(false)
{
    uiLabeledListBox* lunfld = new uiLabeledListBox( this, "Selected Units",
				     true, uiLabeledListBox::AboveMid );
    unfld_ = lunfld->box();
    uiLabeledListBox* llithfld = new uiLabeledListBox( this, "Lithologies",
				     true, uiLabeledListBox::AboveMid );
    lithofld_ = llithfld->box();
    llithfld->attach( rightOf, lunfld );

    uiLabeledComboBox* lstatsfld = new uiLabeledComboBox( this,
						"Statistics on results" );
    statsfld_ = lstatsfld->box();
    lstatsfld->attach( alignedBelow, llithfld );

    const CallBack transfcb( mCB(this,uiLaySeqAttribEd,transfSel) );
    uiLabeledComboBox* ltransffld = new uiLabeledComboBox( this,
						"Statistics on results" );
    transformfld_ = ltransffld->box();
    static const char* transfs[] = { "No", "Power", "Log", "Exp", 0 };
    transformfld_->addItems( BufferStringSet(transfs) );
    transformfld_->selectionChanged.notify( transfcb );
    ltransffld->attach( alignedBelow, lstatsfld );
    valfld_ = new uiGenInput( this, "Value", FloatInpSpec(2) );
    valfld_->attach( rightOf, ltransffld );

    namefld_ = new uiGenInput( this, "Name", attr_.name() );
    namefld_->attach( alignedBelow, ltransffld );

    finaliseDone.notify( transfcb );
}


uiLaySeqAttribEd::~uiLaySeqAttribEd()
{
}


const char* uiLaySeqAttribEd::gtDlgTitle( const Strat::LaySeqAttrib& lsa,
    					  bool isnew ) const
{
    static BufferString ret; ret = isnew ? "Define" : "Edit";
    ret.add( " parameters for " ).add( lsa.prop_.name() ).add( " attribute" );
    return ret;
}


void uiLaySeqAttribEd::transfSel( CallBacker* )
{
    const int sel = transformfld_->currentItem();
    if ( sel == 2 )
	valfld_->setTitleText( "Value" );
    else if ( sel )
	valfld_->setTitleText( "Base" );
    valfld_->display( sel );
}


void uiLaySeqAttribEd::putToScreen()
{
}


bool uiLaySeqAttribEd::getFromScreen()
{
    //TODO get stuff
    attr_.setName( namefld_->text() );
    return true;
}


bool uiLaySeqAttribEd::acceptOK( CallBacker* )
{
    const BufferString oldnm( attr_.name() );
    const BufferString newnm( namefld_->text() );
    if ( newnm.isEmpty() )
	{ uiMSG().error( "Please enter a valid name" ); return false; }
    if ( oldnm != newnm )
    {
	const Strat::LaySeqAttribSet& lsas = attr_.attrSet();
	for ( int idx=0; idx<lsas.size(); idx++ )
	{
	    const Strat::LaySeqAttrib& lsa = lsas.attr( idx );
	    if ( &lsa != &attr_ && newnm == lsa.name() )
	    {
		uiMSG().error("The name is already used for another attribute");
		return false;
	    }
	}
    }

    if ( !getFromScreen() )
	return false;

    anychg_ = true;
    nmchgd_ = oldnm != newnm;
    return true;
}

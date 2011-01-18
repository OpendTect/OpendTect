/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uilayseqattribed.cc,v 1.3 2011-01-18 11:14:22 cvsbert Exp $";

#include "uilayseqattribed.h"
#include "stratlayseqattrib.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "propertyref.h"
#include "stattype.h"
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
    uiLabeledListBox* llithfld = new uiLabeledListBox( this, "Lithologies",
				     true, uiLabeledListBox::AboveMid );
    lithofld_ = llithfld->box();
    uiLabeledListBox* lunfld = new uiLabeledListBox( this, "Selected Units",
				     true, uiLabeledListBox::AboveMid );
    unfld_ = lunfld->box();
    lunfld->attach( rightOf, llithfld );

    uiLabeledComboBox* lstatsfld = new uiLabeledComboBox( this,
						"Statistics on results" );
    statsfld_ = lstatsfld->box();
#   define mAddStatItm(enm) \
    statsfld_-> addItem( Stats::TypeNames()[Stats::enm] );
    if ( attr_.prop_.hasType(PropertyRef::Dist) )
	mAddStatItm(Sum);
    mAddStatItm(Average); mAddStatItm(Median); mAddStatItm(StdDev);
    mAddStatItm(Min); mAddStatItm(Max);
    lstatsfld->attach( alignedBelow, lunfld );
    lstatsfld->attach( ensureBelow, llithfld );

    const CallBack transfcb( mCB(this,uiLaySeqAttribEd,transfSel) );
    uiLabeledComboBox* ltransffld = new uiLabeledComboBox( this,
						"Transform values" );
    transformfld_ = ltransffld->box();
    static const char* transfs[] = { "No", "Power", "Log", "Exp", 0 };
    transformfld_->addItems( BufferStringSet(transfs) );
    transformfld_->setHSzPol( uiObject::Small );
    transformfld_->selectionChanged.notify( transfcb );
    ltransffld->attach( alignedBelow, lstatsfld );
    valfld_ = new uiGenInput( this, "Value", FloatInpSpec(2) );
    valfld_->setElemSzPol( uiObject::Small );
    valfld_->attach( rightOf, ltransffld );

    namefld_ = new uiGenInput( this, "Name", attr_.name() );
    namefld_->attach( alignedBelow, ltransffld );

    fillFlds( rt );
    putToScreen();
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


void uiLaySeqAttribEd::fillFlds( const Strat::RefTree& reftree )
{
    for ( int idx=0; idx<reftree.lithologies().size(); idx++ )
	lithofld_->addItem( reftree.lithologies().getLith(idx).name() );

    Strat::UnitRefIter it( reftree, Strat::UnitRefIter::LeavedNodes );
    while ( it.next() )
	unfld_->addItem( it.unit()->fullCode() );
}


void uiLaySeqAttribEd::transfSel( CallBacker* )
{
    const int sel = transformfld_->currentItem();
    if ( sel == 1 )
	valfld_->setTitleText( "Value" );
    else if ( sel )
	valfld_->setTitleText( "Base" );
    valfld_->display( sel );
}


void uiLaySeqAttribEd::putToScreen()
{
    namefld_->setText( attr_.name() );
    statsfld_->setText( attr_.stat_ );

    if ( mIsUdf(attr_.transformval_) )
	transformfld_->setCurrentItem( 0 );
    else
    {
	transformfld_->setCurrentItem( ((int)attr_.transform_) + 1 );
	valfld_->setValue( attr_.transformval_ );
    }

    for ( int idx=0; idx<unfld_->size(); idx++ )
	unfld_->setSelected( idx, attr_.units_.isPresent(
		    			unfld_->textOfItem(idx)) );
    for ( int idx=0; idx<lithofld_->size(); idx++ )
	lithofld_->setSelected( idx, attr_.lithos_.isPresent(
		    			lithofld_->textOfItem(idx)) );
}


bool uiLaySeqAttribEd::getFromScreen()
{
    BufferStringSet uns, liths;
    unfld_->getSelectedItems( uns ); lithofld_->getSelectedItems( liths );
    if ( uns.isEmpty() || liths.isEmpty() )
    {
	uiMSG().error( "Please select at least one unit and one lithology" );
	return false;
    }
    const int trfldidx = transformfld_->currentItem();
    const bool havetr = trfldidx > 0;
    const int tridx = havetr ? trfldidx - 1 : 0;
    const float trval = havetr ? valfld_->getfValue() : mUdf(float);
    if ( havetr
      && (trval == 0 || (tridx == (int)(Strat::LaySeqAttrib::Log) && trval<0)) )
    {
	uiMSG().error( "Please enter a correct ", valfld_->titleText() );
	return false;
    }

    attr_.setName( namefld_->text() );
    attr_.units_ = uns;
    attr_.lithos_ = liths;
    attr_.stat_ = statsfld_->text();
    attr_.transformval_ = trval;
    attr_.transform_ = (Strat::LaySeqAttrib::Transform)tridx;

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

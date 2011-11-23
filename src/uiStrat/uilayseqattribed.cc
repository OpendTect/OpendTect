/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uilayseqattribed.cc,v 1.6 2011-11-23 11:35:56 cvsbert Exp $";

#include "uilayseqattribed.h"
#include "stratlayseqattrib.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "propertyref.h"
#include "stattype.h"
#include "uilistbox.h"
#include "uilabel.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiseparator.h"
#include "uimsg.h"


uiLaySeqAttribEd::uiLaySeqAttribEd( uiParent* p, Strat::LaySeqAttrib& lsa,
				   const Strat::RefTree& rt,  bool isnew )
    : uiDialog(p,Setup(isnew ? "Add attribute" : "Edit attribute",
		    gtDlgTitle(lsa,isnew),mTODOHelpID))
    , attr_(lsa)
    , nmchgd_(false)
    , anychg_(false)
{
    isslidingfld_ = new uiGenInput( this, "Type",
	    		BoolInpSpec( false, "Sliding", "Integrated") );
    isslidingfld_->valuechanged.notify( mCB(this,uiLaySeqAttribEd,slSel) );

    slidegrp_ = new uiGroup( this, "Slide group" );
    uiLabel* lbl = new uiLabel( slidegrp_, " " );
    uiLabeledComboBox* lupscfld = new uiLabeledComboBox( slidegrp_,
						"From depth intervals" );
    upscaletypfld_ = lupscfld->box();
    upscaletypfld_-> addItems( Stats::UpscaleTypeNames() );
    lupscfld->attach( alignedBelow, lbl );
    slidegrp_->setHAlignObj( lupscfld );
    slidegrp_->attach( alignedBelow, isslidingfld_ );

    localgrp_ = new uiGroup( this, "Local group" );

    lithofld_ = new uiListBox( localgrp_, "Lithologies", false,
	    			rt.lithologies().size() );
    lithofld_->setItemsCheckable( true );
    unfld_ = new uiListBox( localgrp_,"Units" );
    unfld_->setItemsCheckable( true );
    lithofld_->attach( centeredRightOf, unfld_ );
    stattypfld_ = new uiComboBox( localgrp_, "Statistics on results" );
    new uiLabel( localgrp_, "Statistics on results", stattypfld_ );
#   define mAddStatItm(enm) \
    stattypfld_->addItem( Stats::TypeNames()[Stats::enm] );
    if ( attr_.prop_.hasType(PropertyRef::Dist) )
	mAddStatItm(Sum);
    mAddStatItm(Average); mAddStatItm(Median); mAddStatItm(StdDev);
    mAddStatItm(Min); mAddStatItm(Max);
    stattypfld_->attach( alignedBelow, lithofld_ );
    stattypfld_->attach( ensureBelow, unfld_ );
    localgrp_->setHAlignObj( stattypfld_ );
    localgrp_->attach( alignedBelow, isslidingfld_ );

    uiSeparator* sep = new uiSeparator( this, "Sep" );
    sep->attach( stretchedBelow, localgrp_ );

    const CallBack transfcb( mCB(this,uiLaySeqAttribEd,transfSel) );
    uiLabeledComboBox* ltransffld = new uiLabeledComboBox( this,
						"Transform values" );
    transformfld_ = ltransffld->box();
    static const char* transfs[] = { "No", "Power", "Log", "Exp", 0 };
    transformfld_->addItems( BufferStringSet(transfs) );
    transformfld_->setHSzPol( uiObject::Small );
    transformfld_->selectionChanged.notify( transfcb );
    ltransffld->attach( alignedWith, localgrp_ );
    ltransffld->attach( ensureBelow, sep );
    valfld_ = new uiGenInput( this, "Value", FloatInpSpec(2) );
    valfld_->setElemSzPol( uiObject::Small );
    valfld_->attach( rightOf, ltransffld );

    namefld_ = new uiGenInput( this, "Name", attr_.name() );
    namefld_->attach( alignedBelow, ltransffld );

    fillFlds( rt );
    putToScreen();
    postFinalise().notify( mCB(this,uiLaySeqAttribEd,initWin) );
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


void uiLaySeqAttribEd::initWin( CallBacker* c )
{
    slSel( c );
    transfSel( c );
}


void uiLaySeqAttribEd::slSel( CallBacker* )
{
    const bool isslide = isslidingfld_->getBoolValue();
    slidegrp_->display( isslide );
    localgrp_->display( !isslide );
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
    isslidingfld_->setValue( attr_.islocal_ );
    if ( attr_.islocal_ )
	upscaletypfld_->setText( attr_.stat_ );
    else
    {
	stattypfld_->setText( attr_.stat_ );
	for ( int idx=0; idx<unfld_->size(); idx++ )
	    unfld_->setItemChecked( idx, attr_.units_.isPresent(
					    unfld_->textOfItem(idx)) );
	for ( int idx=0; idx<lithofld_->size(); idx++ )
	    lithofld_->setItemChecked( idx, attr_.liths_.isPresent(
					    lithofld_->textOfItem(idx)) );
    }

    if ( mIsUdf(attr_.transformval_) )
	transformfld_->setCurrentItem( 0 );
    else
    {
	transformfld_->setCurrentItem( ((int)attr_.transform_) + 1 );
	valfld_->setValue( attr_.transformval_ );
    }

}


bool uiLaySeqAttribEd::getFromScreen()
{
    const bool islocal = isslidingfld_->getBoolValue();
    BufferStringSet uns, liths;

    if ( !islocal )
    {
	unfld_->getCheckedItems( uns ); lithofld_->getCheckedItems( liths );
	if ( uns.isEmpty() || liths.isEmpty() )
	{
	    uiMSG().error("Please select at least one unit and one lithology");
	    return false;
	}
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
    attr_.islocal_ = islocal;
    if ( islocal )
	attr_.stat_ = upscaletypfld_->text();
    else
    {
	attr_.units_ = uns;
	attr_.liths_ = liths;
	attr_.stat_ = stattypfld_->text();
    }
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

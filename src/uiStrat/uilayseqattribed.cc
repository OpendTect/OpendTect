/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
#include "uistratselunits.h"
#include "uimsg.h"

static BufferString gtDlgTitle( const Strat::LaySeqAttrib& lsa, bool isnew )
{
    BufferString ret( isnew ? "Define" : "Edit" );
    ret.add( " parameters for " ).add( lsa.prop_.name() ).add( " attribute" );
    return ret;
}


uiLaySeqAttribEd::uiLaySeqAttribEd( uiParent* p, Strat::LaySeqAttrib& lsa,
				   const Strat::RefTree& rt,
				   const uiLaySeqAttribEd::Setup& edsu )
    : uiDialog(p,uiDialog::Setup(edsu.isnew_?"Add attribute":"Edit attribute",
		    gtDlgTitle(lsa,edsu.isnew_),"110.3.2"))
    , attr_(lsa)
    , reftree_(rt)
    , nmchgd_(false)
    , anychg_(false)
    , islocalfld_(0)
    , integrgrp_(0)
    , localgrp_(0)
{
    if ( edsu.allowlocal_ && edsu.allowintegr_ )
    {
	islocalfld_ = new uiGenInput( this, "Type",
			    BoolInpSpec( false, "Sliding", "Integrated") );
	islocalfld_->valuechanged.notify( mCB(this,uiLaySeqAttribEd,slSel) );
    }

    if ( edsu.allowlocal_ )
    {
	localgrp_ = new uiGroup( this, "Local group" );
	uiLabel* lbl = 0;
	if ( edsu.allowintegr_ )
	    lbl = new uiLabel( localgrp_, " " );
	uiLabeledComboBox* lupscfld = new uiLabeledComboBox( localgrp_,
						    "From depth intervals" );
	upscaletypfld_ = lupscfld->box();
	upscaletypfld_-> addItems( Stats::UpscaleTypeNames() );
	if ( lbl )
	    lupscfld->attach( alignedBelow, lbl );
	localgrp_->setHAlignObj( lupscfld );
	if ( islocalfld_ )
	    localgrp_->attach( alignedBelow, islocalfld_ );
    }

    uiSeparator* sep = 0;

    if ( edsu.allowintegr_ )
    {
	integrgrp_ = new uiGroup( this, "Integrated group" );

	lithofld_ = new uiListBox( integrgrp_, "Lithologies", false,
				    reftree_.lithologies().size() );
	lithofld_->setItemsCheckable( true );
	for ( int idx=0; idx<reftree_.lithologies().size(); idx++ )
	    lithofld_->addItem( reftree_.lithologies().getLith(idx).name() );

	uiStratSelUnits::Setup ssusu( uiStratSelUnits::Multi,
				      Strat::UnitRefIter::AllNodes );
	ssusu.fldtxt( "Contributing units" );
	unfld_ = new uiStratSelUnits( integrgrp_, reftree_, ssusu );
	unfld_->setExpanded( 1 );

	stattypfld_ = new uiComboBox( integrgrp_, "Statistics on results" );
	new uiLabel( integrgrp_, "Statistics on results", stattypfld_ );
#   define mAddStatItm(enm) \
	stattypfld_->addItem( Stats::TypeNames()[Stats::enm] );
	if ( attr_.prop_.hasType(PropertyRef::Dist) )
	    mAddStatItm(Sum);
	mAddStatItm(Average); mAddStatItm(Median); mAddStatItm(StdDev);
	mAddStatItm(Min); mAddStatItm(Max);

	lithofld_->attach( centeredRightOf, unfld_ );
	stattypfld_->attach( alignedBelow, lithofld_ );
	stattypfld_->attach( ensureBelow, unfld_ );
	integrgrp_->setHAlignObj( stattypfld_ );
	if ( islocalfld_ )
	    integrgrp_->attach( alignedBelow, islocalfld_ );

	sep = new uiSeparator( this, "Sep" );
	sep->attach( stretchedBelow, integrgrp_ );
    }

    const CallBack transfcb( mCB(this,uiLaySeqAttribEd,transfSel) );
    uiLabeledComboBox* ltransffld = new uiLabeledComboBox( this,
						"Transform values" );
    transformfld_ = ltransffld->box();
    static const char* transfs[] = { "No", "Power", "Log", "Exp", 0 };
    transformfld_->addItems( BufferStringSet(transfs) );
    transformfld_->setHSzPol( uiObject::Small );
    transformfld_->selectionChanged.notify( transfcb );
    if ( !sep )
	ltransffld->attach( alignedBelow, localgrp_ );
    else
    {
	ltransffld->attach( alignedWith, integrgrp_ );
	ltransffld->attach( ensureBelow, sep );
    }

    valfld_ = new uiGenInput( this, "Value", FloatInpSpec(2) );
    valfld_->setElemSzPol( uiObject::Small );
    valfld_->attach( rightOf, ltransffld );

    namefld_ = new uiGenInput( this, "Name", attr_.name() );
    namefld_->attach( alignedBelow, ltransffld );

    putToScreen();
    postFinalise().notify( mCB(this,uiLaySeqAttribEd,initWin) );
}


uiLaySeqAttribEd::~uiLaySeqAttribEd()
{
}


void uiLaySeqAttribEd::initWin( CallBacker* c )
{
    slSel( c );
    transfSel( c );
}


bool uiLaySeqAttribEd::isLocal() const
{
    return islocalfld_ ? islocalfld_->getBoolValue() : haveLocal();
}


void uiLaySeqAttribEd::slSel( CallBacker* )
{
    const bool islocal = isLocal();
    if ( localgrp_ ) localgrp_->display( islocal );
    if ( integrgrp_ ) integrgrp_->display( !islocal );
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
    if ( islocalfld_ )
	islocalfld_->setValue( attr_.islocal_ );

    const bool filllocal = !integrgrp_ || attr_.islocal_;
    const bool fillintegr = !localgrp_ || !attr_.islocal_;

    if ( filllocal && localgrp_ )
    {
	if ( localgrp_ )
	    upscaletypfld_->setText( attr_.stat_ );
    }
    if ( fillintegr && integrgrp_ )
    {
	stattypfld_->setText( attr_.stat_ );

	Strat::UnitRefIter it( reftree_ );
	while ( it.next() )
	    unfld_->setSelected( *(Strat::NodeUnitRef*)it.unit(),
			    attr_.units_.isPresent( it.unit()->fullCode() ) );

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
    const bool islocal = isLocal();
    BufferStringSet uns, liths;

    if ( !islocal )
    {
	Strat::UnitRefIter it( reftree_, Strat::UnitRefIter::LeavedNodes );
	while ( it.next() )
	{
	    if ( unfld_->isSelected(*it.unit()) )
		uns.add( it.unit()->fullCode() );
	}
	lithofld_->getCheckedItems( liths );

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

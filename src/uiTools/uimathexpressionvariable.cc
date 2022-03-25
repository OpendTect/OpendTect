/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2012
________________________________________________________________________

-*/


#include "uimathexpressionvariable.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uimathexpression.h"
#include "uimsg.h"
#include "uiunitsel.h"
#include "uitoolbutton.h"
#include "uirockphysform.h"

#include "linekey.h"
#include "separstr.h"
#include "mathspecvars.h"
#include "mathexpression.h"
#include "unitofmeasure.h"


uiMathExpressionVariable::uiMathExpressionVariable( uiParent* p,
	int varidx, bool withunit, bool withsub, const Math::SpecVarSet* svs )
    : uiGroup(p,BufferString("MathExprVar ",varidx))
    , varidx_(varidx)
    , specvars_(*new Math::SpecVarSet(svs?*svs:Math::SpecVarSet::getEmpty()))
    , inpSel(this)
    , subInpSel(this)
{
    inpgrp_ = new uiGroup( this, "Input group" );
    inpfld_ = new uiComboBox( inpgrp_, BufferString("input ",varidx_+1) );
    const uiString lblstr = tr("For %1 use").arg(varidx_+1);
    inpfld_->setHSzPol( uiObject::WideMax );
    inplbl_ = new uiLabel( inpgrp_, lblstr, inpfld_ );
    inplbl_->setPrefWidthInChar( 35 );
    inplbl_->setAlignment( Alignment::Right );
    mAttachCB( inpfld_->selectionChanged, uiMathExpressionVariable::inpChg );

    if ( withsub )
    {
	subinpfld_ = new uiComboBox( inpgrp_, "Sub Input" );
	subinpfld_->attach( rightOf, inpfld_ );
	mAttachCB( subinpfld_->selectionChanged,
		   uiMathExpressionVariable::subInpChg );
    }

    constfld_ = new uiGenInput( inpgrp_, tr("Value for 'c0'"), FloatInpSpec() );
    constfld_->attach( alignedWith, inpfld_ );
    if ( withunit )
    {
	selunfld_ = new uiLineEdit( this, sKey::Unit() );
	selunfld_->setReadOnly();
	selunfld_->attach( rightOf, inpgrp_ );

	uiUnitSel::Setup uussu( Mnemonic::Other );
	uussu.mode( uiUnitSel::Setup::SymbolsOnly );
	unfld_ = new uiUnitSel( this, uussu );
	unfld_->attach( rightOf, selunfld_ );

	if ( varidx==0 )
	{
	    unitlbl_ = new uiLabel( this, tr("Input unit") );
	    unitlbl_->attach( alignedAbove, selunfld_ );
	    formlbl_ = new uiLabel( this, tr("Formula requires") );
	    formlbl_->attach( alignedAbove, unfld_ );
	}
    }

    setHAlignObj( inpfld_ );
    mAttachCB( preFinalize(), uiMathExpressionVariable::initFlds );
}


uiMathExpressionVariable::~uiMathExpressionVariable()
{
    detachAllNotifiers();
    delete &specvars_;
}


void uiMathExpressionVariable::addInpViewIcon( const char* icnm, const char* tt,
						const CallBack& cb )
{
    vwbut_ = new uiToolButton( inpgrp_, icnm, mToUiStringTodo(tt), cb );
    vwbut_->attach( rightOf, subinpfld_ ? subinpfld_ : inpfld_ );
    mAttachCB( inpSel, uiMathExpressionVariable::showHideVwBut );
}


BufferStringSet uiMathExpressionVariable::getInputNms( const Mnemonic* mn,
						       bool sub ) const
{
    BufferStringSet nms;
    if ( isSpec() && !sub )
	specvars_.getNames( nms );
    else if ( !isConst() && !isSpec() )
    {
	nms = sub ? nonspecsubinputs_ : nonspecinputs_;
	if ( mnsel_ && mn && !mn->isUdf() )
	{
	    for ( int idx=nms.size()-1; idx>=0; idx-- )
	    {
		const Mnemonic* selmn = mnsel_->validIdx(idx) ? mnsel_->get(idx)
				      : nullptr;
		if ( !mn->isCompatibleWith(selmn) )
		    nms.removeSingle( idx );
	    }
	    if ( nms.isEmpty() )
		nms = sub ? nonspecsubinputs_ : nonspecinputs_;
	}
    }

    return nms;
}


void uiMathExpressionVariable::updateInpNms( bool sub )
{
    uiComboBox* inpfld = sub ? subinpfld_ : inpfld_;
    if ( !inpfld )
	return;

    const BufferString curseltxt( inpfld->text() );
    inpfld->setEmpty();
    const BufferStringSet nms = getInputNms( curmn_, sub );
    inpfld->addItems( nms );
    inpfld->setCurrentItem( curseltxt );
    if ( sub )
	inpfld->display( !nms.isEmpty() && !isConst() && isActive() );
}


void uiMathExpressionVariable::setNonSpecInputs( const BufferStringSet& nms,
						 const MnemonicSelection* mnsel)
{
    nonspecinputs_ = nms;
    mnsel_ = mnsel;
    curmn_ = nullptr;

    updateInpNms( false );
}


void uiMathExpressionVariable::setNonSpecSubInputs( const BufferStringSet& nms )
{
    nonspecsubinputs_ = nms;
    updateInpNms( true );
}


uiGroup* uiMathExpressionVariable::rightMostField()
{
    if ( unfld_ )
	return unfld_;

    return inpgrp_;
}


void uiMathExpressionVariable::showHideVwBut( CallBacker* )
{
    if ( vwbut_ )
	vwbut_->display( isActive() && !isConst() && !isSpec() );
}


void uiMathExpressionVariable::updateDisp()
{
    constfld_->display( isActive() && isConst() );
    bool dodisp = isActive() && !isConst();
    inpfld_->display( dodisp );
    inplbl_->display( dodisp );
    if ( subinpfld_ )
	subinpfld_->display( dodisp && !nonspecsubinputs_.isEmpty() );

    if ( unfld_ )
    {
	if ( isSpec() )
	    dodisp = dodisp && specvars_.hasUnits(specidx_);
	selunfld_->display( dodisp );
	unfld_->display( dodisp );
	if ( unitlbl_ )
	    unitlbl_->display( dodisp );
	if ( formlbl_ )
	    formlbl_->display( dodisp );
    }

    showHideVwBut();
}


void uiMathExpressionVariable::setActive( bool yn )
{
    isactive_ = yn;
}


void uiMathExpressionVariable::setVariable( const char* varnm, bool isconst )
{
    setActive( true );
    isconst_ = isconst;
    varnm_ = varnm;
    specidx_ = specvars_.getIndexOf( varnm_ );

    if ( !isconst )
    {
	updateInpNms( true );
	updateInpNms( false );
    }

    uiString lbltxt;
    bool issens = true;
    if ( isConst() )
	constfld_->setTitleText( tr("Value for '%1'").arg(varnm_) );
    else if ( isSpec() )
    {
	inplbl_->setText(tr("'%1' filled with").arg(varnm_));
	inpfld_->setCurrentItem( specvars_.dispName(specidx_) );
	inpChg( nullptr );
	issens = false;
	if ( unfld_ && specvars_.hasUnits(specidx_) )
	    unfld_->setPropType( specvars_.propType(specidx_) );
    }
    else
	inplbl_->setText( tr("For '%1' use").arg(varnm_) );

    inpfld_->setSensitive( issens );

    updateDisp();
}


void uiMathExpressionVariable::use( const Math::Expression* expr )
{
    varnm_.setEmpty();
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    if ( varidx_ >= nrvars )
	{ setActive( false ); updateDisp(); return; }
    const BufferString varnm = expr->uniqueVarName( varidx_ );

    const int varidx = expr->indexOfUnVarName( expr->uniqueVarName(varidx_) );
    curmn_ = nullptr;
    setVariable( varnm, expr->getType(varidx) == Math::Expression::Constant );
}


void uiMathExpressionVariable::use( const Math::Formula& form, bool fixedunits )
{
    specvars_ = form.specVars();
    varnm_.setEmpty();
    const int nrvars = form.nrInputs();
    if ( varidx_ >= nrvars )
	{ setActive( false ); updateDisp(); return; }

    const BufferString varnm = form.variableName( varidx_ );
    curmn_ = form.inputMnemonic( varidx_ );
    setVariable( varnm, form.isConst( varidx_ ) );
    const BufferString inpdef( form.inputDef(varidx_) );
    const bool isspec = isSpec();

    if ( isConst() )
    {
	constfld_->setValue( inpdef.toFloat() );
	return;
    }
    else if ( isspec && unfld_ )
    {
	const Mnemonic& formmn = specvars_.mnemonic( specidx_ );
	if ( &formmn != unfld_->mnemonic() )
	    unfld_->setMnemonic( formmn );
    }
    else if ( !isspec )
	selectInput( inpdef );
}


void uiMathExpressionVariable::selectInput( const char* inpnm, bool exact )
{
    if ( !inpnm ) inpnm = "";
    const Math::Expression::VarType vartp =
		Math::ExpressionParser::varTypeOf( inpnm );
    if ( inpfld_->isEmpty() && vartp==Math::Expression::Constant )
    {
	isconst_ = true;
	updateDisp();
	return;
    }

    isconst_ = false;
    updateDisp();
    BufferString varnm( inpnm ), subnm;
    if ( subinpfld_ )
    {
	const LineKey linekey( inpnm );
	varnm = linekey.lineName(); subnm = linekey.attrName();
    }

    bool isfound = FixedString(inpfld_->text()) == varnm;
    if ( !exact && !isfound )
    {
	BufferStringSet avnms; inpfld_->getItems( avnms );
	curmn_ = mnsel_ ? mnsel_->getByName( varnm ) : nullptr;
	if ( curmn_ )
	{
	    for ( const auto* avnm : avnms )
	    {
		if ( curmn_->matches(avnm->str(),true) )
		{
		    varnm.set( avnm->str() );
		    isfound = true;
		    break;
		}
	    }
	}

	if ( !isfound )
	{
	    const int nearidx = avnms.nearestMatch( varnm );
	    if ( avnms.validIdx(nearidx) )
	    {
		varnm = avnms.get( nearidx );
		isfound = true;
	    }
	}
    }

    if ( isfound )
	inpfld_->setCurrentItem( varnm );

    inpChg( nullptr );
    if ( subinpfld_ )
	subinpfld_->setCurrentItem( subnm );
}


bool uiMathExpressionVariable::isSpec() const
{
    return specvars_.validIdx( specidx_ );
}


const char* uiMathExpressionVariable::getInput() const
{
    if ( isConst() )
	return constfld_->text();
    if ( !subinpfld_ || subinpfld_->isEmpty() )
	return inpfld_->text();

    FileMultiString fms( inpfld_->text() );
    fms += subinpfld_->text();
    mDeclStaticString( ret );
    ret.set( fms.buf() );
    return ret;
}


const UnitOfMeasure* uiMathExpressionVariable::getUnit() const
{
    if ( !unfld_ || !unfld_->mainObject()->isDisplayed() )
	return nullptr;
    return unfld_->getUnit();
}


void uiMathExpressionVariable::fill( Math::Formula& form ) const
{
    if ( form.nrInputs() <= varidx_ )
	return;

    form.setInputDef( varidx_, getInput() );
    form.setInputMnemonic( varidx_, curmn_ );
    form.setInputFormUnit( varidx_, getUnit() );
}


void uiMathExpressionVariable::setSelUnit( const UnitOfMeasure* uom )
{
    if ( selunfld_ )
	selunfld_->setText( UnitOfMeasure::getUnitLbl( uom, "-" ) );
}


void uiMathExpressionVariable::setFormType( const Mnemonic& mn )
{
    curmn_ = &mn;
    if ( unfld_ )
	unfld_->setMnemonic( mn );
}


void uiMathExpressionVariable::setFormUnit( const UnitOfMeasure* uom,
					    bool dosensitive )
{
    if ( !unfld_ )
	return;

    unfld_->setUnit( uom );
    unfld_->setSensitive( dosensitive );
}


void uiMathExpressionVariable::setUnit( const char* nm )
{
    const UnitOfMeasure* uom = UoMR().get( nm );
    setFormUnit( uom, true );
    unfld_->setSensitive( !uom );
}


void uiMathExpressionVariable::setPropType( Mnemonic::StdType typ )
{
    setFormType( MNC().getGuessed(typ) );
}


void uiMathExpressionVariable::selectSubInput( int idx )
{
    if ( !subinpfld_ || subinpfld_->isEmpty() )
	return;

    subinpfld_->setCurrentItem( idx );
}

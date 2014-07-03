/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uimathexpressionvariable.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimathexpression.h"
#include "uimsg.h"
#include "uiunitsel.h"
#include "uitoolbutton.h"
#include "uirockphysform.h"

#include "separstr.h"
#include "mathspecvars.h"
#include "mathexpression.h"
#include "unitofmeasure.h"


static const Math::SpecVarSet emptsvs;


uiMathExpressionVariable::uiMathExpressionVariable( uiParent* p,
	int varidx, bool withunit, bool withsub, const Math::SpecVarSet* svs )
    : uiGroup(p,BufferString("MathExprVar ",varidx))
    , varidx_(varidx)
    , isconst_(false)
    , specidx_(-1)
    , isactive_(true)
    , subinpfld_(0)
    , unfld_(0)
    , vwbut_(0)
    , specvars_(*new Math::SpecVarSet(svs?*svs:emptsvs))
    , inpSel(this)
    , subInpSel(this)
{
    inpgrp_ = new uiGroup( this, "Input group" );
    inpfld_ = new uiComboBox( inpgrp_, BufferString("input ",varidx_+1) );
    const BufferString lblstr( "For input number ", varidx_+1, " use" );
    inplbl_ = new uiLabel( inpgrp_, lblstr, inpfld_ );
    inplbl_->setPrefWidthInChar( 35 );
    inplbl_->setAlignment( Alignment::Right );
    inpfld_->selectionChanged.notify(
			mCB(this,uiMathExpressionVariable,inpChg) );

    if ( withsub )
    {
	subinpfld_ = new uiComboBox( inpgrp_, "Sub Input" );
	subinpfld_->attach( rightOf, inpfld_ );
	subinpfld_->selectionChanged.notify(
			    mCB(this,uiMathExpressionVariable,subInpChg) );
    }

    constfld_ = new uiGenInput( inpgrp_, "Value for 'c0'", FloatInpSpec() );
    constfld_->attach( alignedWith, inpfld_ );

    if ( withunit )
    {
	uiUnitSel::Setup uussu( PropertyRef::Other, "convert to:" );
	uussu.withnone( true );
	unfld_ = new uiUnitSel( this, uussu );
	unfld_->attach( rightOf, inpgrp_ );
    }

    setHAlignObj( inpfld_ );
    preFinalise().notify( mCB(this,uiMathExpressionVariable,initFlds) );
}


uiMathExpressionVariable::~uiMathExpressionVariable()
{
    delete &specvars_;
}


void uiMathExpressionVariable::addInpViewIcon( const char* icnm, const char* tt,
						const CallBack& cb )
{
    vwbut_ = new uiToolButton( inpgrp_, icnm, tt, cb );
    vwbut_->attach( rightOf, subinpfld_ ? subinpfld_ : inpfld_ );
    inpSel.notify( mCB(this,uiMathExpressionVariable,showHideVwBut) );
}


void uiMathExpressionVariable::updateInpNms( bool sub )
{
    uiComboBox* inpfld = sub ? subinpfld_ : inpfld_;
    if ( !inpfld )
	return;

    const BufferString curseltxt( inpfld->text() );
    inpfld->setEmpty();
    BufferStringSet nms;
    if ( specidx_ < 0 )
	nms = sub ? nonspecsubinputs_ : nonspecinputs_;
    else if ( !sub )
	specvars_.getNames( nms );
    inpfld->addItems( nms );
    inpfld->setCurrentItem( curseltxt );
    if ( sub )
	inpfld->display( !nms.isEmpty() );
}


void uiMathExpressionVariable::setNonSpecInputs( const BufferStringSet& nms )
{
    nonspecinputs_ = nms;
    updateInpNms( false );
}


void uiMathExpressionVariable::setNonSpecSubInputs( const BufferStringSet& nms)
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
	vwbut_->display( isactive_ && !isconst_ && specidx_ < 0 );
}


void uiMathExpressionVariable::updateDisp()
{
    constfld_->display( isactive_ && isconst_ );
    bool dodisp = isactive_ && !isconst_;
    inpfld_->display( dodisp );
    inplbl_->display( dodisp );
    if ( subinpfld_ )
	subinpfld_->display( dodisp && !nonspecsubinputs_.isEmpty() );
    if ( unfld_ )
    {
	if ( specidx_ >= 0 )
	    dodisp = dodisp && specvars_.hasUnits(specidx_);
	unfld_->display( dodisp );
    }
    showHideVwBut();
}


void uiMathExpressionVariable::setActive( bool yn )
{
    isactive_ = yn;
    updateDisp();
}


void uiMathExpressionVariable::setVariable( const char* varnm, bool isconst )
{
    isconst_ = isconst;
    varnm_ = varnm;
    specidx_ = specvars_.getIndexOf( varnm );

    updateInpNms( true ); updateInpNms( false );

    BufferString lbltxt;
    bool issens = true;
    if ( isconst_ )
	constfld_->setTitleText( BufferString("Value for '",varnm_,"'") );
    if ( specidx_ < 0 )
	inplbl_->setText( BufferString("For '", varnm_,"' use") );
    else
    {
	inplbl_->setText( BufferString("'",varnm_,"' filled with") );
	inpfld_->setCurrentItem( specvars_.dispName(specidx_) );
	issens = false;
	if ( unfld_ && specvars_.hasUnits(specidx_) )
	    unfld_->setPropType( specvars_.propType(specidx_) );
    }
    inpfld_->setSensitive( issens );

    setActive( true );
    showHideVwBut( 0 );
}


void uiMathExpressionVariable::use( const Math::Expression* expr )
{
    varnm_.setEmpty();
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    if ( varidx_ >= nrvars )
	{ setActive( false ); return; }
    const BufferString varnm = expr->uniqueVarName( varidx_ );

    const int varidx = expr->indexOfUnVarName( expr->uniqueVarName(varidx_) );
    setVariable( varnm, expr->getType(varidx) == Math::Expression::Constant );
}


void uiMathExpressionVariable::use( const Math::Formula& form )
{
    specvars_ = form.specVars();
    varnm_.setEmpty();
    const int nrvars = form.nrInputs();
    if ( varidx_ >= nrvars )
	{ setActive( false ); return; }

    const BufferString varnm = form.variableName( varidx_ );

    setVariable( varnm, form.isConst( varidx_ ) );

    if ( isconst_ )
	constfld_->setValue( toFloat(form.inputDef(varidx_)) );
    else if ( specidx_ < 0 )
    {
	BufferString inpdef( form.inputDef(varidx_) );
	selectInput( inpdef.isEmpty() ? varnm.buf() : inpdef.buf() );
    }
    else if ( unfld_ )
	unfld_->setPropType( form.specVars()[specidx_].type_ );

    setUnit( form.inputUnit(varidx_) );
}


void uiMathExpressionVariable::selectInput( const char* inpnm, bool exact )
{
    if ( !inpnm ) inpnm = "";
    if ( inpfld_->isEmpty() )
	{ isconst_ = true; updateDisp(); return; }

    isconst_ = false;
    updateDisp();
    BufferString varnm( inpnm ), subnm;
    if ( subinpfld_ )
    {
	const FileMultiString fms( inpnm );
	varnm = fms[0]; subnm = fms[1];
    }
    if ( !exact )
    {
	BufferStringSet avnms; inpfld_->getItems( avnms );
	const int nearidx = avnms.nearestMatch( varnm );
	varnm = avnms.get( nearidx );
    }

    inpfld_->setCurrentItem( varnm );
    if ( subinpfld_ )
	subinpfld_->setCurrentItem( subnm );
}


const char* uiMathExpressionVariable::getInput() const
{
    if ( isconst_ )
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
	return 0;
    return unfld_->getUnit();
}


void uiMathExpressionVariable::fill( Math::Formula& form ) const
{
    if ( form.nrInputs() <= varidx_ )
	return;

    form.setInputDef( varidx_, getInput() );
    form.setInputUnit( varidx_, getUnit() );
}


void uiMathExpressionVariable::setUnit( const char* s )
{
    if ( unfld_ )
	unfld_->setUnit( s );
}


void uiMathExpressionVariable::setUnit( const UnitOfMeasure* uom )
{
    if ( unfld_ )
	unfld_->setUnit( uom );
}


void uiMathExpressionVariable::setPropType( PropertyRef::StdType typ )
{
    if ( unfld_ )
	unfld_->setPropType( typ );
}


void uiMathExpressionVariable::selectSubInput( int idx )
{
    if ( !subinpfld_ || subinpfld_->isEmpty() )
	return;

    subinpfld_->setCurrentItem( idx );
}

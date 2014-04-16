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
#include "mathformula.h"
#include "mathexpression.h"
#include "unitofmeasure.h"

static const char* specvararr[] = { "MD", "DZ", 0 };
static const BufferStringSet specvars( specvararr );

uiMathExpressionVariable::uiMathExpressionVariable( uiParent* p,
		    int varidx, bool withunit, const BufferStringSet* inpnms )
    : uiGroup(p,BufferString("MathExprVar ",varidx))
    , varidx_(varidx)
    , isconst_(false)
    , isactive_(true)
    , unfld_(0)
    , vwbut_(0)
    , inpSel(this)
{
    BufferStringSet emptybss;
    if ( !inpnms ) inpnms = &emptybss;
    const BufferString lblstr( "For input number ", varidx_, " use" );
    varfld_ = new uiLabeledComboBox( this, *inpnms, lblstr,
				     BufferString("input ",varidx_) );
    varfld_->label()->setPrefWidthInChar( 35 );
    varfld_->label()->setAlignment( Alignment::Right );
    int selidx = varidx_;
    if ( selidx >= inpnms->size() )
	selidx = inpnms->size() - 1;
    varfld_->box()->setCurrentItem( selidx );
    varfld_->box()->selectionChanged.notify(
			mCB(this,uiMathExpressionVariable,selChg) );

    constfld_ = new uiGenInput( this, "Value for 'c0'", FloatInpSpec() );
    constfld_->attach( alignedWith, varfld_ );

    if ( withunit )
    {
	uiUnitSel::Setup uussu( PropertyRef::Other, "convert to:" );
	uussu.withnone( true );
	unfld_ = new uiUnitSel( this, uussu );
    }

    setHAlignObj( varfld_ );
    preFinalise().notify( mCB(this,uiMathExpressionVariable,initFlds) );
}


void uiMathExpressionVariable::addInpViewIcon( const char* icnm, const char* tt,
						const CallBack& cb )
{
    vwbut_ = new uiToolButton( varfld_, "view_log", tt, cb );
    vwbut_->attach( rightOf, varfld_->box() );
    inpSel.notify( mCB(this,uiMathExpressionVariable,showHideVwBut) );
}


void uiMathExpressionVariable::initFlds( CallBacker* )
{
    updateDisp();
    if ( unfld_ )
    {
	unfld_->attach( rightTo, varfld_ );
	unfld_->attach( ensureRightOf, constfld_ );
    }
}


void uiMathExpressionVariable::showHideVwBut( CallBacker* )
{
    if ( vwbut_ )
	vwbut_->display( !isConst() );
}


void uiMathExpressionVariable::updateDisp()
{
    varfld_->display( isactive_ && !isconst_ );
    constfld_->display( isactive_ && isconst_ );
    if ( unfld_ )
	unfld_->display( isactive_ && !isconst_ );
}


void uiMathExpressionVariable::setActive( bool yn )
{
    isactive_ = yn;
    updateDisp();
}


void uiMathExpressionVariable::setVariable( const char* varnm )
{
    varnm_ = varnm;
    BufferString lbltxt( "For '", varnm_,  "' use" );
    varfld_->label()->setText( lbltxt.buf() );
    lbltxt.set( "Value for '" ).add( varnm_ ).add( "'" );
    constfld_->setTitleText( lbltxt );
    setActive( true );
}


void uiMathExpressionVariable::use( const Math::Expression* expr )
{
    varnm_.setEmpty();
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    if ( varidx_ >= nrvars )
	{ setActive( false ); return; }
    const BufferString varnm = expr->uniqueVarName( varidx_ );
    if ( specvars.isPresent(varnm.buf()) )
	{ setActive( false ); return; }

    const int varidx = expr->indexOfUnVarName( expr->uniqueVarName(varidx_) );
    isconst_ = expr->getType(varidx) == Math::Expression::Constant;
    setVariable( varnm );
}


void uiMathExpressionVariable::use( const Math::Formula& form )
{
    varnm_.setEmpty();
    const int nrvars = form.nrInputs();
    if ( varidx_ >= nrvars )
	{ setActive( false ); return; }
    const BufferString varnm = form.variableName( varidx_ );
    if ( specvars.isPresent(varnm.buf()) )
	{ setActive( false ); return; }

    isconst_ = form.isConst( varidx_ );
    setVariable( varnm );

    if ( isconst_ )
	constfld_->setValue( toFloat(form.inputDef(varidx_)) );
    else
    {
	BufferString inpdef( form.inputDef(varidx_) );
	selectInput( inpdef.isEmpty() ? varnm.buf() : inpdef.buf() );
	constfld_->setValue( mUdf(float) );
    }

    setUnit( form.inputUnit(varidx_) );
}


void uiMathExpressionVariable::selectInput( const char* inpnm, bool exact )
{
    if ( !inpnm ) inpnm = "";

    if ( varfld_->box()->isEmpty() )
    {
	isconst_ = true;
	updateDisp();
	return;
    }

    isconst_ = false;
    updateDisp();
    BufferString varnm( inpnm );
    if ( !exact )
    {
	BufferStringSet avnms; varfld_->box()->getItems( avnms );
	const int nearidx = avnms.nearestMatch( inpnm );
	varnm = avnms.get( nearidx );
    }

    varfld_->box()->setCurrentItem( varnm );
}


const char* uiMathExpressionVariable::getInput() const
{
    return isconst_ ? constfld_->text() : varfld_->box()->text();
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


void uiMathExpressionVariable::selChg( CallBacker* )
{
    inpSel.trigger();
}

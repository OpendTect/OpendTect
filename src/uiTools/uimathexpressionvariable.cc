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
		    int varidx, bool withunit, const Math::SpecVarSet* svs )
    : uiGroup(p,BufferString("MathExprVar ",varidx))
    , varidx_(varidx)
    , isconst_(false)
    , specidx_(-1)
    , isactive_(true)
    , unfld_(0)
    , vwbut_(0)
    , specvars_(*new Math::SpecVarSet(svs?*svs:emptsvs))
    , inpSel(this)
{
    BufferStringSet inpnms; getInpNms( inpnms );
    const BufferString lblstr( "For input number ", varidx_+1, " use" );
    varfld_ = new uiLabeledComboBox( this, inpnms, lblstr,
				     BufferString("input ",varidx_+1) );
    varfld_->label()->setPrefWidthInChar( 35 );
    varfld_->label()->setAlignment( Alignment::Right );
    int selidx = varidx_;
    if ( selidx >= inpnms.size() )
	selidx = 0;
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


uiMathExpressionVariable::~uiMathExpressionVariable()
{
    delete &specvars_;
}


void uiMathExpressionVariable::addInpViewIcon( const char* icnm, const char* tt,
						const CallBack& cb )
{
    vwbut_ = new uiToolButton( varfld_, "view_log", tt, cb );
    vwbut_->attach( rightOf, varfld_->box() );
    inpSel.notify( mCB(this,uiMathExpressionVariable,showHideVwBut) );
}


void uiMathExpressionVariable::getInpNms( BufferStringSet& nms ) const
{
    nms.setEmpty();
    if ( specidx_ < 0 )
	nms = nonspecinputs_;
    else
	specvars_.getNames( nms );
}


void uiMathExpressionVariable::updateInpNms()
{
    const BufferString curseltxt( varfld_->box()->text() );
    varfld_->box()->setEmpty();

    BufferStringSet nms; getInpNms( nms );
    varfld_->box()->addItems( nms );
    varfld_->box()->setCurrentItem( curseltxt );
}


void uiMathExpressionVariable::setNonSpecInputs( const BufferStringSet& nms )
{
    nonspecinputs_ = nms;
    updateInpNms();
}


uiGroup* uiMathExpressionVariable::rightMostField()
{
    if ( unfld_ )
	return unfld_;
    return varfld_;
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
	vwbut_->display( !isconst_ && specidx_ < 0 );
}


void uiMathExpressionVariable::updateDisp()
{
    varfld_->display( isactive_ && !isconst_ );
    constfld_->display( isactive_ && isconst_ );
    if ( unfld_ )
    {
	bool dodisp = isactive_ && !isconst_;
	if ( specidx_ >= 0 )
	    dodisp = dodisp && specvars_.hasUnits(specidx_);
	unfld_->display( dodisp );
    }
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

    updateInpNms();

    BufferString lbltxt;
    bool issens = true;
    if ( isconst_ )
	constfld_->setTitleText( BufferString("Value for '",varnm_,"'") );
    if ( specidx_ < 0 )
	varfld_->label()->setText( BufferString("For '", varnm_,"' use") );
    else
    {
	varfld_->label()->setText( BufferString("'",varnm_,"' filled with") );
	varfld_->box()->setCurrentItem( specvars_.dispName(specidx_) );
	issens = false;
	if ( unfld_ && specvars_.hasUnits(specidx_) )
	    unfld_->setPropType( specvars_.propType(specidx_) );
    }
    varfld_->box()->setSensitive( issens );

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

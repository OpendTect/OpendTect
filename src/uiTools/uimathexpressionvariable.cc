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
#include "uirockphysform.h"

#include "separstr.h"
#include "mathformula.h"
#include "mathexpression.h"
#include "unitofmeasure.h"

static const char* specvararr[] = { "MD", "DZ", 0 };
static const BufferStringSet specvars( specvararr );

uiMathExpressionVariable::uiMathExpressionVariable( uiGroup* inpgrp,
				const BufferStringSet& posinpnms,
				int varidx, bool displayuom )
    : uiGroup(inpgrp,"Inp data group")
    , varidx_(varidx)
    , posinpnms_(posinpnms)
    , unfld_(0)
    , inpSel(this)
{
    BufferString lblstr = "For 'input' use";
    inpfld_ = new uiLabeledComboBox( this, posinpnms_, lblstr.buf(),
				     BufferString("input ",varidx_) );
    inpfld_->label()->setPrefWidthInChar( 35 );
    inpfld_->label()->setAlignment( Alignment::Right );
    inpfld_->box()->addItem( "Constant" );
    int selidx = varidx_;
    if ( selidx >= posinpnms_.size() ) selidx = posinpnms_.size();
    inpfld_->box()->setCurrentItem( selidx );
    inpfld_->box()->selectionChanged.notify(
			mCB(this,uiMathExpressionVariable,selChg) );

    if ( displayuom )
    {
	uiUnitSel::Setup uussu( PropertyRef::Other, "convert to:" );
	uussu.withnone( true );
	unfld_ = new uiUnitSel( this, uussu );
	unfld_->attach( rightTo, inpfld_ );
    }

    cstvalfld_ = new uiGenInput( this, "value", FloatInpSpec() );
    cstvalfld_->attach( rightOf, inpfld_ );
    cstvalfld_->display( false );

    setHAlignObj( inpfld_ );
}


bool uiMathExpressionVariable::newVar( const char* varnm )
{
    varnm_ = varnm;
    display( true );
    BufferString inplbl = "For '"; inplbl += varnm_; inplbl += "' use";
    inpfld_->label()->setText( inplbl.buf() );
    return true;
}


bool uiMathExpressionVariable::use( const Math::Expression* expr )
{
    varnm_.setEmpty();
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    if ( varidx_ >= nrvars )
	{ display( false ); return false; }
    const BufferString varnm = expr->uniqueVarName( varidx_ );
    if ( specvars.isPresent(varnm.buf()) )
	{ display( false ); return false; }
    return newVar( varnm );
}


bool uiMathExpressionVariable::use( const Math::Formula& form )
{
    varnm_.setEmpty();
    const int nrvars = form.nrInputs();
    if ( varidx_ >= nrvars )
	{ display( false ); return false; }
    const BufferString varnm = form.variableName( varidx_ );
    if ( specvars.isPresent(varnm.buf()) )
	{ display( false ); return false; }

    return newVar( varnm );
}


const char* uiMathExpressionVariable::getInput() const
{
    return inpfld_->box()->text();
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


const UnitOfMeasure* uiMathExpressionVariable::getUnit() const
{
    if ( !unfld_ || !unfld_->mainObject()->isDisplayed() )
	return 0;
    return unfld_->getUnit();
}


float uiMathExpressionVariable::getCstVal() const
{
    return cstvalfld_->mainObject()->isDisplayed() ? cstvalfld_->getfValue()
						   : mUdf(float);
}


bool uiMathExpressionVariable::isCst() const
{
    return cstvalfld_->mainObject()->isDisplayed();
}


void uiMathExpressionVariable::selChg( CallBacker* )
{
    const int selidx = inpfld_->box()->currentItem();
    const bool iscst = selidx == posinpnms_.size();
    if ( unfld_ ) unfld_->display( !iscst );
    if ( cstvalfld_ ) cstvalfld_->display( iscst );

    inpSel.trigger();
}


void uiMathExpressionVariable::setCurSelIdx( int idx )
{
    inpfld_->box()->setCurrentItem( idx );
}

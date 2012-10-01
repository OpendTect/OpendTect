/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		March 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id: uimathexpressionvariable.cc -1   $";


#include "uimathexpressionvariable.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimathexpression.h"
#include "uimsg.h"
#include "uirockphysform.h"

#include "separstr.h"
#include "mathexpression.h"
#include "unitofmeasure.h"

static const char* specvararr[] = { "MD", "DZ", 0 };
static const BufferStringSet specvars( specvararr );

uiMathExpressionVariable::uiMathExpressionVariable( uiGroup* inpgrp, 
				const BufferStringSet& posinpnms,
       				int curselidx, bool displayuom )
    : uiGroup(inpgrp,"Inp data group")
    , idx_(curselidx)
    , posinpnms_(posinpnms)
    , unfld_(0)
{
    BufferString lblstr = "For 'input' use";
    inpfld_ = new uiLabeledComboBox( this, posinpnms_, lblstr.buf(),
	    			     BufferString("input ",curselidx) );
    inpfld_->label()->setPrefWidthInChar( 35 );
    inpfld_->label()->setAlignment( Alignment::Right );
    inpfld_->box()->addItem( "Constant" );
    int selidx = curselidx;
    if ( selidx >= posinpnms_.size() ) selidx = posinpnms_.size();
    inpfld_->box()->setCurrentItem( selidx );
    inpfld_->box()->selectionChanged.notify(
	    		mCB(this,uiMathExpressionVariable,selChg) );

    if ( displayuom )
    {
	unfld_ = new uiLabeledComboBox( this, "convert to:",
					BufferString( "unitbox ",curselidx ));
	const ObjectSet<const UnitOfMeasure>& alluom( UoMR().all() );
	unfld_->box()->addItem( "-" );
	for ( int idx=0; idx<alluom.size(); idx++ )
	    unfld_->box()->addItem( alluom[idx]->name() );
	unfld_->attach( rightOf, inpfld_ );
    }

    cstvalfld_ = new uiGenInput( this, "value", FloatInpSpec() );
    cstvalfld_->attach( rightOf, inpfld_ );
    cstvalfld_->display( false );

    setHAlignObj( inpfld_ );
}


void uiMathExpressionVariable::use( const MathExpression* expr )
{
    varnm_.setEmpty();
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    if ( idx_ >= nrvars )
	{ display( false ); return; }
    const BufferString varnm = expr->uniqueVarName( idx_ );
    if ( specvars.indexOf(varnm.buf()) >= 0 )
	{ display( false ); return; }

    varnm_ = varnm;
    display( true );
    BufferString inplbl = "For '"; inplbl += varnm; inplbl += "' use";
    inpfld_->label()->setText( inplbl.buf() );
}


bool uiMathExpressionVariable::hasVarName( const char* nm ) const
{
    return varnm_ == nm;
}


const char* uiMathExpressionVariable::getInput() const
{
    return inpfld_->box()->text();
}


void uiMathExpressionVariable::setUnit( const char* s )
{
    if ( unfld_ )
	unfld_->box()->setText( s );
}


const UnitOfMeasure* uiMathExpressionVariable::getUnit() const
{
    if ( !unfld_ || !unfld_->mainObject()->isDisplayed() ) return 0;
    return UoMR().get( unfld_->box()->text() );
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
}


BufferString uiMathExpressionVariable::getVarName() const
{
    return varnm_;
}


void uiMathExpressionVariable::setCurSelIdx( int idx )
{
    inpfld_->box()->setCurrentItem( idx );
}

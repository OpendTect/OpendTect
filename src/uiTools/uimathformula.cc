/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uimathformula.h"

#include "uimathexpression.h"
#include "uimathexpressionvariable.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"
#include "uimsg.h"

#include "mathformula.h"


uiMathFormula::uiMathFormula( uiParent* p, const uiMathFormula::Setup& su )
	: uiGroup(p,"Math Formula")
	, setup_(su)
	, unitfld_(0)
	, formSet(this)
	, inpSet(this)
	, formUnitSet(this)
{
    const CallBack formsetcb( mCB(this,uiMathFormula,formSetCB) );
    const CallBack inpsetcb( mCB(this,uiMathFormula,inpSetCB) );
    const CallBack unitsetcb( mCB(this,uiMathFormula,formUnitSetCB) );

    uiMathExpression::Setup mesu( setup_.label_ );
    mesu.withsetbut( true ).fnsbelow( false );
    exprfld_ = new uiMathExpression( this, mesu );
    exprfld_->formSet.notify( formsetcb );
    setHAlignObj( exprfld_ );

    for ( int idx=0; idx<setup_.maxnrvars_; idx++ )
    {
	uiMathExpressionVariable* fld = new uiMathExpressionVariable(this,idx,
							setup_.withunits_);
	if ( idx )
	    fld->attach( alignedBelow, varflds_[idx-1] );
	else
	    fld->attach( alignedBelow, exprfld_ );
	fld->inpSel.notify( inpsetcb );
	varflds_ += fld;
    }

    if ( setup_.withunits_ )
    {
	uiUnitSel::Setup uussu( PropertyRef::Other, "Formula result is" );
	uussu.selproptype( true ).withnone( true );
	unitfld_ = new uiUnitSel( this, uussu );
	unitfld_->attach( alignedBelow,
				varflds_[varflds_.size()-1]->attachObj() );
	unitfld_->propSelChange.notify( unitsetcb );
	unitfld_->selChange.notify( unitsetcb );
    }

    recbut_ = new uiToolButton( this, "recursion",
	    			"Set start valus for recursion",
				mCB(this,uiMathFormula,recButPush) );
    if ( unitfld_ )
	recbut_->attach( rightTo, unitfld_ );
    else
	recbut_->attach( rightTo, exprfld_ );
    recbut_->attach( rightBorder );
    recbut_->display( false );

    postFinalise().notify( formsetcb );
}


uiButton* uiMathFormula::addButton( const uiToolButtonSetup& tbs )
{
    return exprfld_->addButton( tbs );
}


void uiMathFormula::addInpViewIcon( const char* icnm, const char* tooltip,
					const CallBack& cb )
{
    for ( int idx=0; idx<varflds_.size(); idx++ )
	varflds_[idx]->addInpViewIcon( icnm, tooltip, cb );
}


bool uiMathFormula::checkValidNrInputs( const Math::Formula& form ) const
{
    if ( form.nrInputs() > varflds_.size() )
    {
	BufferString msg( "Sorry, the expression contains ", form.nrInputs(),
			  "variables.\nThe maximum number is " );
	msg.add( varflds_.size() );
	uiMSG().error( msg );
	return false;
    }
    return true;
}


bool uiMathFormula::setText( const char* txt )
{
    Math::Formula form( txt );
    return useForm( form );
}


bool uiMathFormula::updateForm( Math::Formula& form ) const
{
    form.setText( exprfld_->text() );
    if ( !checkValidNrInputs(form) )
	return false;

    for ( int idx=0; idx<form.nrInputs(); idx++ )
	varflds_[idx]->fill( form );

    if ( unitfld_ )
	form.setOutputUnit( unitfld_->getUnit() );

    const int nrrec = form.maxRecShift();
    for ( int idx=0; idx<nrrec; idx++ )
	form.recStartVals()[idx] = idx >= recvals_.size() ? 0 : recvals_[idx];

    return true;
}


bool uiMathFormula::useForm( const Math::Formula& form,
			     const TypeSet<PropertyRef::StdType>* inputtypes )
{
    const bool isbad = form.isBad();
    exprfld_->setText( isbad ? "" : form.text() );
    const UnitOfMeasure* formun = isbad ? 0 : form.outputUnit();
    if ( unitfld_ )
	unitfld_->setUnit( formun );
    for ( int idx=0; idx<varflds_.size(); idx++ )
    {
	uiMathExpressionVariable& inpfld = *varflds_[idx];
	if ( !isbad && idx<form.nrInputs() )
	{
	    const PropertyRef::StdType ptyp
		= inputtypes && inputtypes->validIdx(idx) ? (*inputtypes)[idx]
							  : PropertyRef::Other;
	    inpfld.setPropType( ptyp );
	}
	inpfld.use( form );
    }
    recbut_->display( form.isRecursive() );

    if ( isbad )
    {
	uiMSG().error( BufferString("Invalid expression:\n",form.errMsg()));
	return false;
    }

    return checkValidNrInputs( form );
}


void uiMathFormula::formSetCB( CallBacker* )
{
    Math::Formula form;
    if ( updateForm(form) )
    {
	form.clearInputDefs();
	useForm( form );
    }
}


void uiMathFormula::inpSetCB( CallBacker* )
{
    inpSet.trigger();
}


void uiMathFormula::formUnitSetCB( CallBacker* )
{
    formUnitSet.trigger();
}


void uiMathFormula::recButPush( CallBacker* )
{
    uiMSG().error( "TODO: set recursion start values" );
}

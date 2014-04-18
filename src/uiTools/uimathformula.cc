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
#include "uidialog.h"
#include "uigeninput.h"
#include "uimsg.h"

#include "mathformula.h"


uiMathFormula::uiMathFormula( uiParent* p, Math::Formula& form,
				const uiMathFormula::Setup& su )
	: uiGroup(p,"Math Formula")
	, form_(form)
	, setup_(su)
	, unitfld_(0)
	, recbut_(0)
	, formSet(this)
	, inpSet(this)
	, formUnitSet(this)
	, notifinpnr_(-1)
{
    const CallBack formsetcb( mCB(this,uiMathFormula,formSetCB) );
    const CallBack inpsetcb( mCB(this,uiMathFormula,inpSetCB) );
    const CallBack unitsetcb( mCB(this,uiMathFormula,formUnitSetCB) );

    uiMathExpression::Setup mesu( setup_.label_ );
    mesu.withsetbut( true ).fnsbelow( false );
    exprfld_ = new uiMathExpression( this, mesu );
    exprfld_->formSet.notify( formsetcb );
    setHAlignObj( exprfld_ );

    for ( int idx=0; idx<setup_.maxnrinps_; idx++ )
    {
	uiMathExpressionVariable* fld = new uiMathExpressionVariable(this,idx,
				    setup_.withunits_,&form_.specVars());
	if ( idx )
	    fld->attach( alignedBelow, inpflds_[idx-1] );
	else
	    fld->attach( alignedBelow, exprfld_ );
	fld->inpSel.notify( inpsetcb );
	inpflds_ += fld;
    }

    if ( setup_.withunits_ )
    {
	uiUnitSel::Setup uussu( PropertyRef::Other, "Formula result is" );
	uussu.selproptype( true ).withnone( true );
	unitfld_ = new uiUnitSel( this, uussu );
	unitfld_->attach( alignedBelow,
				inpflds_[inpflds_.size()-1]->attachObj() );
	unitfld_->propSelChange.notify( unitsetcb );
	unitfld_->selChange.notify( unitsetcb );
    }

    if ( form_.inputsAreSeries() )
    {
	recbut_ = new uiToolButton( this, "recursion",
				    "Set start values for recursion",
				    mCB(this,uiMathFormula,recButPush) );
	if ( unitfld_ )
	    recbut_->attach( rightTo, unitfld_ );
	else
	    recbut_->attach( rightTo, exprfld_ );
	recbut_->attach( rightBorder );
	recbut_->display( false );
    }

    postFinalise().notify( formsetcb );
}


uiButton* uiMathFormula::addButton( const uiToolButtonSetup& tbs )
{
    return exprfld_->addButton( tbs );
}


void uiMathFormula::setNonSpecInputs( const BufferStringSet& inps, int ivar )
{
    for ( int idx=0; idx<inpflds_.size(); idx++ )
    {
	if ( ivar < 0 || ivar == idx )
	    inpflds_[idx]->setNonSpecInputs( inps );
    }
}


void uiMathFormula::addInpViewIcon( const char* icnm, const char* tooltip,
					const CallBack& cb )
{
    for ( int idx=0; idx<inpflds_.size(); idx++ )
	inpflds_[idx]->addInpViewIcon( icnm, tooltip, cb );
}


bool uiMathFormula::checkValidNrInputs() const
{
    if ( form_.nrInputs() > inpflds_.size() )
    {
	BufferString msg( "Sorry, the expression contains ", form_.nrInputs(),
			  "inputs.\nThe maximum number is " );
	msg.add( inpflds_.size() );
	uiMSG().error( msg );
	return false;
    }
    return true;
}


bool uiMathFormula::setText( const char* txt )
{
    form_.setText( txt );
    return useForm();
}


bool uiMathFormula::updateForm() const
{
    form_.setText( exprfld_->text() );
    if ( !checkValidNrInputs() )
	return false;

    for ( int idx=0; idx<form_.nrInputs(); idx++ )
	inpflds_[idx]->fill( form_ );

    if ( unitfld_ )
	form_.setOutputUnit( unitfld_->getUnit() );

    const int nrrec = form_.maxRecShift();
    for ( int idx=0; idx<nrrec; idx++ )
	form_.recStartVals()[idx] = idx >= recvals_.size() ? 0 : recvals_[idx];

    return true;
}


bool uiMathFormula::useForm( const TypeSet<PropertyRef::StdType>* inputtypes )
{
    const bool isbad = form_.isBad();
    exprfld_->setText( isbad ? "" : form_.text() );
    const UnitOfMeasure* formun = isbad ? 0 : form_.outputUnit();
    if ( unitfld_ )
	unitfld_->setUnit( formun );
    for ( int idx=0; idx<inpflds_.size(); idx++ )
    {
	uiMathExpressionVariable& inpfld = *inpflds_[idx];
	if ( !isbad && idx<form_.nrInputs() )
	{
	    const PropertyRef::StdType ptyp
		= inputtypes && inputtypes->validIdx(idx) ? (*inputtypes)[idx]
							  : PropertyRef::Other;
	    inpfld.setPropType( ptyp );
	}
	inpfld.use( form_ );
    }

    if ( recbut_ )
	recbut_->display( form_.isRecursive() );

    if ( isbad )
    {
	uiMSG().error( BufferString("Invalid expression:\n",form_.errMsg()));
	return false;
    }

    return checkValidNrInputs();
}


int uiMathFormula::vwLogInpNr( CallBacker* cb ) const
{
    for ( int idx=0; idx<inpflds_.size(); idx++ )
	if ( inpflds_[idx]->viewBut() == cb )
	    return idx;

    return -1;
}


int uiMathFormula::nrInputs() const
{
    int nrinps = 0;
    for ( int idx=0; idx<inpflds_.size(); idx++ )
	if ( inpflds_[idx]->isActive() )
	    nrinps++;
    return nrinps;
}


const char* uiMathFormula::getInput( int inpidx ) const
{
    return inpflds_.validIdx(inpidx) ? inpflds_[inpidx]->getInput() : "";
}


bool uiMathFormula::isConst( int inpidx ) const
{
    if ( !inpflds_.validIdx(inpidx) )
	return false;
    const uiMathExpressionVariable& fld = *inpflds_[inpidx];
    return fld.isActive() && fld.isConst();
}


bool uiMathFormula::isSpec( int inpidx ) const
{
    if ( !inpflds_.validIdx(inpidx) )
	return false;
    const uiMathExpressionVariable& fld = *inpflds_[inpidx];
    return fld.isActive() && fld.specIdx() >= 0;
}


double uiMathFormula::getConstVal( int inpidx ) const
{
    return isConst(inpidx) ? toDouble( getInput(inpidx) )
			   : mUdf(double);
}


const UnitOfMeasure* uiMathFormula::getUnit() const
{
    return unitfld_ ? unitfld_->getUnit() : 0;
}


void uiMathFormula::formSetCB( CallBacker* )
{
    form_.setText( exprfld_->text() );
    form_.clearInputDefs();
    useForm();
    formSet.trigger();
}


void uiMathFormula::inpSetCB( CallBacker* cb )
{
    notifinpnr_ = -1;
    for ( int idx=0; idx<inpflds_.size(); idx++ )
    {
	if ( inpflds_[idx] == cb )
	    { notifinpnr_ = idx; break; }
    }
    inpSet.trigger();
}


void uiMathFormula::formUnitSetCB( CallBacker* )
{
    formUnitSet.trigger();
}


class uiMathFormulaEdRec : public uiDialog
{
public:

uiMathFormulaEdRec( uiParent* p, Math::Formula& form, const char* s_if_2 )
    : uiDialog( this, Setup(
	BufferString("Recursion start value",s_if_2),
	BufferString("Recursive formula: Starting value",s_if_2),
	mNoHelpKey) )
    , form_(form)
{
    for ( int idx=0; idx<form_.maxRecShift(); idx++ )
    {
	inpflds_ += new uiGenInput( this, BufferString("Value at ",-1-idx),
				    DoubleInpSpec(form_.recStartVals()[idx]) );
	if ( idx > 0 )
	    inpflds_[idx]->attach( alignedBelow, inpflds_[idx-1] );
    }
}

bool acceptOK( CallBacker* )
{
    for ( int idx=0; idx<form_.maxRecShift(); idx++ )
    {
	const double val = inpflds_[idx]->getdValue();
	if ( mIsUdf(val) )
	    { uiMSG().error( "Please specify all values" ); return false; }
	form_.recStartVals()[idx] = val;
    }
    return true;
}

    ObjectSet<uiGenInput>	inpflds_;
    Math::Formula&		form_;

};


void uiMathFormula::recButPush( CallBacker* )
{
    if ( !recbut_ || !updateForm() )
	return;
    if ( !form_.isRecursive() )
	{ recbut_->display(false); return; }

    uiMathFormulaEdRec dlg( this, form_, form_.maxRecShift() > 1 ? "s" : 0 );
    if ( !dlg.go() )
	return;

    recvals_ = form_.recStartVals();
}

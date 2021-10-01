/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2014
________________________________________________________________________

-*/


#include "uimathformula.h"

#include "uibuttongroup.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uiioobjseldlg.h"
#include "uimathexpression.h"
#include "uimathexpressionvariable.h"
#include "uimnemonicsel.h"
#include "uimsg.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "mathformula.h"
#include "mathformulatransl.h"
#include "mathspecvars.h"
#include "od_iostream.h"
#include "unitofmeasure.h"


uiMathFormula::uiMathFormula( uiParent* p, Math::Formula& form,
				const uiMathFormula::Setup& su )
    : uiGroup(p,"Math Formula")
    , form_(form)
    , ctio_(*mMkCtxtIOObj(MathFormula))
    , setup_(su)
    , inpSet(this)
    , subInpSet(this)
    , formMnSet(this)
    , formUnitSet(this)
{
    const bool wantio = !setup_.stortype_.isEmpty();
    if ( wantio )
	ctio_.ctxt_.toselect_.require_.set( sKey::Type(), setup_.stortype_ );

    uiMathExpression::Setup mesu( setup_.label_ );
    mesu.withsetbut( true ).fnsbelow( false ).specvars( &form_.specVars() );
    exprfld_ = new uiMathExpression( this, mesu );
    mAttachCB( exprfld_->formSet, uiMathFormula::formSetCB );
    setHAlignObj( exprfld_ );

    for ( int idx=0; idx<setup_.maxnrinps_; idx++ )
    {
	auto* fld = new uiMathExpressionVariable( this, idx,
				    setup_.withunits_, setup_.withsubinps_,
				    &form_.specVars() );
	if ( idx )
	    fld->attach( alignedBelow, inpflds_[idx-1] );
	else
	    fld->attach( alignedBelow, exprfld_ );

	mAttachCB( fld->inpSel, uiMathFormula::inpSetCB );
	mAttachCB( fld->subInpSel, uiMathFormula::subInpSetCB );
	inpflds_ += fld;
    }

    if ( setup_.withunits_ )
    {
	uiObject* aboveobj = inpflds_[inpflds_.size()-1]->attachObj();
	const Mnemonic* mn = setup_.mn_ ? setup_.mn_ : &Mnemonic::undef();
	const bool havemnemonic = !mn->isUdf();
	const uiString formreslbl = tr("Formula result is");
	if ( !havemnemonic )
	{
	    uiMnemonicsSel::Setup uimnsu( nullptr, formreslbl );
	    mnselfld_ = new uiMnemonicsSel( this, uimnsu );
	    mnselfld_->attach( alignedBelow, aboveobj );
	    mAttachCB( mnselfld_->box()->selectionChanged,
		       uiMathFormula::formMnSetCB );
	}

	const uiString unitlbl = mnselfld_ ? tr("in") : formreslbl;
	const uiUnitSel::Setup uussu( mn->stdType(),
				havemnemonic ? tr("Formula output unit") :
				unitlbl );
	unitfld_ = new uiUnitSel( this, uussu );
	if ( mnselfld_ )
	    unitfld_->attach( rightOf, mnselfld_ );
	else
	    unitfld_->attach( alignedBelow, aboveobj );

	mAttachCB( unitfld_->selChange, uiMathFormula::formUnitSetCB );
    }

    uiButtonGroup* bgrp = nullptr;
    if ( wantio || form_.inputsAreSeries() )
	bgrp = new uiButtonGroup( this, "tool buts", OD::Horizontal );

    if ( form_.inputsAreSeries() )
	recbut_ = new uiToolButton( bgrp, "recursion",
				    tr("Set start values for recursion"),
				    mCB(this,uiMathFormula,recButPush) );
    if ( wantio )
    {
	new uiToolButton( bgrp, "open", tr("Open stored formula"),
				    mCB(this,uiMathFormula,readReq) );
	new uiToolButton( bgrp, "save", tr("Save this formula"),
				    mCB(this,uiMathFormula,writeReq) );
    }

    if ( bgrp )
    {
	bgrp->attach( rightTo, exprfld_ );
	bgrp->attach( rightBorder );
    }

    mAttachCB( postFinalise(), uiMathFormula::initFlds );
}


uiMathFormula::~uiMathFormula()
{
    detachAllNotifiers();
    delete ctio_.ioobj_;
    delete &ctio_;
    delete mnsel_;
}


uiButton* uiMathFormula::addButton( const uiToolButtonSetup& tbs )
{
    return exprfld_->addButton( tbs );
}


void uiMathFormula::setNonSpecInputs( const BufferStringSet& inps, int ivar,
				      const MnemonicSelection* mnsel )
{
    delete mnsel_;
    mnsel_ = mnsel ? new MnemonicSelection( *mnsel ) : nullptr;

    for ( int idx=0; idx<inpflds_.size(); idx++ )
    {
	if ( ivar < 0 || ivar == idx )
	    inpflds_[idx]->setNonSpecInputs( inps, mnsel_ );
    }
}


void uiMathFormula::setNonSpecSubInputs( const BufferStringSet& inps, int ivar )
{
    for ( int idx=0; idx<inpflds_.size(); idx++ )
    {
	if ( ivar < 0 || ivar == idx )
	    inpflds_[idx]->setNonSpecSubInputs( inps );
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
	uiString msg = tr("Sorry, the expression contains %1"
			  " inputs.\nThe maximum number is %2")
		     .arg(form_.nrInputs()).arg(inpflds_.size());
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


const char* uiMathFormula::text() const
{
    return exprfld_->text();
}


bool uiMathFormula::updateForm() const
{
    form_.setText( exprfld_->text() );
    if ( !checkValidNrInputs() )
	return false;

    for ( int idx=0; idx<nrInputs(); idx++ )
	inpflds_[idx]->fill( form_ );

    if ( unitfld_ )
    {
	const Mnemonic* mn = mnselfld_ ? mnselfld_->mnemonic() : setup_.mn_;
	if ( !mn || mn->isUdf() )
	{
	    if ( !uiMSG().askGoOn( tr("Formula return type is undefined.\n"
				      "Is this correct?" ) ) )
		return false;
	}

	form_.setOutputMnemonic( mn );
	form_.setOutputFormUnit( unitfld_->getUnit() );
    }

    const int nrrec = form_.maxRecShift();
    for ( int idx=0; idx<nrrec; idx++ )
	form_.recStartVals()[idx] = idx >= recvals_.size() ? 0 : recvals_[idx];

    return true;
}


bool uiMathFormula::useForm( const TypeSet<Mnemonic::StdType>* inputtypes )
{
    fixedunits_ = inputtypes;
    return useForm();
}


bool uiMathFormula::useForm()
{
    const bool isbad = form_.isBad();
    if ( !isbad )
	exprfld_->setText( form_.text() );

    guessInputFormDefs();
    for ( auto* inpfld : inpflds_ )
	inpfld->use( form_, fixedunits_ );

    if ( recbut_ )
	recbut_->display( form_.isRecursive() );

    if ( isbad )
    {
	uiMSG().error(tr("Invalid expression:\n%1").arg(form_.errMsg()));
	return false;
    }

    return checkValidNrInputs();
}


void uiMathFormula::guessInputFormDefs()
{
    if ( fixedunits_ )
	return;

    for ( int idx=0; idx<nrInputs(); idx++ )
    {
	if ( form_.isConst(idx) )
	    continue;

	const uiMathExpressionVariable* inpfld = inpflds_.get( idx );
	if ( form_.isSpec(idx) )
	    { /*TODO: impl;*/ continue; }

	const BufferString varnm = form_.variableName( idx );
	BufferString inpdef( form_.inputDef(idx) );
	if ( inpdef.isEmpty() )
	{
	    inpdef.set( varnm );
	    form_.setInputDef( idx, inpdef );
	}

	const BufferStringSet allinpnms = inpfld->getInputNms();
	if ( !mnsel_ )
	{
	    const int nearidx = allinpnms.nearestMatch( inpdef );
	    if ( allinpnms.validIdx(nearidx) )
	    {
		inpdef.set( allinpnms.get(nearidx) );
		form_.setInputDef( idx, inpdef );
	    }

	    continue;
	}

	bool found = false;
	const Mnemonic* mn = form_.inputMnemonic( idx );
	BufferStringSet inpnms = inpfld->getInputNms( mn );
	if ( mn && !mn->isUdf() )
	{
	    if ( !inpnms.isPresent(inpdef.str()) )
		for ( const auto* inpnm : inpnms )
		    if ( mn->matches(inpnm->str(),true) )
		    {
			inpdef.set( inpnm->str() );
			found = true;
			break;
		    }
	}
	else
	{
	    mn = mnsel_->getByName( inpdef );
	    inpnms = inpfld->getInputNms( mn );
	    if ( mn && !mn->isUdf() )
	    {
		if ( !inpnms.isPresent(inpdef.str()) )
		    for ( const auto* inpnm : inpnms )
			if ( mn->matches(inpnm->str(),true) )
			{
			    inpdef.set( inpnm->str() );
			    found = true;
			    break;
			}
	    }
	    else if ( (!mn || mn->isUdf()) && !inpnms.isEmpty() )
	    {
		const BufferString& firstinp = *inpnms.first();
		const int inpidx = allinpnms.indexOf( firstinp.str() );
		if ( mnsel_->validIdx(inpidx) )
		{
		    if ( !inpnms.isPresent(inpdef.str()) )
		    {
			inpdef.set( firstinp );
			found = true;
		    }
		}
	    }
	}

	if ( found )
	    form_.setInputDef( idx, inpdef );

	form_.setInputMnemonic( idx, mn ? mn : &Mnemonic::undef() );
	if ( mn && !mn->isUdf() && mn->unit() )
	    form_.setInputFormUnit( idx, UoMR().getInternalFor(mn->stdType()) );
    }
}


bool uiMathFormula::guessOutputFormDefs()
{
    if ( !unitfld_ )
	return true;

    if ( fixedunits_ )
	return setOutputDefsFromForm();

    TypeSet<int> varidxs;
    for ( int idx=0; idx<form_.nrInputs(); idx++ )
	if ( !form_.isConst(idx) && !form_.isSpec(idx) )
	    varidxs += idx;

    if ( varidxs.isEmpty() )
	return setOutputDefsFromForm();

    const uiMathExpressionVariable* firstinpfld = inpflds_.get( varidxs[0] );
    const UnitOfMeasure* inpuom = firstinpfld->getUnit();
    for ( int idx=1; idx<varidxs.size(); idx++ )
	if ( inpflds_.get( varidxs[idx] )->getUnit() != inpuom )
	    return setOutputDefsFromForm();

    if ( mnselfld_ )
    {
	const Mnemonic* mn = firstinpfld->getMnemonic();
	if ( !mn )
	    mn = &Mnemonic::undef();
	mnselfld_->setMnemonic( *mn );
	formMnSetCB( nullptr );
    }

    unitfld_->setUnit( inpuom );
    formUnitSetCB( nullptr );
    return true;
}


bool uiMathFormula::setOutputDefsFromForm()
{
    if ( mnselfld_ && form_.isOK() )
    {
	const Mnemonic* mn = form_.outputMnemonic();
	if ( !mn )
	    mn = &Mnemonic::undef();
	mnselfld_->setMnemonic( *mn );
	formMnSetCB( nullptr );
    }

    const UnitOfMeasure* formun = form_.isOK()
				? form_.outputFormUnit() : nullptr;
    unitfld_->setUnit( formun );
    formUnitSetCB( nullptr );
    return true;
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
    return form_.nrInputs();
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


void uiMathFormula::initFlds( CallBacker* )
{
    useForm();
}


void uiMathFormula::formSetCB( CallBacker* )
{
    const FixedString formtxt = form_.text();
    const bool changed = FixedString(exprfld_->text()) != formtxt;
    if ( !changed )
	return;

    form_.setText( exprfld_->text() );
    form_.clearAllDefs();
    fixedunits_ = false;
    useForm();
}


void uiMathFormula::inpSetCB( CallBacker* cb )
{
    int inpidx = -1;
    if ( !setNotifInpNr(cb,inpidx) )
	return;

    uiMathExpressionVariable* inpfld = inpflds_.get( inpidx );
    const bool isvariable = inpidx < nrInputs() &&
			    ( !isSpec(inpidx) || !isConst(inpidx) );
    if ( isvariable )
    {
	const BufferString inpdef = getInput( inpidx );
	const Mnemonic* mn = form_.inputMnemonic( inpidx );
	if ( mnsel_ && (!mn || mn->isUdf()) )
	    mn = mnsel_->getByName( inpdef );

	inpfld->setFormType( mn ? *mn : Mnemonic::undef() );
	const UnitOfMeasure* uom = form_.inputFormUnit( inpidx );
	const bool sensitive = !fixedunits_;
	inpfld->setFormUnit( uom, sensitive );
    }

    inpSet.trigger( inpfld );

    if ( isvariable )
	guessOutputFormDefs();
}


void uiMathFormula::subInpSetCB( CallBacker* cb )
{
    int notifinpnr = -1;
    if ( setNotifInpNr(cb,notifinpnr) )
	subInpSet.trigger( inpflds_.get(notifinpnr) );
}


bool uiMathFormula::setNotifInpNr( const CallBacker* cb, int& notifinpnr )
{
    notifinpnr = -1;
    for ( int idx=0; idx<inpflds_.size(); idx++ )
    {
	if ( inpflds_[idx] == cb )
	    { notifinpnr = idx; break; }
    }

    if ( !inpflds_.validIdx(notifinpnr) )
	{ pErrMsg("Huh" ); return false; }
    return true;
}


void uiMathFormula::formMnSetCB( CallBacker* )
{
    const Mnemonic* mn = mnselfld_->mnemonic();
    if ( !mn )
	mn = &Mnemonic::undef();
    unitfld_->setPropType( mn->stdType() );
    mnselfld_->setSensitive( !fixedunits_ );

    formMnSet.trigger( mnselfld_ );
}


void uiMathFormula::formUnitSetCB( CallBacker* )
{
    unitfld_->setSensitive( !fixedunits_ );
    formUnitSet.trigger( unitfld_ );
}


class uiMathFormulaEdRec : public uiDialog
{ mODTextTranslationClass(uiMathFormulaEdRec);
public:

uiMathFormulaEdRec( uiParent* p, Math::Formula& form, const char* s_if_2 )
    : uiDialog( this, Setup(
	tr("Recursion start value %1").arg(toUiString(s_if_2)),
	tr("Recursive formula: Starting value %1").arg(toUiString(s_if_2)),
	mNoHelpKey) )
    , form_(form)
{
    for ( int idx=0; idx<form_.maxRecShift(); idx++ )
    {
	inpflds_ += new uiGenInput( this, tr("Value at %1").arg(-1-idx),
				    DoubleInpSpec(form_.recStartVals()[idx]) );
	if ( idx > 0 )
	    inpflds_[idx]->attach( alignedBelow, inpflds_[idx-1] );
    }
}

bool acceptOK( CallBacker* )
{
    for ( int idx=0; idx<form_.maxRecShift(); idx++ )
    {
	const double val = inpflds_[idx]->getDValue();
	if ( mIsUdf(val) )
	{ uiMSG().error(tr("Please specify all values")); return false;	}
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


BufferString uiMathFormula::getIOFileName( bool forread )
{
    BufferString fnm;
    ctio_.ctxt_.forread_ = forread;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() )
	return fnm;

    fnm = dlg.ioObj()->fullUserExpr( forread );
    const bool doesexist = File::exists( fnm );
    if ( forread && !doesexist )
	{ uiMSG().error(uiStrings::sFileDoesntExist()); fnm.setEmpty(); }

    ctio_.setObj( dlg.ioObj()->clone() );
    return fnm;
}


void uiMathFormula::readReq( CallBacker* )
{
    const BufferString fnm = getIOFileName( true );
    if ( fnm.isEmpty() )
	return;

    od_istream strm( fnm );
    if ( !strm.isOK() )
	{ uiMSG().error(uiStrings::sCantOpenInpFile()); return; }

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(Math::Formula::sKeyFileType()) )
    {
	uiMSG().error( tr("Input file is not of 'Math Formula' type") );
	return;
    }

    IOPar iop( astrm );
    form_.usePar( iop );
    recvals_ = form_.recStartVals();
    useForm();
}


void uiMathFormula::writeReq( CallBacker* )
{
    if ( !updateForm() )
	return;

    const BufferString fnm = getIOFileName( false );
    if ( fnm.isEmpty() )
	return;

    FilePath fp( fnm ); fp.setExtension( "formula" );
    od_ostream strm( fp.fullPath() );
    if ( !strm.isOK() )
	{ uiMSG().error(uiStrings::sCantOpenOutpFile()); return; }
    ascostream astrm( strm );
    if ( !astrm.putHeader(Math::Formula::sKeyFileType()) )
	{ uiMSG().error(tr("Cannot write file header")); return; }

    IOPar iop; form_.fillPar( iop );
    iop.putTo( astrm );
}

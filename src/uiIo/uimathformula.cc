/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uimathformula.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uiioobjseldlg.h"
#include "uilabel.h"
#include "uimathexpression.h"
#include "uimathexpressionvariable.h"
#include "uimnemonicsel.h"
#include "uimsg.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"

#include "ascstream.h"
#include "file.h"
#include "filepath.h"
#include "mathexpression.h"
#include "mathformula.h"
#include "mathformulatransl.h"
#include "mathspecvars.h"
#include "odpair.h"
#include "rockphysics.h"
#include "unitofmeasure.h"


uiMathFormula::uiMathFormula( uiParent* p, Math::Formula& form,
			      const uiMathFormula::Setup& su )
    : uiGroup(p,"Math Formula")
    , form_(form)
    , setup_(su)
    , inpSet(this)
    , subInpSet(this)
    , formMnSet(this)
{
    mnselflds_.setNullAllowed();
    unitflds_.setNullAllowed();
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
	inpflds_.add( fld );
    }

    if ( setup_.withunits_ && !inpflds_.isEmpty() )
	addFormOutputsDefs();

    const bool wantio = !setup_.stortype_.isEmpty();
    uiButtonGroup* bgrp = nullptr;
    if ( wantio || form_.inputsAreSeries() )
	bgrp = new uiButtonGroup( this, "tool buts", OD::Horizontal );

    if ( wantio )
    {
	new uiToolButton( bgrp, "open", tr("Open stored formula"),
				    mCB(this,uiMathFormula,readReq) );
	new uiToolButton( bgrp, "save", tr("Save this formula"),
				    mCB(this,uiMathFormula,writeReq) );
    }

    if ( form_.inputsAreSeries() )
    {
	bgrp->nextButtonOnNewRowCol();
	recbut_ = new uiToolButton( bgrp, "recursion",
				    tr("Set start values for recursion"),
				    mCB(this,uiMathFormula,recButPush) );
    }

    if ( bgrp )
    {
	bgrp->attach( rightTo, exprfld_ );
	bgrp->attach( rightBorder );
    }

    mAttachCB( postFinalize(), uiMathFormula::initGrp );
}


uiMathFormula::~uiMathFormula()
{
    detachAllNotifiers();
    delete mnsel_;
}


void uiMathFormula::addFormOutputsDefs()
{
    auto* formresgrp = new uiGroup( this, "Formula result pars" );
    uiObject* alignobj = nullptr;
    formlbl_ = inpflds_.first()->getFormLbl();
    const Mnemonic* mn = setup_.mn_ ? setup_.mn_ : &Mnemonic::undef();
    const bool havemnemonic = !mn->isUdf();
    ManagedObjectSet<MnemonicSelection> mnsels;
    BufferStringSet typnms;
    if ( havemnemonic )
    {
	auto* mnsel = new MnemonicSelection;
	mnsel->add( mn );
	mnsels.add( mnsel );
	typnms.add( Mnemonic::toString(mn->stdType()) );
    }
    else
    {
	const BufferStringSet alltypnms( Mnemonic::StdTypeNames() );
	for ( const auto* typnm : alltypnms )
	{
	    const Mnemonic::StdType typ =
			    Mnemonic::parseEnumStdType( typnm->buf() );
	    const bool isall = typ == Mnemonic::Other;
	    MnemonicSelection mnsel( typ );
	    if ( mnsel.isEmpty() )
		continue;

	    if ( isall )
		mnsel = MnemonicSelection( nullptr );

	    mnsels.add( new MnemonicSelection(mnsel) );
	    typnms.add( typnm->buf() );
	}
    }

    if ( typnms.size() > 1 )
    {
	formreslbl_ = new uiLabel( formresgrp, tr("Formula result is") );
	typfld_ = new uiComboBox( formresgrp, typnms, "Property Type" );
	typfld_->setHSzPol( uiObject::SmallVar );
	typfld_->attach( rightTo, formreslbl_ );
	mAttachCB( typfld_->selectionChanged, uiMathFormula::formTypeSetCB);
	alignobj = typfld_;

	for ( int idx=0; idx<typnms.size(); idx++ )
	{
	    const MnemonicSelection& mnsel = *mnsels.get( idx );
	    if ( mnsel.size() < 2 )
	    {
		mnselflds_.add( nullptr );
		continue;
	    }

	    const uiMnemonicsSel::Setup uimnsu( &mnsel, uiString::empty() );
	    auto* mnselfld = new uiMnemonicsSel( formresgrp, uimnsu );
	    mnselfld->attach( rightTo, typfld_ );
	    mAttachCB( mnselfld->box()->selectionChanged,
		       uiMathFormula::formMnSetCB );
	    mnselflds_.add( mnselfld );
	}
    }

    for ( int idx=0; idx<mnsels.size(); idx++ )
    {
	const char* typnm = typnms.get(idx).buf();
	const Mnemonic::StdType typ = Mnemonic::parseEnumStdType( typnm );
	ObjectSet<const UnitOfMeasure> uoms;
	UoMR().getRelevant( typ, uoms );
	if ( uoms.isEmpty() )
	{
	    unitflds_.add( nullptr );
	    continue;
	}

	uiMnemonicsSel* mnselfld = mnselflds_.validIdx(idx)
				 ? mnselflds_.get( idx ) : nullptr;
	if ( !formreslbl_ )
	    formreslbl_ = new uiLabel(formresgrp,tr("Formula output unit"));

	uiObject* attachobj = mnselfld ? (uiObject*)mnselfld
				       : (typfld_ ? (uiObject*)typfld_
						  : (uiObject*)formreslbl_);

	const uiString unitlbl = typfld_ || mnselfld ? tr("in")
						     : uiString::empty();
	uiUnitSel::Setup uussu( typ, unitlbl );
	uussu.mode( uiUnitSel::Setup::SymbolsOnly )
	     .withnone( true ).variableszpol( true );
	auto* unitfld = new uiUnitSel( formresgrp, uussu );
	unitfld->attach( rightOf, attachobj );
	mAttachCB( unitfld->selChange, uiMathFormula::formUnitSetCB );
	unitflds_.add( unitfld );
	if ( !alignobj )
	    alignobj = unitfld->attachObj();
    }

    selectunitsfld_ = new uiCheckBox( formresgrp,
			tr("Specify formula internal units"),
			mCB(this,uiMathFormula,chooseUnitsCB) );
    selectunitsfld_->attach( alignedBelow, alignobj );

    formresgrp->setHAlignObj( alignobj );
    formresgrp->attach( alignedBelow, inpflds_.last()->attachObj() );
}


void uiMathFormula::initGrp( CallBacker* )
{
    if ( recbut_ )
	recbut_->display( false );

    if ( formlbl_ )
	formlbl_->display( false );
    for ( auto* inpflds : inpflds_ )
	inpflds->use( nullptr ); //hide
    if ( setup_.withunits_ )
    {
	setFormMnemonic( Mnemonic::undef(), true );
	selectunitsfld_->display( false );
	selectunitsfld_->setChecked( false );
    }

    if ( form_.isOK() )
	formChangedCB( nullptr );

    mAttachCB( form_.allChanged, uiMathFormula::formChangedCB );
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
    return putToScreen();
}


const char* uiMathFormula::text() const
{
    return exprfld_->text();
}


bool uiMathFormula::hasFixedUnits() const
{
    return setup_.withunits_ && form_.hasFixedUnits();
}


bool uiMathFormula::updateForm() const
{
    form_.setText( exprfld_->text() );
    if ( !checkValidNrInputs() )
	return false;

    for ( int idx=0; idx<nrInputs(); idx++ )
	inpflds_[idx]->fill( form_ );

    const uiUnitSel* unitfld = getUnitSelFld();
    if ( unitfld )
    {
	const uiMnemonicsSel* mnselfld = getMnSelFld();
	const Mnemonic* mn = mnselfld ? mnselfld->mnemonic() : setup_.mn_;
	if ( !mn || mn->isUdf() )
	{
	    if ( !uiMSG().askGoOn( tr("Formula return type is undefined.\n"
				      "Is this correct?" ) ) )
		return false;
	}

	form_.setOutputMnemonic( mn );
	form_.setOutputFormUnit( unitfld->getUnit() );
    }

    const int nrrec = form_.maxRecShift();
    for ( int idx=0; idx<nrrec; idx++ )
	form_.recStartVals()[idx] = idx >= recvals_.size() ? 0 : recvals_[idx];

    return true;
}


bool uiMathFormula::useForm( const TypeSet<Mnemonic::StdType>* inputtypes )
{
    return putToScreen();
}


namespace OD
{

static void displayOptional( uiObject& fld, bool yn )
{
#ifdef __debug__
    fld.setSensitive( yn );
#else
    fld.display( yn );
#endif
}

} // namespace OD


bool uiMathFormula::putToScreen()
{
    if ( form_.isBad() )
    {
	uiMSG().error(tr("Invalid expression:\n%1").arg(form_.errMsg()));
	return false;
    }

    if ( !checkValidNrInputs() )
	return false;

    exprfld_->setText( form_.text() );
    if ( recbut_ )
	recbut_->display( form_.isRecursive() );

    if ( setup_.withunits_ && !hasFixedUnits() )
    {
	guessInputFormDefs();
	guessOutputFormDefs();
    }

    const bool hasfixedunits = hasFixedUnits();
    if ( setup_.withunits_ )
    {
	if ( formlbl_ )
	{
	    formlbl_->display( nrInputs() > 0 );
	    OD::displayOptional( *formlbl_, !hasfixedunits );
	}
    }

    fullformupdate_ = true;
    for ( auto* inpfld : inpflds_ )
	inpfld->use( form_, hasfixedunits );
    fullformupdate_ = false;
    if ( setup_.withunits_ )
    {
	setOutputDefsFromForm( hasfixedunits );
	selectunitsfld_->display( hasfixedunits );
	selectunitsfld_->setChecked( !hasfixedunits );
    }

    return true;
}


void uiMathFormula::guessInputFormDefs()
{
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
	else if ( form_.inputMnemonic(idx) && form_.inputFormUnit(idx) )
	    continue;

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
	    {
		float val; int maxidx = -1; float maxval = -1.f;
		TypeSet<OD::Pair<float,BufferString> > mnemmatches;
		for ( const auto* inpnm : inpnms )
		{
		    const char* inpmstr = inpnm->str();
		    if ( !mn->matches(inpmstr,true,&val) )
			continue;

		    mnemmatches +=
				OD::Pair<float,BufferString>( val, inpmstr );
		    if ( val > maxval )
		    {
			maxval = val;
			maxidx = mnemmatches.size() - 1;
		    }
		}

		if ( mnemmatches.validIdx(maxidx) )
		{
		    inpdef.set( mnemmatches[maxidx].second() );
		    found = true;
		}
	    }
	}
	else
	{
	    mn = mnsel_->getByName( inpdef );
	    inpnms = inpfld->getInputNms( mn );
	    if ( mn && !mn->isUdf() )
	    {
		if ( !inpnms.isPresent(inpdef.str()) )
		{
		    float val; int maxidx = -1; float maxval = -1.f;
		    TypeSet<OD::Pair<float,BufferString> > mnemmatches;
		    for ( const auto* inpnm : inpnms )
		    {
			const char* inpmstr = inpnm->str();
			if ( !mn->matches(inpmstr,true,&val) )
			    continue;

			mnemmatches +=
				   OD::Pair<float,BufferString>( val, inpmstr );
			if ( val > maxval )
			{
			    maxval = val;
			    maxidx = mnemmatches.size() - 1;
			}
		    }

		    if ( mnemmatches.validIdx(maxidx) )
		    {
			inpdef.set( mnemmatches[maxidx].second() );
			found = true;
		    }
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
	if ( mn && !mn->isUdf() )
	    form_.setInputFormUnit( idx, nullptr );
    }
}


bool uiMathFormula::guessFormula( Math::Formula& form )
{
    if ( !form.isOK() )
	return false;

    const int nrinps = form.nrInputs();
    const int nrvals = form.nrValues2Provide();
    const int nrconsts = form.nrConsts();
    const int nrspecs = form.nrSpecs();
    BufferString formexpr( form.text() );
    formexpr.remove( ' ' );

    const RockPhysics::FormulaSet& rpforms = ROCKPHYSFORMS();
    for ( const auto* rpform : rpforms )
    {
	if ( rpform->nrInputs() != nrinps ||
	     rpform->nrValues2Provide() != nrvals ||
	     rpform->nrConsts() != nrconsts || rpform->nrSpecs() != nrspecs )
	    continue;

	BufferString rpexpr = rpform->text();
	rpexpr.remove( ' ' );
	if ( rpexpr.isEqual(formexpr,OD::CaseInsensitive) )
	{
	    form.copyFrom( *rpform );
	    return true;
	}

	for ( int iinp=0; iinp<nrinps; iinp++ )
	{
	    if ( rpform->isConst(iinp) || rpform->isSpec(iinp) )
		continue;

	    rpexpr.replace( rpform->inputVar(iinp), rpform->inputDef(iinp) );
	}

	if ( rpexpr.isEqual(formexpr,OD::CaseInsensitive) )
	{
	    form.copyFrom( *rpform );
	    return true;
	}
    }

    return false;
}


void uiMathFormula::guessOutputFormDefs()
{
    TypeSet<int> varidxs;
    for ( int idx=0; idx<form_.nrInputs(); idx++ )
	if ( !form_.isConst(idx) && !form_.isSpec(idx) )
	    varidxs += idx;

    if ( varidxs.isEmpty() )
	return;

    const Mnemonic* firstinpmn = form_.inputMnemonic( varidxs.first() );
    if ( !firstinpmn || firstinpmn->isUdf() )
	return;

    const UnitOfMeasure* firstuom = form_.inputFormUnit( varidxs.first() );
    if ( varidxs.size() == 1 )
    {
	form_.setOutputMnemonic( firstinpmn );
	form_.setOutputFormUnit( firstuom );
	return;
    }

    for ( int idx=1; idx<varidxs.size(); idx++ )
    {
	const Mnemonic* othermn = form_.inputMnemonic( varidxs[idx] );
	if ( !othermn || othermn->isUdf() || othermn != firstinpmn )
	    return;
    }

    const Math::ExpressionParser mp( exprfld_->text(), false );
    PtrMan<Math::Expression> mathexpr = mp.parse();
    if ( !mathexpr )
	return;

    const BufferString exprtyp( mathexpr->type() );
    if ( exprtyp == "ExpressionPlus" || exprtyp == "ExpressionMinus" )
    {
	form_.setOutputMnemonic( firstinpmn );
	form_.setOutputFormUnit( firstuom );
    }
}


bool uiMathFormula::setOutputDefsFromForm( bool hasfixedunits )
{
    const Mnemonic* mn = form_.outputMnemonic();
    const UnitOfMeasure* formun = form_.outputFormUnit();
    const bool dodisp = !hasfixedunits;
    OD::displayOptional( *formreslbl_, dodisp );
    if ( typfld_ )
    {
	if ( !mn )
	    mn = &Mnemonic::undef();

	setFormMnemonic( *mn, dodisp );
    }

    uiUnitSel* unitselfld = getUnitSelFld();
    if ( unitselfld )
    {
	OD::displayOptional( *unitselfld, dodisp );
	unitselfld->setUnit( formun );
    }

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


void uiMathFormula::formChangedCB( CallBacker* )
{
    putToScreen();
}


void uiMathFormula::formSetCB( CallBacker* )
{
    const StringView formtxt = form_.text();
    const BufferString newexpr( exprfld_->text() );
    const bool changed = newexpr != formtxt;
    if ( !changed )
	return;

    Math::Formula newform( false, newexpr.buf() );
    if ( guessFormula(newform) )
	form_ = newform;
    else
    {
	form_.setText( newexpr.buf() );
	form_.clearAllDefs();
    }

    formChangedCB( nullptr );
}


void uiMathFormula::inpSetCB( CallBacker* cb )
{
    int inpidx = -1;
    if ( !setNotifInpNr(cb,inpidx) )
	return;

    uiMathExpressionVariable* inpfld = inpflds_.get( inpidx );
    const bool isvariable = inpidx < nrInputs() &&
			    ( !isSpec(inpidx) || !isConst(inpidx) );
    bool mnemonicchanged = false;
    if ( isvariable )
    {
	const BufferString inpdef = getInput( inpidx );
	const Mnemonic* mn = form_.inputMnemonic( inpidx );
	if ( mnsel_ && (!mn || mn->isUdf()) )
	    mn = mnsel_->getByName( inpdef );

	mnemonicchanged = inpfld->getMnemonic() != mn;
	if ( mnemonicchanged )
	    inpfld->setFormType( mn ? *mn : Mnemonic::undef() );

	const UnitOfMeasure* uom = form_.inputFormUnit( inpidx );
	const bool dispuom = !mn || mn->isUdf() || !uom;
	inpfld->setFormUnit( uom, dispuom );
    }

    inpSet.trigger( inpfld );

    if ( isvariable && mnemonicchanged && !fullformupdate_ )
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


void uiMathFormula::formTypeSetCB( CallBacker* cb )
{
    const int curidx = typfld_->currentItem();
    const bool hasfixedunits = hasFixedUnits();
    const bool nooutputuom = cb && cb == typfld_ && !form_.outputFormUnit();
    for ( int idx=0; idx<mnselflds_.size(); idx++ )
    {
	uiMnemonicsSel* mnselfld = mnselflds_.get( idx );
	if ( mnselfld )
	    mnselfld->display( idx == curidx );
	if ( idx == curidx )
	{
	    if ( mnselfld )
	    {
		formMnSetCB( nullptr );
		if ( cb && cb == typfld_ && !hasfixedunits )
		    OD::displayOptional( *mnselfld, true );
	    }
	    else
	    {
		const Mnemonic::StdType curtyp = getOutputStdType();
		const MnemonicSelection mnsel( curtyp );
		if ( !mnsel.isEmpty() )
		    formMnSet.trigger( mnsel.first() );
	    }
	}
    }

    for ( int idx=0; idx<unitflds_.size(); idx++ )
    {
	uiUnitSel* unitselfld = unitflds_.get( idx );
	const bool dodisp = idx == curidx && unitselfld;
	if ( unitselfld )
	{
	    unitselfld->display( dodisp );
	    if ( dodisp )
	    {
		if ( nooutputuom )
		    unitselfld->setUnit( (const UnitOfMeasure*)nullptr );

		if ( cb && cb == typfld_ && !hasfixedunits )
		    OD::displayOptional( *unitselfld, true );
	    }
	}
    }
}


void uiMathFormula::formMnSetCB( CallBacker* cb )
{
    uiMnemonicsSel* mnselfld = getMnSelFld();
    if ( !mnselfld )
	return;

    if ( !cb || (cb && cb == mnselfld) )
	form_.setOutputMnemonic( mnselfld->mnemonic() );

    uiUnitSel* unitselfd = getUnitSelFld();
    const Mnemonic::StdType curtyp = getOutputStdType();
    const Mnemonic* mn = mnselfld->mnemonic();
    if ( curtyp == Mnemonic::Other && unitselfd )
    {
	if ( mn && !mn->isUdf() )
	    unitselfd->setPropType( mn->stdType() );
	else
	    unitselfd->setPropType( Mnemonic::Other );
    }

    formMnSet.trigger( mn );
}


void uiMathFormula::formUnitSetCB( CallBacker* cb )
{
    if ( !cb )
	return;

    uiUnitSel* unitfld = getUnitSelFld();
    if ( unitfld && cb == unitfld )
	form_.setOutputFormUnit( unitfld->getUnit() );
}


void uiMathFormula::chooseUnitsCB( CallBacker* )
{
    const bool showunitflds = selectunitsfld_->isChecked();
    if ( formlbl_ )
	OD::displayOptional( *formlbl_, showunitflds );
    if ( formreslbl_ )
	OD::displayOptional( *formreslbl_, showunitflds );

    const bool hasfixedunits = hasFixedUnits();
    int iinp = 0;
    for ( auto* fld : inpflds_ )
    {
	const bool dispinp = showunitflds && fld->isActive() && !fld->isConst()
				&& !fld->isSpec();
	const UnitOfMeasure* inpuom = fld->getUnit();
	if ( !inpuom && hasfixedunits && iinp < form_.nrInputs() )
	    inpuom = form_.inputFormUnit( iinp );

	fld->setFormUnit( inpuom, dispinp );
	iinp++;
    }

    if ( typfld_ )
	OD::displayOptional( *typfld_, showunitflds );

    uiMnemonicsSel* mnselfld = getMnSelFld();
    if ( mnselfld )
	OD::displayOptional( *mnselfld, showunitflds );

    uiUnitSel* unitfld = getUnitSelFld();
    if ( unitfld )
	OD::displayOptional( *unitfld, showunitflds );
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

bool acceptOK( CallBacker* ) override
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


Mnemonic::StdType uiMathFormula::getOutputStdType() const
{
    if ( !typfld_ )
	return Mnemonic::Other;

    const BufferString curtypstr( typfld_->text() );
    return Mnemonic::parseEnumStdType( curtypstr.buf() );
}


void uiMathFormula::setFormMnemonic( const Mnemonic& mn, bool dodisp )
{
    if ( !typfld_ || mnselflds_.isEmpty() )
	return;

    const Mnemonic::StdType typ = mn.stdType();
    const Mnemonic::StdType curtyp = getOutputStdType();
    if ( typ != curtyp )
    {
	typfld_->setCurrentItem( Mnemonic::toString(typ) );
	formTypeSetCB( nullptr );
    }

    OD::displayOptional( *typfld_, dodisp );

    uiMnemonicsSel* mnselfld = getMnSelFld();
    if ( mnselfld )
    {
	if ( mnselfld->mnemonic() != &mn )
	{
	    mnselfld->setMnemonic( mn );
	    formMnSetCB( nullptr );
	}

	OD::displayOptional( *mnselfld, dodisp );
    }

    uiUnitSel* unitselfld = getUnitSelFld();
    if ( unitselfld )
	formUnitSetCB( nullptr );
}


const uiMnemonicsSel* uiMathFormula::getMnSelFld() const
{
    return mSelf().getMnSelFld();
}


uiMnemonicsSel* uiMathFormula::getMnSelFld()
{
    if ( !typfld_ )
	return nullptr;

    const int curidx = typfld_->currentItem();
    return mnselflds_.validIdx( curidx ) ? mnselflds_.get( curidx ) : nullptr;
}


const uiUnitSel* uiMathFormula::getUnitSelFld() const
{
    return mSelf().getUnitSelFld();
}


uiUnitSel* uiMathFormula::getUnitSelFld()
{
    if ( !typfld_ )
	return unitflds_.isEmpty() ? nullptr : unitflds_.first();

    const int curidx = typfld_->currentItem();
    return unitflds_.validIdx( curidx ) ? unitflds_.get( curidx ) : nullptr;
}


BufferString uiMathFormula::getIOFileName( bool forread )
{
    IOObjContext ctxt = mIOObjContext( MathFormula );
    const bool wantio = !setup_.stortype_.isEmpty();
    if ( wantio )
	ctxt.requireType( setup_.stortype_ );

    BufferString fnm;
    ctxt.forread_ = forread;
    uiIOObjSelDlg dlg( this, ctxt );
    if ( !dlg.go() )
	return fnm;

    const IOObj* ioobj = dlg.ioObj();
    if ( !ioobj )
	return fnm;

    fnm = ioobj->fullUserExpr( forread );
    const bool doesexist = File::exists( fnm );
    if ( forread && !doesexist )
    {
	uiMSG().error(uiStrings::sFileDoesntExist());
	fnm.setEmpty();
    }

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
    Math::Formula form;
    form.usePar( iop );
    if ( !form.isOK() )
    {
	uiMSG().error( tr("Cannot restore the stored formula:\n%1")
			  .arg(form.text()) );
	return;
    }

    recvals_ = form.recStartVals();
    form_ = form;
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


bool uiMathFormula::useForm()
{
    return putToScreen();
}

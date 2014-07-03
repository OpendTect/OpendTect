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
#include "uibuttongroup.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"

#include "mathformula.h"
#include "mathspecvars.h"
#include "mathformulatransl.h"
#include "od_iostream.h"
#include "ascstream.h"
#include "file.h"
#include "filepath.h"


uiMathFormula::uiMathFormula( uiParent* p, Math::Formula& form,
				const uiMathFormula::Setup& su )
	: uiGroup(p,"Math Formula")
	, form_(form)
	, ctio_(*mMkCtxtIOObj(MathFormula))
	, setup_(su)
	, unitfld_(0)
	, recbut_(0)
	, formSet(this)
	, inpSet(this)
	, subInpSet(this)
	, formUnitSet(this)
	, notifinpnr_(-1)
{
    bool wantio = !setup_.stortype_.isEmpty();
    if ( wantio )
	ctio_.ctxt.toselect.require_.set( sKey::Type(), setup_.stortype_ );
    const CallBack formsetcb( mCB(this,uiMathFormula,formSetCB) );
    const CallBack inpsetcb( mCB(this,uiMathFormula,inpSetCB) );
    const CallBack subinpsetcb( mCB(this,uiMathFormula,subInpSetCB) );
    const CallBack unitsetcb( mCB(this,uiMathFormula,formUnitSetCB) );

    uiMathExpression::Setup mesu( setup_.label_ );
    mesu.withsetbut( true ).fnsbelow( false ).specvars( &form_.specVars() );
    exprfld_ = new uiMathExpression( this, mesu );
    exprfld_->formSet.notify( formsetcb );
    setHAlignObj( exprfld_ );

    for ( int idx=0; idx<setup_.maxnrinps_; idx++ )
    {
	uiMathExpressionVariable* fld = new uiMathExpressionVariable(this,idx,
				    setup_.withunits_,setup_.withsubinps_,
				    &form_.specVars());
	if ( idx )
	    fld->attach( alignedBelow, inpflds_[idx-1] );
	else
	    fld->attach( alignedBelow, exprfld_ );
	fld->inpSel.notify( inpsetcb );
	fld->subInpSel.notify( subinpsetcb );
	inpflds_ += fld;
    }

    if ( setup_.withunits_ )
    {
	const bool haveproptype = setup_.proptype_ != PropertyRef::Other;
	uiUnitSel::Setup uussu( PropertyRef::Other,
		haveproptype ? "Formula output unit" : "Formula result is" );
	uussu.selproptype( !haveproptype ).withnone( true );
	unitfld_ = new uiUnitSel( this, uussu );
	unitfld_->attach( alignedBelow,
				inpflds_[inpflds_.size()-1]->attachObj() );
	unitfld_->selChange.notify( unitsetcb );
	if ( haveproptype )
	    unitfld_->setPropType( setup_.proptype_ );
	else
	    unitfld_->propSelChange.notify( unitsetcb );
    }

    uiButtonGroup* bgrp = 0;
    if ( wantio || form_.inputsAreSeries() )
	bgrp = new uiButtonGroup( this, "tool buts", OD::Horizontal );

    if ( form_.inputsAreSeries() )
	recbut_ = new uiToolButton( bgrp, "recursion",
				    "Set start values for recursion",
				    mCB(this,uiMathFormula,recButPush) );
    if ( wantio )
    {
	new uiToolButton( bgrp, "open", "Open stored formula",
				    mCB(this,uiMathFormula,readReq) );
	new uiToolButton( bgrp, "save", "Save this formula",
				    mCB(this,uiMathFormula,writeReq) );
    }

    if ( bgrp )
    {
	if ( unitfld_ )
	    bgrp->attach( rightTo, unitfld_ );
	else
	    bgrp->attach( rightTo, exprfld_ );
	bgrp->attach( rightBorder );
    }

    postFinalise().notify( mCB(this,uiMathFormula,initFlds) );
}


uiMathFormula::~uiMathFormula()
{
    delete ctio_.ioobj;
    delete &ctio_;
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
	BufferString msg( "Sorry, the expression contains ", form_.nrInputs(),
			  " inputs.\nThe maximum number is " );
	msg.add( inpflds_.size() ).add( "." );
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
	    const int specidx = form_.specIdx( idx );
	    PropertyRef::StdType ptyp = PropertyRef::Other;
	    if ( specidx < 0 && inputtypes && inputtypes->validIdx(idx) )
		ptyp = (*inputtypes)[idx];
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


void uiMathFormula::initFlds( CallBacker* )
{
    useForm();
    if ( form_.isOK() )
	formSet.trigger();
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
    if ( setNotifInpNr( cb ) )
	inpSet.trigger();
}


void uiMathFormula::subInpSetCB( CallBacker* cb )
{
    if ( setNotifInpNr( cb ) )
	subInpSet.trigger();
}


bool uiMathFormula::setNotifInpNr( const CallBacker* cb )
{
    notifinpnr_ = -1;
    for ( int idx=0; idx<inpflds_.size(); idx++ )
    {
	if ( inpflds_[idx] == cb )
	    { notifinpnr_ = idx; break; }
    }
    if ( notifinpnr_ < 0 )
	{ pErrMsg("Huh" ); return false; }
    return true;
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


BufferString uiMathFormula::getIOFileName( bool forread )
{
    BufferString fnm;
    ctio_.ctxt.forread = forread;
    uiIOObjSelDlg dlg( this, ctio_ );
    if ( !dlg.go() )
	return fnm;

    fnm = dlg.ioObj()->fullUserExpr( forread );
    const bool doesexist = File::exists( fnm );
    if ( forread && !doesexist )
	{ uiMSG().error("File does not exist"); fnm.setEmpty(); }

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
	{ uiMSG().error("Cannot open input file"); return; }

    ascistream astrm( strm, true );
    if ( !astrm.isOfFileType(Math::Formula::sKeyFileType()) )
	{ uiMSG().error("Input file is not of 'Math Formula' type"); return; }
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
	{ uiMSG().error("Cannot open output file"); return; }
    ascostream astrm( strm );
    if ( !astrm.putHeader(Math::Formula::sKeyFileType()) )
	{ uiMSG().error("Cannot write file header"); return; }

    IOPar iop; form_.fillPar( iop );
    iop.putTo( astrm );
}

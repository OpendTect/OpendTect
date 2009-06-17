/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogcalc.cc,v 1.2 2009-06-17 15:07:39 cvsbert Exp $";


#include "uiwelllogcalc.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uitable.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uiseparator.h"

#include "welllogset.h"
#include "welllog.h"
#include "separstr.h"
#include "survinfo.h"
#include "mathexpression.h"

static const int cMaxNrInps = 6;


uiWellLogCalc::uiWellLogCalc( uiParent* p, Well::LogSet& ls )
	: uiDialog(p,uiDialog::Setup("Calculate new logs",
				     "Specify inputs and outputs for new log",
				     mTODOHelpID))
    	, wls_(ls)
    	, formfld_(0)
    	, nrvars_(0)
    	, expr_(0)
    	, havenew_(false)
{
    setCtrlStyle( DoAndStay );

    BufferStringSet lognms;
    for ( int idx=0; idx<wls_.size(); idx++ )
    {
	const BufferString& nm( ls.getLog(idx).name() );
	if ( !nm.isEmpty() )
	    lognms.addIfNew( nm );
    }

    const CallBack formsetcb( mCB(this,uiWellLogCalc,formSet) );
    const CallBack inpsetcb( mCB(this,uiWellLogCalc,inpSel) );

    uiGroup* inpgrp = new uiGroup( this, "inp grp" );
    formfld_ = new uiGenInput( inpgrp, "Formula (like 'x0 + x1')",
				     StringInpSpec().setName("Formula") );
    formfld_->valuechanged.notify( formsetcb );
    UserInputObj* uiobj = formfld_->element(0);
    mDynamicCastGet(uiLineEdit*,le,uiobj)
    if ( le ) le->returnPressed.notify( formsetcb );
    uiButton* setbut = new uiPushButton( inpgrp, "&Set", formsetcb, true );
    setbut->attach( rightOf, formfld_ );
    inpgrp->setHAlignObj( formfld_ );

    const int maxnrinps = wls_.size() < cMaxNrInps ? wls_.size() : cMaxNrInps;
    for ( int idx=0; idx<maxnrinps; idx++ )
    {
	const BufferString fldtxt( "For x", idx, " use" );
	uiLabeledComboBox* lcb = new uiLabeledComboBox( inpgrp, lognms, fldtxt,
					BufferString("input ",idx) );
	int selidx = idx; if ( selidx >= wls_.size() ) selidx = wls_.size()-1;
	uiComboBox* cb = lcb->box();
	cb->setCurrentItem( selidx );
	cb->selectionChanged.notify( inpsetcb );
	if ( idx )
	    lcb->attach( alignedBelow, varselflds_[idx-1] );
	else
	    lcb->attach( alignedBelow, formfld_ );
	varselflds_ += lcb;
    }

    uiSeparator* sep = new uiSeparator( this, "sep" );
    sep->attach( stretchedBelow, inpgrp );

    dahrgfld_ = new uiGenInput( this, "Output MD range",
	    			  FloatInpIntervalSpec(true) );
    dahrgfld_->attach( alignedBelow, inpgrp );
    dahrgfld_->attach( ensureBelow, sep );
    ftbox_ = new uiCheckBox( this, "Feet" );
    ftbox_->setChecked( SI().zInFeet() );
    ftbox_->activated.notify( mCB(this,uiWellLogCalc,feetSel) );
    ftbox_->attach( rightOf, dahrgfld_ );

    nmfld_ = new uiGenInput( this, "Name for new log" );
    nmfld_->attach( alignedBelow, dahrgfld_ );

    finaliseDone.notify( formsetcb );
}


uiWellLogCalc::~uiWellLogCalc()
{
    delete expr_;
}


void uiWellLogCalc::dispVarInps( int nr )
{
    for ( int idx=0; idx<varselflds_.size(); idx++ )
	varselflds_[idx]->display( idx < nr );
}


void uiWellLogCalc::getMathExpr()
{
    delete expr_; expr_ = 0;
    const BufferString inp( formfld_->text() );
    if ( inp.isEmpty() ) return;
    expr_ = MathExpression::parse( inp );
    if ( !expr_ )
	uiMSG().warning( "The provided expression cannot be used."
	    	       "\nPlease enter an expression with variables like:\n"
		       "2.345 * x0 + x1 / x2" );
}


void uiWellLogCalc::feetSel( CallBacker* )
{
    dahrg_ = dahrgfld_->getFStepInterval();
    const float fac = ftbox_->isChecked() ? mToFeetFactor : mFromFeetFactor;
    if ( !mIsUdf(dahrg_.start) ) dahrg_.start *= fac;
    if ( !mIsUdf(dahrg_.stop) ) dahrg_.stop *= fac;
    if ( !mIsUdf(dahrg_.step) ) dahrg_.step *= fac;
    dahrgfld_->setValue( dahrg_ );
}


void uiWellLogCalc::formSet( CallBacker* )
{
    getMathExpr();
    nrvars_ = expr_ ? expr_->getNrDiffVariables() : 0;
    if ( nrvars_ > varselflds_.size() )
	uiMSG().warning(
	    "Found more variables than the maximum number of input logs." );

    dispVarInps( nrvars_ );
    inpSel( 0 );
}


void uiWellLogCalc::inpSel( CallBacker* )
{
    if ( nrvars_ < 1 ) return;

    StepInterval<float> dahrg; int actualnr = 0;
    for ( int idx=0; idx<nrvars_; idx++ )
    {
	if ( idx >= varselflds_.size() ) break;

	const Well::Log* wl = wls_.getLog( varselflds_[idx]->box()->text() );
	if ( !wl ) { pErrMsg("Huh"); continue; }
	if ( wl->isEmpty() ) continue;

	StepInterval<float> curdahrg( wl->dah( 0 ), wl->dah( wl->size()-1 ),
				      wl->dahStep( false ) );
	if ( actualnr == 0 )
	    dahrg = curdahrg;
	else
	    { dahrg.include( curdahrg, false ); dahrg.step += curdahrg.step; }
	actualnr++;
    }
    if ( actualnr < 1 ) return;

    dahrg.step /= actualnr;
    dahrgfld_->setValue( dahrg );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiWellLogCalc::acceptOK( CallBacker* )
{
    getMathExpr();
    if ( !expr_ ) return false;
    nrvars_ = expr_->getNrDiffVariables();

    const BufferString newnm = nmfld_->text();
    if ( newnm.isEmpty() )
	mErrRet("Please provide a name for the new log")
    if ( wls_.getLog(newnm) )
	mErrRet("A log with this name already exists."
		"\nPlease enter a different name for the new log")

    dahrg_ = dahrgfld_->getFStepInterval();
    if ( mIsUdf(dahrg_.start) || mIsUdf(dahrg_.stop) || mIsUdf(dahrg_.step) )
	mErrRet("Please provide the MD range and step for the output log")
    if ( ftbox_->isChecked() )
    	dahrg_.scale( mFromFeetFactor );

    ObjectSet<const Well::Log> inps; inps.allowNull( true );
    TypeSet<int> shifts;
    if ( !getInpsAndShifts(inps,shifts) || !getRecInfo() )
	return false;

    Well::Log* newwl = new Well::Log( newnm );
    if ( !calcLog(*newwl,inps,shifts) )
	{ delete newwl; return false; }

    wls_.add( newwl );
    uiMSG().message( "Successfully added this log" );
    havenew_ = true;
    return false;
}


bool uiWellLogCalc::getInpsAndShifts( ObjectSet<const Well::Log>& inps,
				      TypeSet<int>& shifts )
{
    BufferString pfx; int shift;
    recvars_.erase(); startvals_.erase();
    for ( int iexpr=0; iexpr<expr_->getNrVariables(); iexpr++ )
    {
	const MathExpression::VarType typ = expr_->getType( iexpr );
	const BufferString varnm( expr_->getVariableStr(iexpr) );
	if ( typ == MathExpression::Constant )
	    mErrRet(BufferString("Please insert the actual value rather than: '"
				 ,varnm,"'"))
	expr_->getPrefixAndShift( varnm, pfx, shift );
	const Well::Log* wl = 0;
	if ( typ == MathExpression::Recursive )
	{
	    if ( shift == 0 )
		mErrRet("Recursive 'THIS' variables must have a shift.\n"
			"For example, specify 'THIS[-2]' for 2 samples shift")
	    if ( shift > 0 ) shift = -shift;
	    recvars_ += iexpr;
	}
	else
	{
	    const int varidx = expr_->getUsrVarIdx( iexpr );
	    if ( varidx >= varselflds_.size() )
		mErrRet(BufferString("The variable number is too high: '"
				     ,varnm,"'"))

	    wl = wls_.getLog( varselflds_[varidx]->box()->text() );
	    if ( !wl ) { pErrMsg("Huh"); return false; }
	    if ( wl->isEmpty() )
		mErrRet(BufferString("Empty well log: '",wl->name(),"'"))
	}

	inps += wl;
	shifts += shift;
    }

    return true;
}


bool uiWellLogCalc::getRecInfo()
{
    float startval = 0;
    const int nrrec = recvars_.size();
    if ( nrrec < 1 ) return true;

    const char* wintitl = nrrec > 1 ? "Specify values" : "Specify value";
    uiDialog dlg( this, uiDialog::Setup(wintitl,mNoDlgTitle,mNoHelpID) );
    uiLabel* lbl = new uiLabel( &dlg,
	    "Recursive calculation: Please enter starting value(s)" );
    uiGenInput* fld = new uiGenInput( &dlg,
				     "Start values (comma separated)" );
    fld->attach( centeredBelow, lbl );
    lbl = new uiLabel( &dlg,
	    "These will be the first THIS[0], THIS[-1], ..." );
    lbl->attach( centeredBelow, fld );
    if ( !dlg.go() )
	return false;

    const SeparString usrinp( fld->text() );
    const int nrvals = usrinp.size();
    for ( int idx=0; idx<nrvals; idx++ )
    {
	float val = atof( usrinp[idx] );
	if ( mIsUdf(val) )
	    break;
	startvals_ += val;
    }

    if ( startvals_.isEmpty() )
	startvals_ += 0;
    return true;
}


bool uiWellLogCalc::calcLog( Well::Log& wlout, ObjectSet<const Well::Log>& inps,
			     TypeSet<int>& shifts )
{
    TypeSet<float> vals; int rgidx = 0;
    int nrstart = startvals_.size();
    if ( nrstart > 0 )
	{ vals = startvals_; rgidx = 1; }
    if ( nrstart > 0 ) nrstart--;

    dahrg_.sort();
    const int endrgidx = dahrg_.nrSteps();
    for ( ; rgidx<=endrgidx; rgidx++ )
    {
	float outval = 0;
	const float dah = dahrg_.atIndex( rgidx );
	for ( int iinp=0; iinp<inps.size(); iinp++ )
	{
	    const Well::Log* wl = inps[iinp];
	    if ( wl )
	    {
		float curdah = dah + dahrg_.step * shifts[iinp];
		const float val = wl->getValue( curdah );
		if ( mIsUdf(val) )
		    { outval = val; break; }
		else
		    expr_->setVariable( iinp, val );
	    }
	    else
	    {
		const int valsidx = rgidx + nrstart + shifts[iinp];
		if ( valsidx < 0 || valsidx >= vals.size() )
		    { outval = mUdf(float); break; }
		expr_->setVariable( iinp, vals[valsidx] );
	    }
	}

	if ( !mIsUdf(outval) )
	    outval = expr_->getValue();
	vals += outval;
    }

    for ( int idx=nrstart; idx<vals.size(); idx++ )
    {
	const float dah = dahrg_.atIndex( idx - nrstart );
	wlout.addValue( dah, vals[idx] );
    }

    return true;
}

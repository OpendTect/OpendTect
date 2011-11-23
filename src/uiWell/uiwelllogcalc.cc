/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogcalc.cc,v 1.14 2011-11-23 11:35:56 cvsbert Exp $";


#include "uiwelllogcalc.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uimathexpression.h"
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
#include "unitofmeasure.h"

static const int cMaxNrInps = 6;
static const char* specvararr[] = { "MD", "DZ", 0 };
static const BufferStringSet specvars( specvararr );

class uiWellLogCalcInpData : public uiGroup
{
public:

uiWellLogCalcInpData( uiWellLogCalc* p, uiGroup* inpgrp,
		      const BufferStringSet& lognms, int fieldnr )
    : uiGroup(inpgrp,"Inp data group")
    , wls_(p->wls_)
    , idx_(fieldnr)
    , lognms_(lognms)
    , lognmsettodef_(false)
{
    varmfld_ = new uiGenInput( this, "For" );
    varmfld_->setElemSzPol( uiObject::Small );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, lognms_, "use",
				    BufferString("input ",fieldnr) );
    inpfld_ = lcb->box();
    int selidx = fieldnr; if ( selidx >= wls_.size() ) selidx = wls_.size()-1;
    inpfld_->setCurrentItem( selidx );
    inpfld_->selectionChanged.notify( mCB(p,uiWellLogCalc,inpSel) );
    lcb->attach( rightOf, varmfld_ );

    udfbox_ = new uiCheckBox( this, "Fill empty sections" );
    udfbox_->attach( rightOf, lcb );

    setHAlignObj( lcb );
}

void use( MathExpression* expr )
{
    const int nrvars = expr ? expr->nrUniqueVarNames() : 0;
    if ( idx_ >= nrvars )
	{ display( false ); return; }
    const BufferString varnm = expr->uniqueVarName( idx_ );
    if ( specvars.indexOf(varnm.buf()) >= 0 )
	{ display( false ); return; }

    display( true );
    varmfld_->setText( varnm );
    if ( !lognmsettodef_ )
    {
	const int nearidx = lognms_.nearestMatch( varnm );
	if ( nearidx >= 0 )
	{
	    inpfld_->setCurrentItem( nearidx );
	    lognmsettodef_ = true;
	}
    }
}

bool hasVarName( const char* nm )
{
    BufferString selnm( varmfld_->text() );
    return selnm == nm;
}

const Well::Log* getLog()
{
    return wls_.getLog( inpfld_->text() );
}

bool getInp( uiWellLogCalc::InpData& inpdata )
{
    inpdata.noudf_ = udfbox_->isChecked();
    inpdata.wl_ = getLog();
    return inpdata.wl_;
}

    uiGenInput*		varmfld_;
    uiComboBox*		inpfld_;
    uiCheckBox*		udfbox_;
    const Well::LogSet&	wls_;
    const int		idx_;
    BufferStringSet	lognms_;
    bool		lognmsettodef_;

};


uiWellLogCalc::uiWellLogCalc( uiParent* p, Well::LogSet& ls )
	: uiDialog(p,uiDialog::Setup("Calculate new logs",
				     "Specify inputs and outputs for new log",
				     "107.1.10"))
    	, wls_(ls)
    	, formfld_(0)
    	, nrvars_(0)
    	, expr_(0)
    	, havenew_(false)
{
    setCtrlStyle( DoAndStay );
    const CallBack formsetcb( mCB(this,uiWellLogCalc,formSet) );

    uiGroup* inpgrp = new uiGroup( this, "inp grp" );
    formfld_ = new uiMathExpression( inpgrp );
    uiLabel* lbl = new uiLabel( inpgrp, "Formula (like 'den / son')" );
    formfld_->attach( rightOf, lbl );
    formfld_->formSet.notify( formsetcb );
    uiButton* setbut = new uiPushButton( inpgrp, "&Set", formsetcb, true );
    setbut->attach( rightOf, formfld_ );
    inpgrp->setHAlignObj( formfld_ );

    BufferStringSet lognms;
    for ( int idx=0; idx<wls_.size(); idx++ )
    {
	const BufferString& nm( ls.getLog(idx).name() );
	if ( !nm.isEmpty() )
	    lognms.addIfNew( nm );
    }

    const int maxnrinps = wls_.size() < cMaxNrInps ? wls_.size() : cMaxNrInps;
    for ( int idx=0; idx<maxnrinps; idx++ )
    {
	uiWellLogCalcInpData* fld = new uiWellLogCalcInpData( this, inpgrp,
							      lognms, idx );
	if ( idx )
	    fld->attach( alignedBelow, inpdataflds_[idx-1] );
	else
	    fld->attach( alignedBelow, formfld_ );
	inpdataflds_ += fld;
    }

    uiSeparator* sep = new uiSeparator( this, "sep" );
    sep->attach( stretchedBelow, inpgrp );

    dahrgfld_ = new uiGenInput( this, "Output MD range",
	    			  FloatInpIntervalSpec(true) );
    dahrgfld_->attach( alignedBelow, inpgrp );
    dahrgfld_->attach( ensureBelow, sep );
    ftbox_ = new uiCheckBox( this, "Feet" );
    ftbox_->setChecked( SI().depthsInFeetByDefault() );
    ftbox_->activated.notify( mCB(this,uiWellLogCalc,feetSel) );
    ftbox_->attach( rightOf, dahrgfld_ );

    nmfld_ = new uiGenInput( this, "Name for new log" );
    nmfld_->attach( alignedBelow, dahrgfld_ );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
	    					"Output unit of measure" );
    unfld_ = lcb->box();
    const ObjectSet<const UnitOfMeasure>& uns( UoMR().all() );
    unfld_->addItem( "-" );
    for ( int idx=0; idx<uns.size(); idx++ )
	unfld_->addItem( uns[idx]->name() );
    lcb->attach( alignedBelow, nmfld_ );

    postFinalise().notify( formsetcb );
}


uiWellLogCalc::~uiWellLogCalc()
{
    delete expr_;
}


void uiWellLogCalc::getMathExpr()
{
    delete expr_; expr_ = 0;
    const BufferString inp( formfld_->text() );
    if ( inp.isEmpty() ) return;

    MathExpressionParser mep( inp );
    expr_ = mep.parse();

    if ( !expr_ )
	uiMSG().warning(
	BufferString("The provided expression cannot be used:\n",mep.errMsg()));
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
    const int totnrvars = expr_ ? expr_->nrVariables() : 0;
    nrvars_ = 0; bool haveconst = false;
    for ( int idx=0; idx<totnrvars; idx++ )
    {
	if ( expr_->getType(idx) == MathExpression::Constant )
	{
	    haveconst = true;
	    uiMSG().error(
		    BufferString("Constants not supported here:\n'",
			expr_->fullVariableExpression(idx),
			"'.\nPlease insert the value itself.") );
	    break;
	}
    }

    MathExpression* useexpr = haveconst ? 0 : expr_;
    nrvars_ = useexpr ? useexpr->nrUniqueVarNames() : 0;
    for ( int idx=0; idx<inpdataflds_.size(); idx++ )
	inpdataflds_[idx]->use( useexpr );

    inpSel( 0 );
}


void uiWellLogCalc::inpSel( CallBacker* )
{
    if ( nrvars_ < 1 ) return;

    StepInterval<float> dahrg; int actualnr = 0;
    for ( int idx=0; idx<nrvars_; idx++ )
    {
	if ( idx >= inpdataflds_.size() ) break;

	const Well::Log* wl = inpdataflds_[idx]->getLog();
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
    if ( ftbox_->isChecked() )
	dahrg.scale( mToFeetFactor );
    dahrgfld_->setValue( dahrg );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiWellLogCalc::acceptOK( CallBacker* )
{
    getMathExpr();
    if ( !expr_ ) return false;
    nrvars_ = expr_->nrUniqueVarNames();

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

    TypeSet<InpData> inpdata;
    if ( !getInpData(inpdata) || !getRecInfo() )
	return false;

    Well::Log* newwl = new Well::Log( newnm );
    if ( !calcLog(*newwl,inpdata) )
	{ delete newwl; return false; }
    const int unselidx = unfld_->currentItem();
    if ( unselidx > 0 )
	newwl->setUnitMeasLabel( unfld_->text() );

    wls_.add( newwl );
    uiMSG().message( "Successfully added this log" );
    havenew_ = true;
    return false;
}


bool uiWellLogCalc::getInpData( TypeSet<uiWellLogCalc::InpData>& inpdata )
{
    BufferString pfx;
    recvaridxs_.erase(); startvals_.erase();
    for ( int iexpr=0; iexpr<expr_->nrVariables(); iexpr++ )
    {
	InpData inpd;
	const MathExpression::VarType typ = expr_->getType( iexpr );
	const BufferString fullvarexpr( expr_->fullVariableExpression(iexpr) );
	const BufferString varnm = MathExpressionParser::varNameOf( fullvarexpr,
							      &inpd.shift_ );

	switch ( typ )
	{
	case MathExpression::Constant:
	    mErrRet(BufferString("Please insert the actual value rather than: '"
				 ,varnm,"'"))

	case MathExpression::Recursive:
	{
	    if ( inpd.shift_ == 0 )
	    {
		BufferString msg( "Problem with ", fullvarexpr );
		mErrRet(":\nRecursive 'out' variables must have a shift.\n"
			"For example, specify 'out[-2]' for 2 samples shift")
	    }
	    if ( inpd.shift_ > 0 ) inpd.shift_ = -inpd.shift_;
	    recvaridxs_ += iexpr;
	} break;

	case MathExpression::Variable:
	{
	    inpd.specidx_ = specvars.indexOf( varnm.buf() );
	    if ( inpd.specidx_ < 0 )
	    {
		uiWellLogCalcInpData* inpfld = 0;
		for ( int ivar=0; ivar<nrvars_; ivar++ )
		{
		    if ( inpdataflds_[ivar]->hasVarName(varnm) )
			{ inpfld = inpdataflds_[ivar]; break; }
		}
		if ( !inpfld || !inpfld->getInp(inpd) )
		    mErrRet("Internal: Can't find log")
		if ( inpd.wl_->isEmpty() )
		    mErrRet(BufferString("Empty well log: '",
					 inpd.wl_->name(),"'"))
	    }
	} break;
	}

	inpdata += inpd;
    }

    return true;
}


bool uiWellLogCalc::getRecInfo()
{
    float startval = 0;
    const int nrrec = recvaridxs_.size();
    if ( nrrec < 1 ) return true;

    const char* wintitl = nrrec > 1 ? "Specify values" : "Specify value";
    uiDialog dlg( this, uiDialog::Setup(wintitl,mNoDlgTitle,mNoHelpID) );
    uiLabel* lbl = new uiLabel( &dlg,
	    "Recursive calculation: Please enter starting value(s)" );
    uiGenInput* fld = new uiGenInput( &dlg,
				     "Start values (comma separated)" );
    fld->attach( centeredBelow, lbl );
    lbl = new uiLabel( &dlg, "This will provide the first 'out' value(s)" );
    lbl->attach( centeredBelow, fld );
    if ( !dlg.go() )
	return false;

    const SeparString usrinp( fld->text() );
    const int nrvals = usrinp.size();
    for ( int idx=0; idx<nrvals; idx++ )
    {
	float val = toFloat( usrinp[idx] );
	if ( mIsUdf(val) )
	    break;
	startvals_ += val;
    }

    if ( startvals_.isEmpty() )
	startvals_ += 0;
    return true;
}


bool uiWellLogCalc::calcLog( Well::Log& wlout,
			     const TypeSet<uiWellLogCalc::InpData>& inpdata )
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
	const float dah = dahrg_.atIndex( rgidx );
	for ( int iinp=0; iinp<inpdata.size(); iinp++ )
	{
	    const uiWellLogCalc::InpData& inpd = inpdata[iinp];
	    const float curdah = dah + dahrg_.step * inpd.shift_;
	    if ( inpd.wl_ )
	    {
		const float val = inpd.wl_->getValue( curdah, inpd.noudf_ );
		expr_->setVariableValue( iinp, val );
	    }
	    else if ( inpd.specidx_ < 0 )
	    {
		const int valsidx = rgidx + nrstart + inpd.shift_;
		float varval = valsidx < 0 || valsidx >= vals.size()
		    	     ? mUdf(float) : vals[valsidx];
		expr_->setVariableValue( iinp, varval );
	    }
	    else
	    {
		float val = mUdf(float);
		if ( inpd.specidx_ == 0 )	val = curdah;
		else if ( inpd.specidx_ == 1 )	val = dahrg_.step;
		expr_->setVariableValue( iinp, val );
	    }
	}

	vals += expr_->getValue();
    }

    for ( int idx=nrstart; idx<vals.size(); idx++ )
    {
	const float dah = dahrg_.atIndex( idx - nrstart );
	wlout.addValue( dah, vals[idx] );
    }

    return true;
}

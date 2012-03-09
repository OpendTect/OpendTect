/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelllogcalc.cc,v 1.23 2012-03-09 15:03:39 cvshelene Exp $";


#include "uiwelllogcalc.h"

#include "uitoolbutton.h"
#include "uigeninput.h"
#include "uimathexpression.h"
#include "uirockphysform.h"
#include "uicombobox.h"
#include "uimsg.h"
#include "uitable.h"
#include "uilabel.h"
#include "uilineedit.h"
#include "uiseparator.h"

#include "welllogset.h"
#include "wellreader.h"
#include "welldata.h"
#include "wellwriter.h"
#include "welllog.h"
#include "separstr.h"
#include "survinfo.h"
#include "mathexpression.h"
#include "unitofmeasure.h"
#include "ioman.h"
#include "ioobj.h"

static const int cMaxNrInps = 6;
static const char* specvararr[] = { "MD", "DZ", 0 };
static const BufferStringSet specvars( specvararr );

class uiWellLogCalcInpData : public uiGroup
{
public:

uiWellLogCalcInpData( uiWellLogCalc* p, uiGroup* inpgrp, int fieldnr )
    : uiGroup(inpgrp,"Inp data group")
    , idx_(fieldnr)
    , wls_(&p->wls_)
    , lognms_(p->lognms_)
    , lognmsettodef_(false)
    , convertedlog_(0)
{
    varmfld_ = new uiGenInput( this, "For" );
    varmfld_->setElemSzPol( uiObject::Small );

    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, lognms_, "use",
				    BufferString("input ",fieldnr) );
    inpfld_ = lcb->box();
//    inpfld_->addItem( "Constant" );
    int selidx = fieldnr;
//    if ( selidx > lognms_.size() ) selidx = lognms_.size();
    if ( selidx >= lognms_.size() ) selidx = lognms_.size()-1;
    inpfld_->setCurrentItem( selidx );
    inpfld_->selectionChanged.notify( mCB(p,uiWellLogCalc,inpSel) );
    lcb->attach( rightOf, varmfld_ );

    uiLabeledComboBox* unitlcb = new uiLabeledComboBox( this, "convert to:",
					    BufferString( "unitbox ",fieldnr ));
    unfld_ = unitlcb->box();
    const ObjectSet<const UnitOfMeasure>& alluom( UoMR().all() );
    unfld_->addItem( "-" );
    for ( int idx=0; idx<alluom.size(); idx++ )
	unfld_->addItem( alluom[idx]->name() );
    unitlcb->attach( rightOf, lcb );

    udfbox_ = new uiCheckBox( this, "Fill empty sections" );
    udfbox_->attach( rightOf, unitlcb );

    cstvalfld_ = new uiGenInput( this, "value", FloatInpSpec() );
    cstvalfld_->attach( rightOf, lcb );
    cstvalfld_->display( false );

    setHAlignObj( lcb );
}


~uiWellLogCalcInpData()
{
    if ( convertedlog_ ) delete convertedlog_;
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
    return wls_->getLog( inpfld_->text() );
}

bool getInp( uiWellLogCalc::InpData& inpdata )
{
    inpdata.noudf_ = udfbox_->isChecked();
    inpdata.wl_ = getLog();
    const char* logunitnm = inpdata.wl_->unitMeasLabel();
    const UnitOfMeasure* logun = UoMR().get( logunitnm );
    const UnitOfMeasure* convertun = UoMR().get( unfld_->text() );
    if ( !logun || !convertun )
	return inpdata.wl_;		//TODO: would we want to stop?

    if ( logun == convertun )
	return inpdata.wl_;

    if ( !inpdata.wl_ )
	return false;

    convertedlog_ = new Well::Log( *inpdata.wl_ );
    for ( int idx=0; idx<inpdata.wl_->size(); idx++ )
    {
	const float initialval = inpdata.wl_->value( idx );
	const float valinsi = logun->getSIValue( initialval );
	const float convertedval = convertun->getUserValueFromSI( valinsi );
	convertedlog_->valArr()[idx] = convertedval;
    }

    inpdata.wl_ = convertedlog_;
    return true;
}


void setUnit( const char* s )
{
    unfld_->setText( s );
}

    uiGenInput*		varmfld_;
    uiComboBox*		inpfld_;
    uiComboBox*		unfld_;
    uiCheckBox*		udfbox_;
    uiGenInput*		cstvalfld_;
    const int		idx_;
    const Well::LogSet*	wls_;
    Well::Log*		convertedlog_;
    const BufferStringSet&	lognms_;
    bool		lognmsettodef_;

};


static BufferString getDlgTitle( const TypeSet<MultiID>& wllids )
{
    const int sz = wllids.size();
    if ( sz < 1 )
	return BufferString( "No wells selected" );

    BufferString ret( "Calculate new logs for '", IOM().nameOf(wllids[0]), "'");
    for ( int idx=1; idx<sz; idx++ )
	ret.add( ", '" ).add( IOM().nameOf(wllids[idx]) ).add( "'" );

    ret = getLimitedDisplayString( ret.buf(), 80, true );
    return ret;
}


uiWellLogCalc::uiWellLogCalc( uiParent* p, const Well::LogSet& ls,
			      const BufferStringSet& lognms,
			      const TypeSet<MultiID>& wllids )
	: uiDialog(p,uiDialog::Setup("Calculate new logs",
				     getDlgTitle(wllids),
				     "107.1.10"))
    	, wls_(ls)
    	, lognms_(lognms)
    	, wellids_(wllids)
    	, formfld_(0)
    	, nrvars_(0)
    	, expr_(0)
    	, havenew_(false)
{
    if ( lognms_.isEmpty() || wellids_.isEmpty() )
    {
	new uiLabel( this, lognms.isEmpty() ? "No logs" : "No wells" );
	setCtrlStyle( LeaveOnly );
	return;
    }

    setCtrlStyle( DoAndStay );
    const CallBack formsetcb( mCB(this,uiWellLogCalc,formSet) );

    uiGroup* inpgrp = new uiGroup( this, "inp grp" );
    uiMathExpression::Setup mesu( "Formula (like 'den / son')" );
    mesu.withsetbut( true ).fnsbelow( false );
    formfld_ = new uiMathExpression( inpgrp, mesu );
    formfld_->formSet.notify( formsetcb );
    uiToolButtonSetup tbsu( "rockphys.png", "Choose rockphysics formula",
	    		    mCB(this,uiWellLogCalc,rockPhysReq), "RockPhysics");
    formfld_->addButton( tbsu );
    inpgrp->setHAlignObj( formfld_ );

    for ( int idx=0; idx<cMaxNrInps; idx++ )
    {
	uiWellLogCalcInpData* fld = new uiWellLogCalcInpData(this,inpgrp,idx);
	if ( idx )
	    fld->attach( alignedBelow, inpdataflds_[idx-1] );
	else
	    fld->attach( alignedBelow, formfld_ );
	inpdataflds_ += fld;
    }

    uiSeparator* sep = new uiSeparator( this, "sep" );
    sep->attach( stretchedBelow, inpgrp );

    float defsr = 1;
    if ( !wls_.isEmpty() )
	defsr = wls_.getLog(0).dahStep( false );
    srfld_ = new uiGenInput( this, "Output sample distance",
	    			FloatInpSpec(defsr));
    srfld_->attach( alignedBelow, inpgrp );
    srfld_->attach( ensureBelow, sep );
    ftbox_ = new uiCheckBox( this, "Feet" );
    ftbox_->setChecked( SI().depthsInFeetByDefault() );
    ftbox_->activated.notify( mCB(this,uiWellLogCalc,feetSel) );
    ftbox_->attach( rightOf, srfld_ );

    nmfld_ = new uiGenInput( this, "Name for new log" );
    nmfld_->attach( alignedBelow, srfld_ );

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
    if ( !formfld_ ) return;

    const BufferString inp( formfld_->text() );
    if ( inp.isEmpty() ) return;

    MathExpressionParser mep( inp );
    expr_ = mep.parse();

    if ( !expr_ )
	uiMSG().warning(
	BufferString("The provided expression cannot be used:\n",mep.errMsg()));
}


class uiWellLogCalcRockPhys : public uiDialog
{
public:

uiWellLogCalcRockPhys( uiParent* p )
    : uiDialog(p, uiDialog::Setup("Rock Physics",
				 "Use a rock physics formula", mTODOHelpID))
{
    formgrp_ = new uiRockPhysForm( this );
}

bool acceptOK( CallBacker* )
{
    if ( !formgrp_->isOK() )
    {
	uiMSG().error( formgrp_->errMsg() );
	return false;
    }

    return true;
}


bool getFormulaInfo( BufferString& cleanformula, BufferString& outputunit,
		     BufferStringSet& varsunits ) const
{
    return formgrp_->getFormulaInfo( cleanformula, outputunit, varsunits );
}

    uiRockPhysForm*	formgrp_;

};


void uiWellLogCalc::rockPhysReq( CallBacker* )
{
    uiWellLogCalcRockPhys dlg( this );
    BufferString formula;
    BufferString outunit;
    if ( dlg.go() && dlg.getFormulaInfo( formula, outunit, inputunits_ ) )
    {
	formfld_->setText( formula.buf() );
	unfld_->setText( outunit.buf() );
	rpoutunit_ = outunit;
	formSet( 0 );
    }
}


void uiWellLogCalc::feetSel( CallBacker* )
{
    zsampintv_ = srfld_->getfValue();
    if ( !mIsUdf(zsampintv_) )
    {
	zsampintv_ *= ftbox_->isChecked() ? mToFeetFactor : mFromFeetFactor;
	srfld_->setValue( zsampintv_ );
    }
}


void uiWellLogCalc::formSet( CallBacker*  c )
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
    {
	inpdataflds_[idx]->use( useexpr );
	if ( inputunits_.size()>idx )
	    inpdataflds_[idx]->setUnit( inputunits_.get(idx).buf() );
    }

    inpSel( 0 );
    if ( c ) rpoutunit_.setEmpty();
}


void uiWellLogCalc::inpSel( CallBacker* )
{
    if ( nrvars_ < 1 ) return;

    float sr = mUdf(float);
    for ( int idx=0; idx<nrvars_; idx++ )
    {
	if ( idx >= inpdataflds_.size() ) break;

	const Well::Log* wl = inpdataflds_[idx]->getLog();
	if ( !wl ) { pErrMsg("Huh"); continue; }
	if ( wl->isEmpty() ) continue;

	sr = wl->dahStep( false );
	if ( !mIsUdf(sr) )
	    break;
    }
    if ( mIsUdf(sr) ) return;

    if ( ftbox_->isChecked() )
	sr *= mToFeetFactor;
    srfld_->setValue( sr );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiWellLogCalc::acceptOK( CallBacker* )
{
    getMathExpr();
    if ( !expr_ ) return formfld_ ? false : true;
    nrvars_ = expr_->nrUniqueVarNames();

    const BufferString newnm = nmfld_->text();
    if ( newnm.isEmpty() )
	mErrRet("Please provide a name for the new log")
    if ( lognms_.isPresent(newnm) || wls_.getLog(newnm) )
	mErrRet("A log with this name already exists."
		"\nPlease enter a different name for the new log")

    zsampintv_ = srfld_->getfValue();
    if ( mIsUdf(zsampintv_) )
	mErrRet("Please provide the Z dample rate for the  output log")
    if ( ftbox_->isChecked() )
    	zsampintv_ *= mFromFeetFactor;


    //TODO needs to be in Executor
    for ( int iwell=0; iwell<wellids_.size(); iwell++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( wellids_[iwell] );
	if ( !ioobj ) continue; //TODO errh required
	Well::Data wd;
	const BufferString fnm( ioobj->fullUserExpr(true) );
	Well::Reader rdr( fnm, wd );
	if ( !rdr.getLogs() ) continue; //TODO errh required
	Well::LogSet& wls = wd.logs();
	setCurWls( wls );
	TypeSet<InpData> inpdata;
	if ( !getInpData(inpdata) || !getRecInfo() )
	    continue; //TODO errh required

	Well::Log* newwl = new Well::Log( newnm );
	if ( !calcLog(*newwl,inpdata) )
	    { delete newwl; continue; } //TODO errh required
	const int unselidx = unfld_->currentItem();
	if ( unselidx > 0 )
	{
	    const char* desunittxt = unfld_->text();
	    newwl->setUnitMeasLabel( desunittxt );
	    if ( !rpoutunit_.isEmpty() )
	    {
		const UnitOfMeasure* logun = UoMR().get( rpoutunit_.buf() );
		const UnitOfMeasure* convertun = UoMR().get( desunittxt );
		if ( logun && convertun && logun!=convertun )
		{
		    for ( int idx=0; idx<newwl->size(); idx++ )
		    {
			const float initialval = newwl->value( idx );
			const float valinsi = logun->getSIValue( initialval );
			const float convertedval =
				    convertun->getUserValueFromSI( valinsi );
			newwl->valArr()[idx] = convertedval;
		    }
		}
	    }
	}

	wls.add( newwl );

	Well::Writer wtr( fnm, wd );
	if ( !wtr.putLogs() ) continue; //TODO errh required
    }

    setCurWls( wls_ );
    uiMSG().message( "Successfully added this log" );
    havenew_ = true;
    return false;
}


void uiWellLogCalc::setCurWls( const Well::LogSet& wls )
{
    for ( int idx=0; idx<inpdataflds_.size(); idx++ )
	inpdataflds_[idx]->wls_ = &wls;
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

    StepInterval<float> samprg;
    samprg.step = zsampintv_;
    Interval<float> dahrg( wls_.dahInterval() );
    if ( !inpdata.isEmpty() )
    {
	dahrg = inpdata[0].wl_->dahRange();
	for ( int idx=1; idx<inpdata.size(); idx++ )
	    dahrg.include( inpdata[idx].wl_->dahRange(), false );
    }
    samprg.start = dahrg.start; samprg.stop = dahrg.stop;
    const int endrgidx = samprg.nrSteps();
    for ( ; rgidx<=endrgidx; rgidx++ )
    {
	const float dah = samprg.atIndex( rgidx );
	for ( int iinp=0; iinp<inpdata.size(); iinp++ )
	{
	    const uiWellLogCalc::InpData& inpd = inpdata[iinp];
	    const float curdah = dah + samprg.step * inpd.shift_;
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
		else if ( inpd.specidx_ == 1 )	val = samprg.step;
		expr_->setVariableValue( iinp, val );
	    }
	}

	vals += expr_->getValue();
    }

    for ( int idx=nrstart; idx<vals.size(); idx++ )
    {
	const float dah = samprg.atIndex( idx - nrstart );
	wlout.addValue( dah, vals[idx] );
    }

    wlout.removeTopBottomUdfs();
    return true;
}

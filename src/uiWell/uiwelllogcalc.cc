/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


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
#include "uiwelllogcalcinpdata.h"
#include "uiunitsel.h"

#include "ioman.h"
#include "ioobj.h"
#include "mathformula.h"
#include "separstr.h"
#include "survinfo.h"
#include "mathexpression.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellreader.h"
#include "welltrack.h"
#include "wellwriter.h"

#define mMDIdx	0
#define mDZIdx	1
#define mTVDSSIdx	2
#define mTVDIdx	3
#define mTVDSDIdx	4
#define	mTWTIdx	5
#define	mVelIdx	6

static const int cMaxNrInps = 6;
static const char* specvararr[] = { "MD", "DZ", "TVDSS", "TVD", "TVDSD", "TWT",
				    "VINT", 0 };
static const BufferStringSet specvars( specvararr );

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


uiWellLogCalc::uiWellLogCalc( uiParent* p, const TypeSet<MultiID>& wllids,
			      bool rockphysmode )
	: uiDialog(p,uiDialog::Setup("Calculate new logs",
				     getDlgTitle(wllids),
				     "107.1.10"))
	, wls_(*new Well::LogSet)
	, form_(*new Math::Formula)
	, wellids_(wllids)
	, formfld_(0)
	, havenew_(false)
{
    if ( wellids_.isEmpty() )
    {
	new uiLabel( this, "No wells.\nPlease import or create a well first." );
	setCtrlStyle( CloseOnly );
	return;
    }

    MouseCursorChanger mcc( MouseCursor::Wait );
    getAllLogs();
    if ( wls_.isEmpty() || lognms_.isEmpty() )
    {
	new uiLabel( this, "Selected wells have no logs.\n"
			   "Please import at least one." );
	setCtrlStyle( CloseOnly );
	return;
    }

    setCtrlStyle( RunAndClose );
    const CallBack formsetcb( mCB(this,uiWellLogCalc,formSet) );

    uiGroup* inpgrp = new uiGroup( this, "inp grp" );
    uiMathExpression::Setup mesu( "Formula (like 'den / son')" );
    mesu.withsetbut( true ).fnsbelow( false );
    formfld_ = new uiMathExpression( inpgrp, mesu );
    formfld_->formSet.notify( formsetcb );
    const CallBack rockphyscb( mCB(this,uiWellLogCalc,rockPhysReq) );
    uiToolButtonSetup tbsu( "rockphys", "Choose rockphysics formula",
			    rockphyscb, "&Rock Physics");
    formfld_->addButton( tbsu );
    inpgrp->setHAlignObj( formfld_ );

    for ( int idx=0; idx<cMaxNrInps; idx++ )
    {
	uiWellLogCalcInpData* fld = new uiWellLogCalcInpData(this,inpgrp,idx);
	if ( idx )
	    fld->attach( alignedBelow, inpdataflds_[idx-1] );
	else
	    fld->attach( alignedBelow, formfld_ );
	fld->inpSel.notify( mCB(this,uiWellLogCalc,inpSel) );
	inpdataflds_ += fld;
    }

    uiUnitSel::Setup uussu( PropertyRef::Other, "Formula result is" );
    uussu.selproptype( true ).withnone( true );
    formulaunfld_ = new uiUnitSel( inpgrp, uussu );
    formulaunfld_->attach( alignedBelow,
		    inpdataflds_[inpdataflds_.size()-1]->attachObj() );

    uiSeparator* sep = new uiSeparator( this, "sep" );
    sep->attach( stretchedBelow, inpgrp );

    float defsr = SI().depthsInFeet() ? 0.5f : 0.1524f;
    if ( !wls_.isEmpty() )
	defsr = wls_.getLog(0).dahStep( false );
    srfld_ = new uiGenInput( this, "Output sample distance",
			     FloatInpSpec(defsr) );
    srfld_->attach( alignedBelow, inpgrp );
    srfld_->attach( ensureBelow, sep );
    ftbox_ = new uiCheckBox( this, "Feet" );
    ftbox_->setChecked( SI().depthsInFeet() );
    ftbox_->activated.notify( mCB(this,uiWellLogCalc,feetSel) );
    ftbox_->attach( rightOf, srfld_ );

    nmfld_ = new uiGenInput( this, "Name for new log" );
    nmfld_->attach( alignedBelow, srfld_ );

    uussu.lbltxt( "Output unit of measure" );
    outunfld_ = new uiUnitSel( this, uussu );
    outunfld_->attach( alignedBelow, nmfld_ );

    postFinalise().notify( formsetcb );
    if ( rockphysmode )
	afterPopup.notify( rockphyscb );
}


void uiWellLogCalc::getAllLogs()
{
    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	IOObj* ioobj = IOM().get( wellids_[idx] );
	if ( !ioobj ) continue;

	Well::Data wd; Well::Reader wr( ioobj->fullUserExpr(true), wd );
	BufferStringSet nms; wr.getLogInfo( nms );
	bool havenewlog = false;
	for ( int inm=0; inm<nms.size(); inm++ )
	{
	    const char* lognm = nms.get(inm).buf();
	    if ( !lognms_.isPresent(lognm) )
	    {
		havenewlog = true;
		lognms_.add( lognm );
	    }
	}
	if ( havenewlog )
	{
	    wr.getLogs();
	    for ( int ilog=0; ilog<wd.logs().size(); ilog++ )
	    {
		const Well::Log& wl = wd.logs().getLog( ilog );
		if ( !wls_.getLog(wl.name()) )
		    wls_.add( new Well::Log(wl) );
	    }
	}
	delete ioobj;
    }
}


uiWellLogCalc::~uiWellLogCalc()
{
    delete &form_;
    delete &wls_;
}


bool uiWellLogCalc::checkValidNrInputs( const Math::Formula& form ) const
{
    if ( form.nrInputs() > inpdataflds_.size() )
    {
	BufferString msg( "Sorry, the expression contains ", form.nrInputs(),
			  "variables.\nThe maximum number is " );
	msg.add( inpdataflds_.size() );
	uiMSG().error( msg );
	return false;
    }
    return true;
}


bool uiWellLogCalc::updateForm( Math::Formula& form ) const
{
    if ( !formfld_ )
	return false;

    form.setText( formfld_->text() );
    if ( !checkValidNrInputs(form) )
	return false;

    for ( int idx=0; idx<form.nrInputs(); idx++ )
    {
	const uiWellLogCalcInpData& inpfld = *inpdataflds_[idx];
	form.setInputDef( idx, inpfld.getInput() );
	form.setInputUnit( idx, inpfld.getUnit() );
    }

    form.setOutputUnit( formulaunfld_->getUnit() );
    return true;
}


bool uiWellLogCalc::useForm( const Math::Formula& form,
			     const TypeSet<PropertyRef::StdType>* inputtypes )
{
    const bool isbad = form.isBad();
    formfld_->setText( isbad ? "" : form.text() );
    formulaunfld_->setUnit( isbad ? 0 : form.outputUnit() );
    for ( int idx=0; idx<inpdataflds_.size(); idx++ )
    {
	uiWellLogCalcInpData& inpfld = *inpdataflds_[idx];
	inpfld.use( form );
	if ( !isbad )
	{
	    const PropertyRef::StdType ptyp
		= inputtypes && inputtypes->validIdx(idx) ? (*inputtypes)[idx]
							  : PropertyRef::Other;
	    inpfld.restrictLogChoice( ptyp );
	}
    }

    if ( isbad )
    {
	uiMSG().error( BufferString("Invalid expression:\n",form.errMsg()));
	return false;
    }

    return checkValidNrInputs( form );
}


class uiWellLogCalcRockPhys : public uiDialog
{
public:

uiWellLogCalcRockPhys( uiParent* p )
    : uiDialog(p, uiDialog::Setup("Rock Physics",
				 "Use a rock physics formula", "107.1.12"))
{
    formgrp_ = new uiRockPhysForm( this );
}

bool acceptOK( CallBacker* )
{
    if ( !formgrp_->isOK() )
	{ uiMSG().error( formgrp_->errMsg() ); return false; }

    return true;
}

bool getFormulaInfo( Math::Formula& form,
		     TypeSet<PropertyRef::StdType>& varstypes ) const
{
    return formgrp_->getFormulaInfo(form,&varstypes);
}

    uiRockPhysForm*	formgrp_;

};


void uiWellLogCalc::rockPhysReq( CallBacker* )
{
    uiWellLogCalcRockPhys dlg( this );
    Math::Formula newform;
    TypeSet<PropertyRef::StdType> inputtypes;
    if ( !dlg.go() || !dlg.getFormulaInfo(newform,inputtypes) )
	return;

    useForm( newform, &inputtypes );
}


void uiWellLogCalc::feetSel( CallBacker* )
{
    zsampintv_ = srfld_->getfValue();
    if ( !mIsUdf(zsampintv_) )
    {
	zsampintv_ *= ftbox_->isChecked() ? mToFeetFactorF : mFromFeetFactorF;
	srfld_->setValue( zsampintv_ );
    }
}


void uiWellLogCalc::formSet( CallBacker* )
{
    Math::Formula form;
    if ( updateForm(form) )
	useForm( form );
}


void uiWellLogCalc::inpSel( CallBacker* )
{
    float sr = srfld_->getfValue();
    if ( !mIsUdf(sr) )
	return;

    for ( int idx=0; idx<inpdataflds_.size(); idx++ )
    {
	const Well::Log* wl = inpdataflds_[idx]->getLog();
	if ( !wl && !inpdataflds_[idx]->isCst() )
		{ pErrMsg("Huh"); continue; }
	if ( !wl || wl->isEmpty() )
	    continue;

	sr = wl->dahStep( false );
	if ( !mIsUdf(sr) )
	    break;
    }

    if ( mIsUdf(sr) ) return;

    if ( ftbox_->isChecked() )
	sr *= mToFeetFactorF;
    srfld_->setValue( sr );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

#define mErrContinue(s) { uiMSG().error(s); continue; }


bool uiWellLogCalc::acceptOK( CallBacker* )
{
    if ( !formfld_ )
	return true;

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
	zsampintv_ *= mFromFeetFactorF;

    Math::Formula form;
    if ( !updateForm(form) )
	return false;

    bool successfulonce = false;
    for ( int iwell=0; iwell<wellids_.size(); iwell++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( wellids_[iwell] );
	if ( !ioobj )
	{
	    BufferString msg = "Impossible to retrieve the ";
	    msg += "selected well number ";
	    msg += iwell;
	    mErrContinue( msg.buf());
	}
	Well::Data wd;
	const BufferString fnm( ioobj->fullUserExpr(true) );
	Well::Reader rdr( fnm, wd );
	if ( !rdr.getLogs() )
	{
	    BufferString msg( "Cannot read well logs for well ",ioobj->name() );
	    mErrContinue( msg.buf());
	}
	Well::LogSet& wls = wd.logs();
	setCurWls( wls );
	TypeSet<InpData> inpdata;
	if ( !getInpData(form,inpdata) || !getRecInfo(form) )
	    continue;

	if ( SI().zIsTime() )
	{
	    if ( !rdr.getD2T() )
	    {
		BufferString msg( "Cannot read time-depth model for well ",
				  ioobj->name() );
		mErrContinue( msg.buf() );
	    }
	}

	Well::Log* newwl = new Well::Log( newnm );
	if ( !calcLog(*newwl,form,inpdata,wd.track(),wd.d2TModel()) )
	{
	    delete newwl;
	    BufferString msg( "Cannot compute well log '", newnm, "'" );
	    mErrContinue( msg.buf() );
	}

	const UnitOfMeasure* outun = outunfld_->getUnit();
	if ( outun )
	{
	    const UnitOfMeasure* logun = form.outputUnit();
	    for ( int idx=0; idx<newwl->size(); idx++ )
	    {
		const float initialval = newwl->value( idx );
		const float valinsi = !logun ? initialval
				    : logun->getSIValue( initialval );
		const float convertedval = outun->getUserValueFromSI( valinsi );
		newwl->valArr()[idx] = convertedval;
	    }
	}

	if ( outun )
	    newwl->setUnitMeasLabel( outun->name() );

	wls.add( newwl );
	Well::Writer wtr( fnm, wd );
	if ( !wtr.putLogs() )
	{
	    BufferString msg( "Cannot write new logs for well ",ioobj->name() );
	    msg.addNewLine().add( "Check the permissions of the *.wll files" );
	    wls.remove( wls.size()-1 );
	    //Do not keep in memory what is not on disk
	    mErrContinue( msg.buf() );
	}

	successfulonce = true;
    }

    setCurWls( wls_ );
    if ( !successfulonce )
	return false;

    uiMSG().message( "Successfully added this log" );
    havenew_ = true;
    return false;
}


void uiWellLogCalc::setCurWls( const Well::LogSet& wls )
{
    for ( int idx=0; idx<inpdataflds_.size(); idx++ )
	inpdataflds_[idx]->wls_ = &wls;
}


bool uiWellLogCalc::getInpData( const Math::Formula& form,
				TypeSet<uiWellLogCalc::InpData>& inpdata )
{
    for ( int iinp=0; iinp<form.nrInputs(); iinp++ )
    {
	InpData inpd;

	const BufferString varnm( form.variableName(iinp) );
	inpd.specidx_ = specvars.indexOf( varnm.buf() );
	if ( inpd.specidx_ < 0 )
	{
	    uiWellLogCalcInpData* inpfld = 0;
	    for ( int ivar=0; ivar<inpdataflds_.size(); ivar++ )
	    {
		if ( inpdataflds_[ivar]->hasVarName(varnm) )
		    { inpfld = inpdataflds_[ivar]; break; }
	    }
	    if ( !inpfld || !inpfld->getInp(inpd) )
		mErrRet(BufferString(
		    "Internal: Can't find log corresponding to variable: '",
		     varnm.buf(),"'") )
	    if ( inpd.wl_ && inpd.wl_->isEmpty() )
		mErrRet(BufferString("The well log chosen for variable: '",
				     varnm.buf(),"' is empty."))
	}
	else if ( !SI().zIsTime() &&
		  (inpd.specidx_ == mTWTIdx || inpd.specidx_ == mVelIdx) )
	{
	    mErrRet(BufferString("Cannot use: '",
				  varnm.buf(),"' in a depth survey."))
	}

	inpdata += inpd;
    }

    return true;
}


bool uiWellLogCalc::getRecInfo( Math::Formula& form )
{
    if ( !form_.isRecursive() )
	return true;

    const int maxshft = form_.maxRecShift();
    const char* addeds = maxshft > 0 ? "s" : 0;
    uiDialog dlg( this, uiDialog::Setup(BufferString("Recursion start value",
				addeds),mNoDlgTitle,mNoHelpKey) );
    uiLabel* lbl = new uiLabel( &dlg, BufferString(
		"Recursive calculation: Please enter starting value", addeds) );

    BufferString txt( "Start value" );
    if ( maxshft > 1 )
	txt.add( "s (" ).add( maxshft ).add( " comma-separated values)" );
    uiGenInput* fld = new uiGenInput( &dlg, txt, StringInpSpec() );
    fld->attach( centeredBelow, lbl );
    lbl = new uiLabel( &dlg, "This will provide the first 'out' value(s)" );
    lbl->attach( centeredBelow, fld );
    if ( !dlg.go() )
	return false;

    const SeparString usrinp( fld->text() );
    int nrvals = usrinp.size();
    if ( nrvals > maxshft ) nrvals = maxshft;
    for ( int idx=0; idx<nrvals; idx++ )
    {
	float val = toFloat( usrinp[idx] );
	if ( mIsUdf(val) )
	    break;
	form.recStartVals()[idx] = val;
    }

    return true;
}


bool uiWellLogCalc::calcLog( Well::Log& wlout, const Math::Formula& form,
			     const TypeSet<uiWellLogCalc::InpData>& inpdata,
			     Well::Track& track, Well::D2TModel* d2t )
{
    if ( inpdata.isEmpty() )
	{ pErrMsg("Wrong equation: check syntax"); return false; }
    form.startNewSeries();

    Interval<float> dahrg( mUdf(float), mUdf(float) );
    for ( int iinp=0; iinp<inpdata.size(); iinp++ )
    {
	if ( !inpdata[iinp].wl_ )
	    continue;

	if ( dahrg.isUdf() )
	    dahrg = inpdata[iinp].wl_->dahRange();
	else
	    dahrg.include( inpdata[iinp].wl_->dahRange(), false );
    }
    if ( dahrg.isUdf() )
	dahrg = track.dahRange();

    StepInterval<float> samprg( dahrg.start, dahrg.stop, zsampintv_ );
    const int nrsamps = samprg.nrSteps() + 1;
    TypeSet<float> inpvals( form.nrInputs(), 0 );
    for ( int rgidx=0; rgidx<nrsamps; rgidx++ )
    {
	const float dah = samprg.atIndex( rgidx );
	inpvals.setAll( 0 );
	for ( int iinp=0; iinp<inpdata.size(); iinp++ )
	{
	    const uiWellLogCalc::InpData& inpd = inpdata[iinp];
	    const float curdah = dah + samprg.step * inpd.shift_;
	    if ( inpd.wl_ )
	    {
		const float val = inpd.wl_->getValue( curdah, inpd.noudf_ );
		inpvals[iinp] = val;
	    }
	    else if ( inpd.iscst_ )
		inpvals[iinp] = inpd.cstval_;
	    else
	    {
		float val = mUdf(float);
		if ( inpd.specidx_ == mMDIdx )	val = curdah;
		else if ( inpd.specidx_ == mDZIdx )	val = samprg.step;
		else if ( inpd.specidx_ == mTVDSSIdx ||
			  inpd.specidx_ == mTVDIdx ||
			  inpd.specidx_ == mTVDSDIdx )
		{
		    val = mCast(float,track.getPos(curdah).z);
		    if ( inpd.specidx_ == mTVDIdx && !mIsUdf(val) )
			val += track.getKbElev();
		    else if ( inpd.specidx_ == mTVDSDIdx  && !mIsUdf(val) )
			val += mCast(float, SI().seismicReferenceDatum());
		}
		else if ( inpd.specidx_ == mTWTIdx && d2t )
		{
		    val = d2t->getTime( curdah, track );
		    const UnitOfMeasure* uom =
					UnitOfMeasure::surveyDefZUnit();
		    if ( uom ) val = uom->userValue( val );

		}
		else if ( inpd.specidx_ == mVelIdx && d2t )
		{
		    val = mCast(float,d2t->getVelocityForDah( curdah, track ));
		    const UnitOfMeasure* uom =
					 UnitOfMeasure::surveyDefDepthUnit();
		    if ( uom ) val = uom->userValue( val );
		}

		inpvals[iinp] = val;
	    }
	}

	const float formval = form.getValue( inpvals.arr(), false );
	wlout.addValue( dah, formval );
    }

    wlout.removeTopBottomUdfs();
    return true;
}


void uiWellLogCalc::setOutputLogName( const char* nm )
{
    nmfld_->setText( nm );
}


const char* uiWellLogCalc::getOutputLogName() const
{
    return nmfld_->text();
}

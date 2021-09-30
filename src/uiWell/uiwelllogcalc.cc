/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		October 2003
________________________________________________________________________

-*/


#include "uiwelllogcalc.h"

#include "uitoolbutton.h"
#include "uigeninput.h"
#include "uimathformula.h"
#include "uimathexpressionvariable.h"
#include "uirockphysform.h"
#include "uimsg.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uicombobox.h"
#include "uiunitsel.h"
#include "uiwelllogdisplay.h"

#include "hiddenparam.h"
#include "ioman.h"
#include "ioobj.h"
#include "mathformula.h"
#include "mathspecvars.h"
#include "ptrman.h"
#include "survinfo.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellreader.h"
#include "welltrack.h"
#include "wellwriter.h"
#include "od_helpids.h"

#define mMDIdx		0
#define mTVDIdx		1
#define mTVDSSIdx	2
#define mTVDSDIdx	3
#define mDZIdx		4
#define	mTWTIdx		5
#define	mVelIdx		6

#define mInterpNone	0
#define mInterpMax1	1
#define mInterpNotAll	2
#define mInterpAll	3
static uiStringSet getInterpolationOptions()
{
    uiStringSet pols;
    pols.add(uiStrings::sNo()).add( toUiString("One log max"))
	.add(toUiString("Unless all undef")).add(uiStrings::sYes());
    return pols;
}


static Math::SpecVarSet& getSpecVars()
{
    mDefineStaticLocalObject( Math::SpecVarSet, svs, );
    svs.setEmpty();

    svs.add( "MD", "Depth Along Hole", true, PropertyRef::Dist );
    svs.add( "TVD", "Z coordinate", true, PropertyRef::Dist );
    svs.add( "TVDSS", "TVD below SS", true, PropertyRef::Dist );
    svs.add( "TVDSD", "TVD below SD", true, PropertyRef::Dist );
    svs.add( "DZ", "Delta Z", true, PropertyRef::Dist );
    if ( SI().zIsTime() )
    {
	svs.add( "TWT", "Two-way traveltime", true, PropertyRef::Time );
	svs.add( "VINT", "Interval velocity", true, PropertyRef::Vel );
    }
    return svs;
}


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


static HiddenParam<uiWellLogCalc,uiToolButton*> outputviewbuts(nullptr);
HiddenParam<uiWellLogCalc,Notifier<uiWellLogCalc>*> hp_logschanged( nullptr );


uiWellLogCalc::uiWellLogCalc( uiParent* p, const TypeSet<MultiID>& wllids,
			      bool rockphysmode )
	: uiDialog(p,uiDialog::Setup(tr("Calculate New Logs"),
				     mToUiStringTodo(getDlgTitle(wllids)),
				     mODHelpKey(mWellLogCalcHelpID) ))
	, superwls_(*new Well::LogSet)
	, form_(*new Math::Formula(true,getSpecVars()))
	, wellids_(wllids)
	, formfld_(0)
	, nmfld_(0)
	, havenew_(false)
{
    outputviewbuts.setParam( this, nullptr );
    hp_logschanged.setParam( this, new Notifier<uiWellLogCalc>( this ) );

    if ( wellids_.isEmpty() )
    {
	new uiLabel( this, tr("No wells.\nPlease import"
			      " or create a well first.") );
	setCtrlStyle( CloseOnly );
	return;
    }

    MouseCursorChanger mcc( MouseCursor::Wait );
    getAllLogs();
    if ( superwls_.isEmpty() || lognms_.isEmpty() )
    {
	new uiLabel( this, tr("Selected wells have no logs.\n"
			   "Please import at least one.") );
	setCtrlStyle( CloseOnly );
	return;
    }

    setOkCancelText( uiStrings::sCalculate(), uiStrings::sClose() );
    const CallBack formsetcb( mCB(this,uiWellLogCalc,formSet) );
    const CallBack formunitcb( mCB(this,uiWellLogCalc,formUnitSel) );
    const CallBack inpselcb( mCB(this,uiWellLogCalc,inpSel) );

    uiMathFormula::Setup mfsu( tr("Formula (like 'den / son')") );
    mfsu.stortype_ = "Log calculation";
    formfld_ = new uiMathFormula( this, form_, mfsu );
    formfld_->addInpViewIcon( "view_log", "Display this log",
			      mCB(this,uiWellLogCalc,vwLog) );
    formfld_->setNonSpecInputs( lognms_ );
    formfld_->inpSet.notify( inpselcb );
    formfld_->formSet.notify( formsetcb );
    formfld_->formUnitSet.notify( formunitcb );
    const CallBack rockphyscb( mCB(this,uiWellLogCalc,rockPhysReq) );
    uiToolButtonSetup tbsu( "rockphys", tr("Choose rockphysics formula"),
			    rockphyscb, uiStrings::sRockPhy() );
    formfld_->addButton( tbsu );

    uiSeparator* sep = new uiSeparator( this, "sep" );
    sep->attach( stretchedBelow, formfld_ );

    float defsr = SI().depthsInFeet() ? 0.5f : 0.1524f;
    if ( !superwls_.isEmpty() )
    {
	defsr = superwls_.getLog(0).dahStep( false );
	if ( SI().depthsInFeet() )
	    defsr *= mToFeetFactorF;
    }

    srfld_ = new uiGenInput( this, uiStrings::phrOutput( tr("sample distance")),
			     FloatInpSpec(defsr) );
    srfld_->attach( alignedBelow, formfld_ );
    srfld_->attach( ensureBelow, sep );

    ftbox_ = new uiCheckBox( this, tr("Feet") );
    ftbox_->setChecked( SI().depthsInFeet() );
    ftbox_->activated.notify( mCB(this,uiWellLogCalc,feetSel) );
    ftbox_->attach( rightOf, srfld_ );

    auto* lcb = new uiLabeledComboBox( this, getInterpolationOptions(),
				       tr("Inter/extrapolate input logs?"));
    interppolfld_ = lcb->box();
    interppolfld_->setCurrentItem( 0 );
    lcb->attach( rightTo, ftbox_ );

    nmfld_ = new uiGenInput( this, tr("Name for new log") );
    nmfld_->attach( alignedBelow, srfld_ );

/*
    uiToolButton* viewbut = new uiToolButton( this, "view_log",
	tr("View output log"), mCB(this,uiWellLogCalc,viewOutputCB) );
    viewbut->attach( rightTo, nmfld_ );
    viewbut->setSensitive( false );
    outputviewbuts.setParam( this, viewbut );
*/

    uiUnitSel::Setup uussu( PropertyRef::Other,
			    tr("New log's unit of measure") );
    uussu.withnone( true );
    outunfld_ = new uiUnitSel( this, uussu );
    outunfld_->attach( alignedBelow, lcb );

    postFinalise().notify( formsetcb );
    if ( rockphysmode )
	afterPopup.notify( rockphyscb );
}


Notifier<uiWellLogCalc>& uiWellLogCalc::logschanged()
{
    return *hp_logschanged.getParam( this );
}


bool uiWellLogCalc::updateWells( const TypeSet<MultiID>& wellids )
{
    auto& wids = const_cast<TypeSet<MultiID>& >( this->wellids_ );
    wids = wellids;
    if ( wellids_.isEmpty() )
    {
	uiMSG().error(tr( "No wells.\nPlease import or create a well first.") );
	return false;
    }
    setTitleText( tr("%1").arg(getDlgTitle(wellids_)) );
    getAllLogs();
    if ( superwls_.isEmpty() || lognms_.isEmpty() )
    {
	uiMSG().error( tr("Selected wells have no logs.\n"
			   "Please import at least one.") );
	return false;
    }
    return true;
}


void uiWellLogCalc::getAllLogs()
{
    for ( int idx=0; idx<wellids_.size(); idx++ )
    {
	const MultiID wmid = wellids_[idx];
	RefMan<Well::Data> wd = new Well::Data;
	PtrMan<Well::Reader> wrdr;
	BufferStringSet nms;
	if ( Well::MGR().isLoaded(wmid ) )
	{
	    wd = Well::MGR().get( wmid );
	    if ( !wd ) continue;
	    wd->logs().getNames( nms, false );
	}
	else
	{
	    wrdr = new Well::Reader( wmid, *wd );
	    wrdr->getLogInfo( nms );
	}

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
	    if ( wrdr && !wrdr->getLogs() ) continue;
	    for ( int ilog=0; ilog<wd->logs().size(); ilog++ )
	    {
		const Well::Log& wl = wd->logs().getLog( ilog );
		if ( !superwls_.getLog(wl.name()) )
		    superwls_.add( new Well::Log(wl) );
	    }
	}
    }
}


uiWellLogCalc::~uiWellLogCalc()
{
    outputviewbuts.removeParam( this );
    hp_logschanged.removeAndDeleteParam( this );
    delete &form_;
    delete &superwls_;
}


bool uiWellLogCalc::useForm( const TypeSet<PropertyRef::StdType>* inputtypes )
{
    if ( !formfld_ )
	return false;

    if ( inputtypes )
    {
	if ( !formfld_->useForm(inputtypes) )
	    return false;

	for ( int idx=0; idx<inputtypes->size(); idx++ )
	{
	    TypeSet<int> propidxs = superwls_.getSuitable( (*inputtypes)[idx] );
	    BufferStringSet nms;
	    for ( int ilog=0; ilog<propidxs.size(); ilog++ )
		nms.add( superwls_.getLog(propidxs[ilog]).name() );
	    formfld_->setNonSpecInputs( nms, idx );
	}
    }
    else
    {
	BufferStringSet nms;
	superwls_.getNames( nms, false );
	formfld_->setNonSpecInputs( nms, -1 );
    }

    const UnitOfMeasure* formun = formfld_->getUnit();
    const PropertyRef::StdType prevtyp = outunfld_->propType();
    const PropertyRef::StdType newtyp = formun ? formun->propType()
					       : PropertyRef::Other;
    if ( prevtyp != newtyp )
    {
	outunfld_->setPropType( newtyp );
	outunfld_->setUnit( UoMR().getInternalFor(newtyp) );
    }

    for ( int iinp=0; iinp<form_.nrInputs(); iinp++ )
	setUnits4Log( iinp );

    return true;
}


class uiWellLogCalcRockPhys : public uiDialog
{ mODTextTranslationClass(uiWellLogCalcRockPhys);
public:

uiWellLogCalcRockPhys( uiParent* p )
    : uiDialog(p, uiDialog::Setup(uiStrings::sRockPhy(),
				  tr("Use a rock physics formula"),
				  mODHelpKey(mWellLogCalcRockPhysHelpID) ))
{ formgrp_ = new uiRockPhysForm( this ); }

bool acceptOK( CallBacker* )
{
    bool rv = formgrp_->isOK();
    if ( !rv ) uiMSG().error( mToUiStringTodo(formgrp_->errMsg()) );
    return rv;
}

bool getFormulaInfo( Math::Formula& form,
		     TypeSet<PropertyRef::StdType>& varstypes ) const
{ return formgrp_->getFormulaInfo(form,&varstypes); }

    uiRockPhysForm*	formgrp_;

};


void uiWellLogCalc::rockPhysReq( CallBacker* )
{
    uiWellLogCalcRockPhys dlg( this );
    TypeSet<PropertyRef::StdType> inputtypes;
    if ( !dlg.go() || !dlg.getFormulaInfo(form_,inputtypes) )
	return;

    useForm( &inputtypes );

    const UnitOfMeasure* formun = form_.outputUnit();
    if ( formun )
	outunfld_->setPropType( formun->propType() );
}


void uiWellLogCalc::feetSel( CallBacker* )
{
    zsampintv_ = srfld_->getFValue();
    if ( !mIsUdf(zsampintv_) )
    {
	zsampintv_ *= ftbox_->isChecked() ? mToFeetFactorF : mFromFeetFactorF;
	srfld_->setValue( zsampintv_ );
    }
}


void uiWellLogCalc::formSet( CallBacker* )
{
    useForm();
}


void uiWellLogCalc::formUnitSel( CallBacker* )
{
    const UnitOfMeasure* formun = formfld_->getUnit();
    if ( !formun )
	outunfld_->setPropType( PropertyRef::Other );
    else
	outunfld_->setUnit( formun );
}


Well::Log* uiWellLogCalc::getLog4InpIdx( Well::LogSet& wls, int varnr )
{
    return wls.getLog( formfld_->getInput(varnr) );
}


void uiWellLogCalc::setUnits4Log( int inpidx )
{
    if ( form_.isSpec(inpidx) || form_.isConst(inpidx) )
	return;

    const Well::Log* wl = getLog4InpIdx( superwls_, inpidx );
    if ( !wl )
	{ pErrMsg("Huh"); return; }

    uiMathExpressionVariable* inpfld = formfld_->inpFld( inpidx );
    inpfld->setSelUnit( wl->unitOfMeasure() );
}


void uiWellLogCalc::fillSRFld( int inpidx )
{
    float sr = srfld_->getFValue();
    if ( !mIsUdf(sr) )
	return;

    const Well::Log* wl = getLog4InpIdx( superwls_, inpidx );
    if ( wl && !wl->isEmpty() )
	sr = wl->dahStep( false );
    if ( !mIsUdf(sr) )
    {
	if ( ftbox_->isChecked() )
	    sr *= mToFeetFactorF;
	srfld_->setValue( sr );
    }
}


void uiWellLogCalc::inpSel( CallBacker* )
{
    const int inpidx = formfld_->inpSelNotifNr();
    if ( inpidx >= form_.nrInputs() )
	return;

    fillSRFld( inpidx );
    setUnits4Log( inpidx );
}


void uiWellLogCalc::vwLog( CallBacker* cb )
{
    const int inpnr = formfld_->vwLogInpNr( cb );
    if ( inpnr < 0 ) return;
    const Well::Log* wl = getLog4InpIdx( superwls_, inpnr );
    if ( !wl ) return;

    uiWellLogDisplay::Setup wldsu;
    wldsu.nrmarkerchars( 10 );
    uiWellLogDispDlg* dlg = new uiWellLogDispDlg( this, wldsu, true );
    dlg->setLog( wl, true );
    dlg->setDeleteOnClose( true );
    dlg->show();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

#define mErrContinue(s) { deleteLog(inpdatas); uiMSG().error(s); continue; }

void uiWellLogCalc::deleteLog( TypeSet<InpData>& inpdatas )
{
    for ( int idx=0; idx<inpdatas.size(); idx++ )
	deleteAndZeroPtr( inpdatas[idx].wl_ );
}


bool uiWellLogCalc::acceptOK( CallBacker* )
{
    if ( !formfld_ )
	return true;

    if ( !formfld_->updateForm() )
	return false;

    const BufferString newnm = nmfld_ ? nmfld_->text() : "";
    if ( newnm.isEmpty() )
	mErrRet(tr("Please provide a name for the new log"))
    if ( lognms_.isPresent(newnm) || superwls_.getLog(newnm) )
    {
	const bool ret = uiMSG().askOverwrite(
			tr("A log with name '%1' already exists."
			"\nDo you want to overwrite it?").arg(newnm));
	if ( !ret )
	    return false;
    }

    zsampintv_ = srfld_->getFValue();
    if ( mIsUdf(zsampintv_) )
	mErrRet(tr("Please provide the Z sample rate for the  output log"))
    if ( ftbox_->isChecked() )
	zsampintv_ *= mFromFeetFactorF;

    bool successfulonce = false;
    for ( int iwell=0; iwell<wellids_.size(); iwell++ )
    {
	TypeSet<InpData> inpdatas;
	const MultiID wmid = wellids_[iwell];
	RefMan<Well::Data> wd = Well::MGR().get( wmid );
	if ( !wd )
	    mErrContinue( tr("%1").arg(Well::MGR().errMsg()) )

	Well::LogSet& wls = wd->logs();
	// TODO: Change to an ObjectSet. Can not do proper memory management
	// with a TypeSet. Hence the addition of deleteLog.
	if ( !getInpDatas(wls,inpdatas) )
	{
	    deleteLog( inpdatas );
	    continue;
	}

	auto* newwl = new Well::Log( newnm );
	if ( !calcLog(*newwl,inpdatas,wd->track(),wd->d2TModel()) )
	    mErrContinue( tr("Cannot compute log for %1").arg(wd->name()))

	const UnitOfMeasure* outun = outunfld_->getUnit();
	if ( outun )
	{
	    const UnitOfMeasure* logun = form_.outputUnit();
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

	if ( !Well::MGR().writeAndRegister(wmid,*newwl) )
	    mErrContinue( tr(Well::MGR().errMsg()) );

	successfulonce = true;
	deleteLog( inpdatas );
    }

    if ( !successfulonce )
	return false;

    uiMSG().message( tr("Successfully added this log") );
    havenew_ = true;
    logschanged().trigger();
    return false;
}


bool uiWellLogCalc::getInpDatas( Well::LogSet& wls,
				TypeSet<uiWellLogCalc::InpData>& inpdatas )
{
    for ( int iinp=0; iinp<form_.nrInputs(); iinp++ )
    {
	if ( form_.isConst(iinp) )
	{
	    InpData inpd; inpd.isconst_ = true;
	    inpd.constval_ = (float)form_.getConstVal( iinp );
	    if ( mIsUdf(inpd.constval_) )
		mErrRet(tr("Please enter a value for %1")
		      .arg(form_.variableName(iinp)))
	    inpdatas += inpd;
	    continue;
	}

	const int specidx = form_.specIdx( iinp );
	const TypeSet<int>& reqshifts = form_.getShifts( iinp );
	for ( int ishft=0; ishft<reqshifts.size(); ishft++ )
	{
	    InpData inpd;
	    inpd.shift_ = reqshifts[ishft];
	    if ( specidx < 0 )
	    {
		inpd.wl_ = getInpLog( wls, iinp, ishft==0 );
		if ( !inpd.wl_ )
		    mErrRet(tr("%1: empty log").arg(toUiString(
							 form_.inputDef(iinp))))
	    }

	    inpd.specidx_ = specidx;
	    inpdatas += inpd;
	}
    }

    return true;
}


Well::Log* uiWellLogCalc::getInpLog( Well::LogSet& wls, int inpidx,
				     bool convtosi )
{
    Well::Log* inplog = getLog4InpIdx( wls, inpidx );
    if ( !inplog || inplog->isEmpty() )
	return 0;

    Well::Log* ret = new Well::Log( *inplog );
    if ( convtosi )
    {
	const char* logunitnm = ret->unitMeasLabel();
	const UnitOfMeasure* logun = UnitOfMeasure::getGuessed( logunitnm );
	if ( logun )
	{
	    float* valarr = ret->valArr();
	    for ( int idx=0; idx<ret->size(); idx++ )
		valarr[idx] = logun->getSIValue( valarr[idx] );
	}
    }

    return ret;
}


static void selectInpVals( const TypeSet<float>& noudfinpvals,
			int interppol, TypeSet<float>& inpvals )
{
    const int sz = inpvals.size();
    if ( sz == 0 || interppol == mInterpNone )
	return;

    int nrudf = 0;
    for ( int idx=0; idx<sz; idx++ )
	if ( mIsUdf(inpvals[idx]) )
	    nrudf++;
    if ( nrudf == 0 )
	return;

    if ( (interppol == mInterpAll)
      || (interppol == mInterpMax1 && nrudf < 2)
      || (interppol == mInterpNotAll && nrudf != sz) )
	inpvals = noudfinpvals;
}


bool uiWellLogCalc::calcLog( Well::Log& wlout,
			     const TypeSet<uiWellLogCalc::InpData>& inpdatas,
			     Well::Track& track, Well::D2TModel* d2t )
{
    form_.startNewSeries();

    Interval<float> dahrg( mUdf(float), mUdf(float) );
    for ( int iinp=0; iinp<inpdatas.size(); iinp++ )
    {
	if ( !inpdatas[iinp].wl_ )
	    continue;

	if ( dahrg.isUdf() )
	    dahrg = inpdatas[iinp].wl_->dahRange();
	else
	    dahrg.include( inpdatas[iinp].wl_->dahRange(), false );
    }
    if ( dahrg.isUdf() )
	dahrg = track.dahRange();

    StepInterval<float> samprg( dahrg.start, dahrg.stop, zsampintv_ );
    const int nrsamps = samprg.nrSteps() + 1;
    const int interppol = interppolfld_->currentItem();
    TypeSet<float> inpvals( inpdatas.size(), 0 );
    TypeSet<float> noudfinpvals( inpdatas.size(), 0 );
    for ( int rgidx=0; rgidx<nrsamps; rgidx++ )
    {
	const float dah = samprg.atIndex( rgidx );
	inpvals.setAll( mUdf(float) );
	noudfinpvals.setAll( mUdf(float) );
	for ( int iinp=0; iinp<inpdatas.size(); iinp++ )
	{
	    const uiWellLogCalc::InpData& inpd = inpdatas[iinp];
	    const float curdah = dah + samprg.step * inpd.shift_;
	    if ( inpd.wl_ )
	    {
		const float val = inpd.wl_->getValue( curdah, false );
		inpvals[iinp] = val;
		noudfinpvals[iinp] = !mIsUdf(val) ? val
				   : inpd.wl_->getValue( curdah, true );
	    }
	    else if ( inpd.isconst_ )
		inpvals[iinp] = noudfinpvals[iinp] = inpd.constval_;
	    else
	    {
		float val = mUdf(float);
		if ( inpd.specidx_ == mMDIdx )
		    val = curdah;
		else if ( inpd.specidx_ == mDZIdx )
		    val = samprg.step;
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

		inpvals[iinp] = noudfinpvals[iinp] = val;
	    }
	}

	float formval = mUdf(float);
	selectInpVals( noudfinpvals, interppol, inpvals );
	if ( inpvals.isEmpty() )
	    return false;;

	formval = form_.getValue( inpvals.arr(), false );
	wlout.addValue( dah, formval );
    }

    wlout.removeTopBottomUdfs();
    return true;
}


void uiWellLogCalc::setOutputLogName( const char* nm )
{ if ( nmfld_ ) nmfld_->setText( nm ); }


const char* uiWellLogCalc::getOutputLogName() const
{ return nmfld_ ? nmfld_->text() : 0; }


void uiWellLogCalc::viewOutputCB( CallBacker* )
{
// TODO: Implement viewing of output logs
}

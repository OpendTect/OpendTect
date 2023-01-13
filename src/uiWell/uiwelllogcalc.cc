/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelllogcalc.h"

#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimathexpressionvariable.h"
#include "uimathformula.h"
#include "uimnemonicsel.h"
#include "uimsg.h"
#include "uirockphysform.h"
#include "uiseparator.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"
#include "uiwelllogdisplay.h"

#include "ioman.h"
#include "mathformula.h"
#include "mathspecvars.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "ptrman.h"
#include "survinfo.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellreader.h"
#include "welltrack.h"

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

    const Mnemonic* distmn = &Mnemonic::distance();
    svs.add( "MD", "Depth Along Hole", true, distmn );
    svs.add( "TVD", "Z coordinate", true, distmn );
    svs.add( "TVDSS", "TVD below SS", true, distmn );
    svs.add( "TVDSD", "TVD below SD", true, distmn );
    svs.add( "DZ", "Delta Z", true, distmn );
    if ( SI().zIsTime() )
    {
	svs.add( "TWT", "Two-way traveltime", true, &Mnemonic::defTime() );
	svs.add( "VINT", "Interval velocity", true, &Mnemonic::defVEL() );
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


uiWellLogCalc::uiWellLogCalc( uiParent* p, const TypeSet<MultiID>& wllids,
			      bool rockphysmode )
    : uiDialog(p,uiDialog::Setup(tr("Calculate New Logs"),
				 mToUiStringTodo(getDlgTitle(wllids)),
				 mODHelpKey(mWellLogCalcHelpID) ))
    , logschanged(this)
    , superwls_(*new Well::LogSet)
    , form_(*new Math::Formula(true,getSpecVars()))
    , wellids_(wllids)
{
    if ( wellids_.isEmpty() )
    {
	new uiLabel( this, tr("No wells.\nPlease import"
			      " or create a well first.") );
	setCtrlStyle( CloseOnly );
	return;
    }

    MouseCursorChanger mcc( MouseCursor::Wait );
    getAllLogs();

    setOkCancelText( uiStrings::sCalculate(), uiStrings::sClose() );

    uiMathFormula::Setup mfsu( tr("Formula (like 'den / son')") );
    mfsu.stortype( "Log calculation" );
    formfld_ = new uiMathFormula( this, form_, mfsu );
    formfld_->addInpViewIcon( "view_log", "Display this log",
			      mCB(this,uiWellLogCalc,vwLog) );
    formfld_->setNonSpecInputs( lognms_, -1, &mnsel_ );
    mAttachCB( formfld_->inpSet, uiWellLogCalc::inpSel );
    mAttachCB( formfld_->formMnSet, uiWellLogCalc::formMnSet );
    mAttachCB( formfld_->formUnitSet, uiWellLogCalc::formUnitSel );
    const CallBack rockphyscb( mCB(this,uiWellLogCalc,rockPhysReq) );
    uiToolButtonSetup tbsu( "rockphys", tr("Choose rockphysics formula"),
			    rockphyscb, uiStrings::sRockPhy() );
    formfld_->addButton( tbsu );

    auto* sep = new uiSeparator( this, "sep" );
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

    viewlogbut_ = new uiToolButton( this, "view_log",
	tr("View output log"), mCB(this,uiWellLogCalc,viewOutputCB) );
    viewlogbut_->attach( rightTo, nmfld_ );
    viewlogbut_->setSensitive( false );

    uiUnitSel::Setup uussu( Mnemonic::Other,
			    tr("New log's unit of measure") );
    uussu.mode( uiUnitSel::Setup::SymbolsOnly ).variableszpol(true);
    outunfld_ = new uiUnitSel( this, uussu );
    outunfld_->attach( alignedBelow, lcb );
    outunfld_->attach( ensureRightOf, viewlogbut_ );

    if ( rockphysmode )
	afterPopup.notify( rockphyscb );
}


uiWellLogCalc::~uiWellLogCalc()
{
    detachAllNotifiers();
    delete &form_;
    delete &superwls_;
}


bool uiWellLogCalc::updateWells( const TypeSet<MultiID>& wellids )
{
    wellids_ = wellids;
    if ( wellids_.isEmpty() )
    {
	uiMSG().error(tr( "No wells.\nPlease import or create a well first.") );
	return false;
    }
    setTitleText( tr("%1").arg(getDlgTitle(wellids_)) );
    getAllLogs();
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
	    wd->logs().getNames( nms );
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
	    if ( wrdr && !wrdr->getLogs() )
		continue;

	    for ( int ilog=0; ilog<wd->logs().size(); ilog++ )
	    {
		const Well::Log& wl = wd->logs().getLog( ilog );
		if ( !superwls_.getLog(wl.name().buf()) )
		    superwls_.add( new Well::Log(wl) );
	    }
	}
    }

    mnsel_.setEmpty();
    for ( const auto* lognm : lognms_ )
    {
	const Well::Log* wl = superwls_.getLog( lognm->str() );
	if ( wl )
	    mnsel_.add( wl->mnemonic() );
    }

    if ( mnsel_.size() < lognms_.size() )
	{ pErrMsg("Unexpected error"); }
}


class uiWellLogCalcRockPhys : public uiDialog
{ mODTextTranslationClass(uiWellLogCalcRockPhys);
public:

uiWellLogCalcRockPhys( uiParent* p )
    : uiDialog(p, uiDialog::Setup(uiStrings::sRockPhy(),
				  tr("Use a rock physics formula"),
				  mODHelpKey(mWellLogCalcRockPhysHelpID) ))
{
    formgrp_ = new uiRockPhysForm( this );
}

bool acceptOK( CallBacker* ) override
{
    const uiRetVal uirv = formgrp_->isOK();
    if ( !uirv.isOK() )
	uiMSG().error( uirv );

    return uirv.isOK();
}

bool getFormulaInfo( Math::Formula& form ) const
{
    return formgrp_->getFormulaInfo( form );
}

    uiRockPhysForm*	formgrp_;

}; // class uiWellLogCalcRockPhys


void uiWellLogCalc::rockPhysReq( CallBacker* )
{
    uiWellLogCalcRockPhys dlg( this );
    if ( !dlg.go() || !dlg.getFormulaInfo(form_) )
	return;

    formfld_->setFixedFormUnits( true );
    formfld_->useForm();
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


Well::Log* uiWellLogCalc::getLog4InpIdx( Well::LogSet& wls, const char* lognm )
{
    return wls.getLog( lognm );
}


void uiWellLogCalc::setUnits4Log( const char* lognm,
				  uiMathExpressionVariable& inpfld )
{
    const Well::Log* wl = getLog4InpIdx( superwls_, lognm );
    if ( !wl )
	{ pErrMsg("Huh"); return; }

    inpfld.setSelUnit( wl->unitOfMeasure() );
}


void uiWellLogCalc::fillSRFld( const char* lognm )
{
    float sr = srfld_->getFValue();
    if ( !mIsUdf(sr) )
	return;

    const Well::Log* wl = getLog4InpIdx( superwls_, lognm );
    if ( wl && !wl->isEmpty() )
	sr = wl->dahStep( false );
    if ( !mIsUdf(sr) )
    {
	if ( ftbox_->isChecked() )
	    sr *= mToFeetFactorF;
	srfld_->setValue( sr );
    }
}


void uiWellLogCalc::inpSel( CallBacker* cb )
{
    mDynamicCastGet(uiMathExpressionVariable*,inpfld,cb);
    if ( !inpfld || !inpfld->isActive() ||
	  inpfld->isConst() || inpfld->isSpec() )
	return;

    const BufferString inpnm( inpfld->getInput() );

    fillSRFld( inpnm.buf() );
    setUnits4Log( inpnm.buf(), *inpfld );
}


void uiWellLogCalc::formMnSet( CallBacker* cb )
{
    mDynamicCastGet(uiMnemonicsSel*,uimnselfld,cb)
    if ( !uimnselfld )
	return;

    const Mnemonic* mn = uimnselfld->mnemonic();
    if ( !mn )
	mn = &Mnemonic::undef();

    const UnitOfMeasure* prevoutuom = outunfld_->getUnit();
    const UnitOfMeasure* mnunit = mn->unit();
    outunfld_->setPropType( mn->stdType() );
    if ( (prevoutuom && mnunit && !prevoutuom->isCompatibleWith(*mnunit)) ||
	  !uimnselfld->sensitive() )
	outunfld_->setUnit( mnunit );
}


void uiWellLogCalc::formUnitSel( CallBacker* cb )
{
    mDynamicCastGet(uiUnitSel*,unitfld,cb);
    if ( !unitfld )
	return;

    //TODO: Record and use last user preference?
}


void uiWellLogCalc::vwLog( CallBacker* cb )
{
    const int inpnr = formfld_->vwLogInpNr( cb );
    if ( inpnr < 0 )
	return;

    const Well::Log* wl = getLog4InpIdx( superwls_, formfld_->getInput(inpnr) );
    if ( !wl )
	return;

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
	deleteAndNullPtr( inpdatas[idx].wl_ );
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
    if ( lognms_.isPresent(newnm) || superwls_.getLog(newnm.buf()) )
    {
	const bool ret = uiMSG().askOverwrite(
			tr("A log with this name already exists."
			"\nDo you want to overwrite it?"));
	if ( !ret )
	    return false;
    }

    zsampintv_ = srfld_->getFValue();
    if ( mIsUdf(zsampintv_) )
	mErrRet(tr("Please provide the Z sample rate for the  output log"))
    if ( ftbox_->isChecked() )
	zsampintv_ *= mFromFeetFactorF;

    const Mnemonic* outmn = form_.outputMnemonic();
    const UnitOfMeasure* outun = outunfld_->getUnit();
    form_.setOutputValUnit( outun );

    bool successfulonce = false;
    uiRetVal errormsg;
    uiString errorstr;
    for ( int iwell=0; iwell<wellids_.size(); iwell++ )
    {
	TypeSet<InpData> inpdatas;
	const MultiID wmid = wellids_[iwell];
	RefMan<Well::Data> wd = Well::MGR().get( wmid );
	if ( !wd )
	{
	    deleteLog( inpdatas );
	    errormsg.add( tr("%1").arg(Well::MGR().errMsg()) );
	    continue;
	}

	Well::LogSet& wls = wd->logs();
	// TODO: Change to an ObjectSet. Can not do proper memory management
	// with a TypeSet. Hence the addition of deleteLog.
	if ( !getInpDatas(wls,inpdatas,errorstr) )
	{
	    errormsg.add( tr("%1: %2").arg(wd->name()).arg(errorstr) );
	    deleteLog( inpdatas );
	    continue;
	}

	PtrMan<Well::Log> newwl = new Well::Log( newnm );
	if ( !calcLog(*newwl,inpdatas,wd->track(),wd->d2TModel()) )
	{
	    deleteLog( inpdatas );
	    errormsg.add( tr("Cannot compute log for %1").arg(wd->name()) );
	    continue;
	}

	if ( outmn && !outmn->isUdf() )
	    newwl->setMnemonic( *outmn );

	newwl->setUnitOfMeasure( outun );
	if ( !Well::MGR().writeAndRegister(wmid,newwl) )
	{
	    deleteLog( inpdatas );
	    errormsg.add( tr(Well::MGR().errMsg()) );
	    continue;
	}

	successfulonce = true;
	deleteLog( inpdatas );
    }

    if ( !successfulonce )
    {
	if ( errormsg.isError() )
	    uiMSG().errorWithDetails( errormsg.messages(),
				      tr("Adding new log failed") );
	return false;
    }

    if ( errormsg.isError() )
	uiMSG().errorWithDetails( errormsg.messages(),
		tr("Adding new log failed for some of the selected wells") );
    else
	uiMSG().message( tr("Successfully added this log") );

    havenew_ = true;
    viewlogbut_->setSensitive( true );
    logschanged.trigger();
    return false;
}


bool uiWellLogCalc::getInpDatas( Well::LogSet& wls,
				 TypeSet<uiWellLogCalc::InpData>& inpdatas,
				 uiString& errorstr )
{
    for ( int iinp=0; iinp<form_.nrInputs(); iinp++ )
    {
	if ( form_.isConst(iinp) )
	{
	    InpData inpd; inpd.isconst_ = true;
	    inpd.constval_ = form_.getConstVal( iinp );
	    if ( mIsUdf(inpd.constval_) )
	    {
		errorstr = tr("Please enter a value for %1")
						.arg(form_.variableName(iinp));
		return false;
	    }
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
		inpd.wl_ = getInpLog( wls, iinp );
		if ( !inpd.wl_ )
		{
		    errorstr = tr("%1: empty log").arg(toUiString(
						     form_.inputDef(iinp)));
		    return false;
		}
	    }

	    inpd.specidx_ = specidx;
	    inpdatas += inpd;
	}
    }

    return true;
}


Well::Log* uiWellLogCalc::getInpLog( Well::LogSet& wls, int inpidx )
{
    Well::Log* inplog = getLog4InpIdx( wls, formfld_->getInput(inpidx) );
    if ( !inplog || inplog->isEmpty() )
	return nullptr;

    return new Well::Log( *inplog );
}


static void selectInpVals( const TypeSet<double>& noudfinpvals,
			int interppol, TypeSet<double>& inpvals )
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

    const StepInterval<float> samprg( dahrg.start, dahrg.stop, zsampintv_ );
    const int nrsamps = samprg.nrSteps() + 1;
    const int interppol = interppolfld_->currentItem();
    TypeSet<double> inpvals( inpdatas.size(), 0. );
    TypeSet<double> noudfinpvals( inpdatas.size(), 0. );
    for ( int rgidx=0; rgidx<nrsamps; rgidx++ )
    {
	const float dah = samprg.atIndex( rgidx );
	inpvals.setAll( mUdf(double) );
	noudfinpvals.setAll( mUdf(double) );
	for ( int iinp=0; iinp<inpdatas.size(); iinp++ )
	{
	    const uiWellLogCalc::InpData& inpd = inpdatas[iinp];
	    const float curdah = dah + samprg.step * inpd.shift_;
	    if ( inpd.wl_ )
	    {
		form_.setInputValUnit( iinp, inpd.wl_->unitOfMeasure() );
		const double val = inpd.wl_->getValue( curdah, false );
		inpvals[iinp] = val;
		noudfinpvals[iinp] = !mIsUdf(val) ? val
				   : inpd.wl_->getValue( curdah, true );
	    }
	    else if ( inpd.isconst_ )
		inpvals[iinp] = noudfinpvals[iinp] = inpd.constval_;
	    else
	    {
		double val = mUdf(double);
		form_.setInputValUnit( iinp,
				       UnitOfMeasure::surveyDefDepthUnit() );
		if ( inpd.specidx_ == mMDIdx )
		    val = curdah;
		else if ( inpd.specidx_ == mDZIdx )
		    val = samprg.step;
		else if ( inpd.specidx_ == mTVDSSIdx ||
			  inpd.specidx_ == mTVDIdx ||
			  inpd.specidx_ == mTVDSDIdx )
		{
		    val = track.getPos( curdah ).z;
		    if ( inpd.specidx_ == mTVDIdx && !mIsUdf(val) )
			val += track.getKbElev();
		    else if ( inpd.specidx_ == mTVDSDIdx  && !mIsUdf(val) )
			val += SI().seismicReferenceDatum();
		}
		else if ( inpd.specidx_ == mTWTIdx && d2t )
		{
		    val = d2t->getTime( curdah, track );
		    form_.setInputValUnit( iinp,
					   UnitOfMeasure::surveyDefZUnit() );

		}
		else if ( inpd.specidx_ == mVelIdx && d2t )
		{
		    val = d2t->getVelocityForDah( curdah, track );
		    form_.setInputValUnit( iinp,
					   UnitOfMeasure::surveyDefVelUnit() );
		}

		inpvals[iinp] = noudfinpvals[iinp] = val;
	    }
	}

	selectInpVals( noudfinpvals, interppol, inpvals );
	if ( inpvals.isEmpty() )
	    return false;;

	const double formval = form_.getValue( inpvals.arr() );
	wlout.addValue( dah, float(formval) );
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
    const char* lognm = getOutputLogName();
    const Well::Log* wl = superwls_.getLog( lognm );
    if ( !wl )
    {
	uiMSG().error( tr("Can not find log '%1' for this well.").arg(lognm) );
	return;
    }

    uiWellLogDisplay::Setup wldsu;
    wldsu.nrmarkerchars( 10 );
    uiWellLogDispDlg* dlg = new uiWellLogDispDlg( this, wldsu, true );
    dlg->setLog( wl, true );
    dlg->setDeleteOnClose( true );
    dlg->show();
}

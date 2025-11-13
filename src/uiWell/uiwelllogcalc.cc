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
#include "uimathexpression.h"
#include "uimathexpressionvariable.h"
#include "uimathformula.h"
#include "uimsg.h"
#include "uirockphysform.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"
#include "uiwelllogdisplay.h"

#include "ioman.h"
#include "mathformula.h"
#include "mathspecvars.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "survinfo.h"
#include "timer.h"
#include "welld2tmodel.h"
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


static uiString getDlgTitle( const TypeSet<MultiID>& wllids )
{
    const int sz = wllids.size();
    if ( sz < 1 )
	return od_static_tr( "getDlgTitle", "No wells selected" );

    uiString ret = od_static_tr("getDlgTitle","Calculate new logs for '%1'").
					    arg( IOM().nameOf(wllids[0]));
    BufferString nmsstr;
    for ( int idx=1; idx<sz; idx++ )
	nmsstr.add( " ,'").add(IOM().nameOf(wllids[idx])).add("'");

    if ( !nmsstr.isEmpty() )
    {
	nmsstr = getLimitedDisplayString( nmsstr.buf(), 80, true );
	ret.appendPhrase( toUiString(nmsstr), uiString::NoSep,
							uiString::OnSameLine );
    }

    return ret;
}


uiWellLogCalc::uiWellLogCalc( uiParent* p, const TypeSet<MultiID>& wllids,
			      bool rockphysmode )
    : uiDialog(p,Setup(tr("Calculate New Logs"),getDlgTitle(wllids),
		       mODHelpKey(mWellLogCalcHelpID)))
    , logschanged(this)
    , form_(*new Math::Formula(true,getSpecVars()))
    , wellids_(wllids)
    , rockphysmode_(rockphysmode)
    , timer_(*new Timer("Well log calc WD timer"))
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

    uiMathFormula::Setup mfsu( tr("Formula") );
    mfsu.stortype( "Log calculation" );
    formfld_ = new uiMathFormula( this, form_, mfsu );
    formfld_->exprFld()->setPlaceholderText( toUiString("density / sonic") );
    formfld_->addInpViewIcon( "view_log", tr("Display this log"),
			      mCB(this,uiWellLogCalc,vwLog) );
    formfld_->setNonSpecInputs( lognms_, -1, &mnsel_ );
    mAttachCB( formfld_->formMnSet, uiWellLogCalc::formMnSet );
    const CallBack rockphyscb( mCB(this,uiWellLogCalc,rockPhysReq) );
    uiToolButtonSetup tbsu( "rockphys", tr("Choose rockphysics formula"),
			    rockphyscb, uiStrings::sRockPhy() );
    formfld_->addButton( tbsu );

    auto* sep = new uiSeparator( this, "sep" );
    sep->attach( stretchedBelow, formfld_ );

    const float defsr = SI().depthsInFeet() ? 0.5f : 0.1524f;
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
    uussu.mode( uiUnitSel::Setup::SymbolsOnly ).variableszpol( true );
    outunfld_ = new uiUnitSel( this, uussu );
    outunfld_->attach( alignedBelow, lcb );
    outunfld_->attach( ensureRightOf, viewlogbut_ );

    mAttachCB( timer_.tick, uiWellLogCalc::releaseWDS );
    mAttachCB( afterPopup, uiWellLogCalc::afterPopupCB );
}


uiWellLogCalc::~uiWellLogCalc()
{
    detachAllNotifiers();
    delete &timer_;
    delete &form_;
}


void uiWellLogCalc::afterPopupCB( CallBacker* cb )
{
    resetTimer();
    if ( rockphysmode_ )
	rockPhysReq( cb );
}


void uiWellLogCalc::resetTimer()
{
    timer_.stop();
    timer_.start( 360000, true ); //One hour
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
    const Well::LoadReqs lreqs( Well::LogInfos );
    MultiWellReader rdr( wellids_, wds_, lreqs );
    uiTaskRunner uitr( this );
    if ( !uitr.execute(rdr) )
    {
	uiMSG().error( rdr.uiMessage() );
	return;
    }

    mnsel_.setEmpty();
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	ConstRefMan<Well::Data> wd = wds_[idx];
	if ( !wd )
	    continue;

	const Well::LogSet& logs = wd->logs();
	BufferStringSet lognms;
	logs.getNames( lognms );
	lognms_.add( lognms, false );
	for ( const auto* lognm : lognms )
	    mnsel_.add( logs.getMnemonicOfLog(lognm->buf()) );
    }

    if ( mnsel_.size() < lognms_.size() )
	{ pErrMsg("Unexpected error"); }
}


class uiWellLogCalcRockPhys : public uiDialog
{ mODTextTranslationClass(uiWellLogCalcRockPhys);
public:

uiWellLogCalcRockPhys( uiParent* p, Mnemonic::StdType typ )
    : uiDialog(p,Setup(uiStrings::sRockPhy(),
		       tr("Use a rock physics formula"),
		       mODHelpKey(mWellLogCalcRockPhysHelpID)))
{
    formgrp_ = typ == Mnemonic::Other ? new uiRockPhysForm( this )
				      : new uiRockPhysForm( this, typ );
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
    const Mnemonic* mn = form_.outputMnemonic();
    const Mnemonic::StdType typ = mn ? mn->stdType() : Mnemonic::Other;
    uiWellLogCalcRockPhys dlg( this, typ );
    if ( dlg.go() != uiDialog::Accepted )
	return;

    dlg.getFormulaInfo( form_ );
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


const Well::Log* uiWellLogCalc::getFirstLog4InpIdx( const char* lognm ) const
{
    const BufferStringSet lognms( lognm );
    const Well::LoadReqs lreqs( lognms );
    for ( int idx=0; idx<wds_.size(); idx++ )
    {
	ConstRefMan<Well::Data> wd = wds_[idx];
	if ( !wd )
	    continue;

	const Well::LogSet& logs = wd->logs();
	if ( !logs.isPresent(lognm) )
	    continue;

	if ( !Well::MGR().get(wd->multiID(),lreqs) )
	    continue;

	return logs.getLog( lognm );
    }

    return nullptr;
}


void uiWellLogCalc::formMnSet( CallBacker* cb )
{
    if ( !cb || !cb->isCapsule() )
	return;

    mCBCapsuleUnpack(const Mnemonic*,mn,cb);
    const bool isfixed = mn;
    if ( !mn )
	mn = &Mnemonic::undef();

    const UnitOfMeasure* prevoutuom = outunfld_->getUnit();
    const UnitOfMeasure* mnunit = mn->unit();
    outunfld_->setPropType( mn->stdType() );
    if ( (prevoutuom && mnunit && !prevoutuom->isCompatibleWith(*mnunit)) ||
	  isfixed )
	outunfld_->setUnit( mnunit );
    else if ( mn->isUdf() )
	outunfld_->setUnit( (const UnitOfMeasure*)nullptr );
}


void uiWellLogCalc::vwLog( CallBacker* cb )
{
    const int inpnr = formfld_->vwLogInpNr( cb );
    if ( inpnr < 0 )
	return;

    const Well::Log* log = getFirstLog4InpIdx( formfld_->getInput(inpnr) );
    if ( !log || !log->isLoaded() )
	return;

    uiWellLogDisplay::Setup wldsu;
    wldsu.nrmarkerchars( 10 );
    auto* dlg = new uiWellLogDispDlg( this, wldsu, true );
    dlg->setLog( log, true );
    dlg->setDeleteOnClose( true );
    dlg->show();
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mErrContinue(s) { uiMSG().error(s); continue; }

bool uiWellLogCalc::acceptOK( CallBacker* )
{
    if ( !formfld_ )
	return true;

    if ( !formfld_->updateForm() )
	return false;

    const BufferString newnm = nmfld_ ? nmfld_->text() : "";
    if ( newnm.isEmpty() )
	mErrRet(tr("Please provide a name for the new log"))
    if ( lognms_.isPresent(newnm.buf()) || getFirstLog4InpIdx(newnm.buf()) )
    {
	const bool ret = uiMSG().askOverwrite(
			tr("A log with this name already exists."
			"\nDo you want to overwrite it?") );
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
    for ( const auto& wmid : wellids_ )
    {
	ManagedObjectSet<InpData> inpdatas;
	Well::LoadReqs lreqs( Well::Trck );
	if ( !getInpDatas(wmid,inpdatas,lreqs,errorstr) )
	{
	    errormsg.add( errorstr );
	    continue;
	}

	ConstRefMan<Well::Data> wd = Well::MGR().get( wmid, lreqs );
	if ( !wd )
	{
	    errormsg.add( Well::MGR().errMsg() );
	    continue;
	}

	if ( !lreqs.logNames().isEmpty() )
	{
	    bool haserror = false;
	    for ( auto* inpdata : inpdatas )
	    {
		if ( inpdata->lognm_.isEmpty() )
		    continue;

		const Well::Log* inplog =
				wd->logs().getLog( inpdata->lognm_.str() );
		if ( !inplog || !inplog->isLoaded() )
		{
		    errormsg.add( tr("Cannot read input logs for well '%1'")
					.arg(wd->name()) );
		    haserror = true;
		    break;
		}

		inpdata->wl_ = inplog;
	    }

	    if ( haserror )
		continue;
	}

	PtrMan<Well::Log> newwl = new Well::Log( newnm );
	if ( !calcLog(inpdatas,wd->track(),wd->d2TModel(),*newwl.ptr()) )
	{
	    errormsg.add( tr("Cannot compute log for %1").arg(wd->name()) );
	    continue;
	}

	if ( outmn && !outmn->isUdf() )
	    newwl->setMnemonic( *outmn );

	newwl->setUnitOfMeasure( outun );
	if ( !Well::MGR().writeAndRegister(wmid,newwl) )
	{
	    errormsg.add( Well::MGR().errMsg() );
	    continue;
	}

	successfulonce = true;
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
    resetTimer();

    return false;
}


bool uiWellLogCalc::getInpDatas( const MultiID& wid,
				 ObjectSet<InpData>& inpdatas,
				 Well::LoadReqs& lreqs, uiString& errorstr )
{
    for ( int iinp=0; iinp<form_.nrInputs(); iinp++ )
    {
	if ( form_.isConst(iinp) )
	{
	    PtrMan<InpData> inpd = new InpData;
	    inpd->isconst_ = true;
	    inpd->constval_ = form_.getConstVal( iinp );
	    if ( mIsUdf(inpd->constval_) )
	    {
		errorstr = tr("Please enter a value for %1")
						.arg(form_.variableName(iinp));
		return false;
	    }

	    inpdatas.add( inpd.release() );
	    continue;
	}

	const int specidx = form_.specIdx( iinp );
	const TypeSet<int>& reqshifts = form_.getShifts( iinp );
	for ( int ishft=0; ishft<reqshifts.size(); ishft++ )
	{
	    PtrMan<InpData> inpd = new InpData;
	    inpd->shift_ = reqshifts[ishft];
	    if ( specidx < 0 )
	    {
		inpd->lognm_ = formfld_->getInput( iinp );
		lreqs.include( Well::LogInfos );
		lreqs.addLog( inpd->lognm_.buf() );
	    }
	    else if ( inpd->specidx_ == mTWTIdx || inpd->specidx_ == mVelIdx )
		lreqs.include( Well::D2T );

	    inpd->specidx_ = specidx;
	    inpdatas.add( inpd.release() );
	}
    }

    return true;
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


bool uiWellLogCalc::calcLog( const ObjectSet<InpData>& inpdatas,
			     const Well::Track& track,const Well::D2TModel* d2t,
			     Well::Log& wlout )
{
    form_.startNewSeries();

    Interval<float> dahrg = Interval<float>::udf();
    for ( const auto* inpd : inpdatas )
    {
	if ( !inpd->wl_ )
	    continue;

	if ( dahrg.isUdf() )
	    dahrg = inpd->wl_->dahRange();
	else
	    dahrg.include( inpd->wl_->dahRange(), false );
    }

    if ( dahrg.isUdf() )
	dahrg = track.dahRange();

    const ZSampling samprg( dahrg.start_, dahrg.stop_, zsampintv_ );
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
	    const InpData& inpd = *inpdatas.get( iinp );
	    const float curdah = dah + samprg.step_ * inpd.shift_;
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
		    val = samprg.step_;
		else if ( inpd.specidx_ == mTVDSSIdx ||
			  inpd.specidx_ == mTVDIdx ||
			  inpd.specidx_ == mTVDSDIdx )
		{
                    val = track.getPos( curdah ).z_;
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
{
    if ( nmfld_ )
	nmfld_->setText( nm );
}


const char* uiWellLogCalc::getOutputLogName() const
{
    return nmfld_ ? nmfld_->text() : nullptr;
}


void uiWellLogCalc::viewOutputCB( CallBacker* )
{
    const char* lognm = getOutputLogName();
    const Well::Log* log = getFirstLog4InpIdx( lognm );
    if ( !log )
    {
	uiMSG().error( tr("Can not find log '%1' for this well.").arg(lognm) );
	return;
    }

    uiWellLogDisplay::Setup wldsu;
    wldsu.nrmarkerchars( 10 );
    auto* dlg = new uiWellLogDispDlg( this, wldsu, true );
    dlg->setLog( log, true );
    dlg->setDeleteOnClose( true );
    dlg->show();
}


void uiWellLogCalc::releaseWDS( CallBacker* )
{
    /*After one hour of inactivity, this GUI releases its well data,
      but not before, to avoid re-reading the well data too frequently
      if nothing else locks it */

    wds_.setEmpty();
}


// uiWellLogCalc::InpData

uiWellLogCalc::InpData::InpData()
{}


uiWellLogCalc::InpData::~InpData()
{}

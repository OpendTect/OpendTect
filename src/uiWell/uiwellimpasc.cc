/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2003
________________________________________________________________________

-*/

#include "uiwellimpasc.h"

#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "od_istream.h"
#include "ptrman.h"
#include "separstr.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "welld2tmodel.h"
#include "wellimpasc.h"
#include "wellmanager.h"
#include "welltrack.h"
#include "wellinfo.h"

#include "uibutton.h"
#include "uid2tmodelgrp.h"
#include "uifilesel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiselsurvranges.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"
#include "uiwellsel.h"
#include "od_helpids.h"

#define mAdvOptStr() \
    toUiString("%1/%2").arg(uiStrings::sAdvanced()).arg(uiStrings::sOptional())


uiWellImportAsc::uiWellImportAsc( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrImport(uiStrings::sWellTrack()),
				mNoDlgTitle, mODHelpKey(mWellImportAscHelpID))
				.modal(false))
    , fd_(*Well::TrackAscIO::getDesc())
    , dirfd_(*Well::DirectionalAscIO::getDesc())
    , wd_(*new Well::Data)
    , zun_(UnitOfMeasure::surveyDefDepthUnit())
    , importReady(this)
{
    enableSaveButton( tr("Display after import") );
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiStringSet options;
    options.add( tr("Well Track file (XYZ)") )
	   .add( tr("Directional Well") )
	   .add( tr("Vertical Well") );
    tracksrcfld_ = new uiGenInput( this, tr("Input type"),
				StringListInpSpec(options) );
    tracksrcfld_->valuechanged.notify( mCB(this,uiWellImportAsc,haveTrckSel) );

    trckinpfld_ = new uiFileSel( this, tr("Well Track File"),
		   uiFileSel::Setup().withexamine(true) );
    trckinpfld_->newSelection.notify( mCB(this,uiWellImportAsc,inputChgd) );
    trckinpfld_->attach( alignedBelow, tracksrcfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
			mODHelpKey(mWellImportAscDataSelHelpID) );
    dataselfld_->attach( alignedBelow, trckinpfld_ );
    dataselfld_->descChanged.notify( mCB(this,uiWellImportAsc,trckFmtChg) );

    dirselfld_ = new uiTableImpDataSel( this, dirfd_,
			mODHelpKey(mWellImportAscDataSelHelpID) );
    dirselfld_->attach( alignedBelow, trckinpfld_ );
    dirselfld_->display( false );

    uiString coordunitslbl = uiStrings::sCoordinate().withSurvXYUnit();
    coordfld_ = new uiGenInput( this, coordunitslbl,
			PositionInpSpec(PositionInpSpec::Setup(true)) );
    coordfld_->attach( alignedBelow, dataselfld_ );

    const uiString zunit = UnitOfMeasure::surveyDefDepthUnitAnnot( true );
    uiString kblbl = Well::Info::sKBElev().withUnit( zunit );
    kbelevfld_ = new uiGenInput( this, kblbl, FloatInpSpec(0) );
    kbelevfld_->setWithCheck();
    kbelevfld_->setChecked( false );
    kbelevfld_->attach( alignedBelow, coordfld_ );

    uiString tdlbl = Well::Info::sTD().withUnit( zunit );
    tdfld_ = new uiGenInput( this, tdlbl, FloatInpSpec() );
    tdfld_->setWithCheck();
    tdfld_->setChecked( false );
    tdfld_->attach( alignedBelow, kbelevfld_ );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, tdfld_ );

    wd_.ref();
    float dispval = wd_.info().replacementVelocity();
    if ( !mIsUdf(dispval) )
	wd_.info().setReplacementVelocity( dispval );

    const bool zistime = SI().zIsTime();
    if ( zistime )
    {
	uiD2TModelGroup::Setup su; su.asksetcsmdl( true );
	d2tgrp_ = new uiD2TModelGroup( this, su );
	d2tgrp_->attach( alignedBelow, dataselfld_ );
	d2tgrp_->attach( ensureBelow, sep );
	sep = new uiSeparator( this, "H sep 2" );
	sep->attach( stretchedBelow, d2tgrp_ );
    }

    uiButton* but = new uiPushButton( this, mAdvOptStr(),
					mCB(this,uiWellImportAsc,doAdvOpt),
					false );
    but->attach( alignedBelow, zistime ? (uiObject*)d2tgrp_
				       : (uiObject*)dataselfld_ );
    but->attach( ensureBelow, sep );

    outfld_ = new uiWellSel( this, false, uiString::empty(), false );
    outfld_->attach( alignedBelow, but );

    postFinalise().notify( mCB(this,uiWellImportAsc,haveTrckSel) );
}


uiWellImportAsc::~uiWellImportAsc()
{
    delete &fd_;
    delete &dirfd_;
    wd_.unRef();
}


DBKey uiWellImportAsc::getWellID() const
{ return outfld_->key(); }


void uiWellImportAsc::haveTrckSel( CallBacker* )
{
    const int cursel = tracksrcfld_->getIntValue();
    const bool istrack = cursel==0;
    const bool isdir = cursel==1;
    const bool isvert = cursel==2;
    trckinpfld_->setSensitive( istrack || isdir );
    coordfld_->setSensitive( isdir || isvert );
    kbelevfld_->setChecked( isvert );
    tdfld_->setChecked( isvert );

    dataselfld_->display( !isdir );
    dataselfld_->setSensitive( istrack );
    dirselfld_->display( isdir );
}


void uiWellImportAsc::inputChgd( CallBacker* )
{
    File::Path fnmfp( trckinpfld_->fileName() );
    outfld_->setInputText( fnmfp.baseName() );
    kbelevfld_->setValue(0);
    tdfld_->setChecked( false );
    tdfld_->setValue( mUdf(float) );
}


void uiWellImportAsc::trckFmtChg( CallBacker* )
{
    const Table::FormatDesc& fd = dataselfld_->desc();
    if ( !fd.isGood() )
	return;

    bool havez = false;
    bool havemd = false;
    for ( int idx=0; idx<fd.bodyinfos_.size(); idx++ )
    {
	const Table::TargetInfo& ti = *fd.bodyinfos_[idx];
	if ( ti.name().startsWith("Z") && ti.selection_.isInFile(0) )
	    havez = true;

	if ( ti.hasName(sKey::MD()) && ti.selection_.isInFile(0) )
	    havemd = true;
    }

    if ( !havez && !havemd )
    {
	uiMSG().error( tr("The format you defined has neither Z nor MD."
			  "\nYou should define at least one."
			  "\nAs it is now, the track will not load.") );
    }

    kbelevfld_->setChecked( !havez || !havemd );
}



class uiWellImportAscOptDlg : public uiDialog
{ mODTextTranslationClass(uiWellImportAscOptDlg);
public:

uiWellImportAscOptDlg( uiWellImportAsc* p )
    : uiDialog(p,uiDialog::Setup(toUiString("%1: %2").arg(uiStrings::sImport())
						     .arg(mAdvOptStr()),
				 mNoDlgTitle,mODHelpKey(mWellImpPptDlgHelpID)))
    , uwia_(p)
    , zun_(UnitOfMeasure::surveyDefDepthUnit())
{
    const Well::Info& info = uwia_->wd_.info();

    PositionInpSpec::Setup possu( true );
    if ( !mIsZero(info.surfaceCoord().x_,0.1) )
	possu.coord_ = info.surfaceCoord();
    coordfld = new uiGenInput( this,
	tr("Surface Coordinate (if different from first "
	   "coordinate in track file)"),
	PositionInpSpec(possu).setName( "X", 0 ).setName( "Y", 1 ) );

    const bool zinfeet = SI().depthsInFeet();

    float dispval = info.replacementVelocity();
    if ( zinfeet && zun_ )
	dispval = zun_->userValue( info.replacementVelocity() );

    uiString lbl = Well::Info::sReplVel()
	    .withUnit( UnitOfMeasure::surveyDefVelUnitAnnot(true) );
    replvelfld = new uiGenInput( this, lbl, FloatInpSpec(dispval) );
    replvelfld->attach( alignedBelow, coordfld );

    dispval = info.groundElevation();
    if ( zinfeet && zun_ ) dispval = zun_->userValue( info.groundElevation() );
    lbl = Well::Info::sGroundElev()
	      .withUnit( UnitOfMeasure::surveyDefDepthUnitAnnot(true) );
    gdelevfld = new uiGenInput( this, lbl, FloatInpSpec(dispval) );
    gdelevfld->attach( alignedBelow, replvelfld );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, gdelevfld );

    idfld = new uiGenInput( this, Well::Info::sUwid(),
			    StringInpSpec(info.UWI()) );
    idfld->attach( alignedBelow, gdelevfld );
    idfld->attach( ensureBelow, horsep );

    operfld = new uiGenInput( this, Well::Info::sOper(),
			      StringInpSpec(info.wellOperator()) );
    operfld->attach( rightTo, idfld );

    statefld = new uiGenInput( this, Well::Info::sState(),
			       StringInpSpec(info.getState()) );
    statefld->attach( alignedBelow, idfld );

    countyfld = new uiGenInput( this, Well::Info::sCounty(),
				StringInpSpec(info.getCounty()) );
    countyfld->attach( rightTo, statefld );
}


bool acceptOK()
{
    Well::Info& info = uwia_->wd_.info();

    if ( *coordfld->text() )
	info.setSurfaceCoord( coordfld->getCoord() );

    if ( *replvelfld->text() )
    {
	const float replvel = replvelfld->getFValue();
	if ( !mIsUdf(replvel) && zun_ )
	    info.setReplacementVelocity( zun_->internalValue(replvel) );
    }

    if ( *gdelevfld->text() )
    {
	const float gdevel = gdelevfld->getFValue();
	if ( !mIsUdf(gdevel)  && zun_ )
	    info.setGroundElevation( zun_->internalValue(gdevel) );
    }

    info.setUWI( idfld->text() );
    info.setWellOperator( operfld->text() );
    info.setState( statefld->text() );
    info.setCounty( countyfld->text() );

    return true;
}

    const UnitOfMeasure* zun_;
    uiWellImportAsc*	uwia_;
    uiGenInput*		coordfld;
    uiGenInput*		elevfld;
    uiGenInput*		replvelfld;
    uiGenInput*		gdelevfld;
    uiGenInput*		idfld;
    uiGenInput*		operfld;
    uiGenInput*		statefld;
    uiGenInput*		countyfld;
    uiCheckBox*		zinftbox;
};


void uiWellImportAsc::doAdvOpt( CallBacker* )
{
    uiWellImportAscOptDlg dlg( this );
    dlg.go();
}


bool uiWellImportAsc::acceptOK()
{
    if ( !checkInpFlds() )
	return false;

    doWork();
    wd_.info().setSurfaceCoord( Coord(0,0) );
    wd_.info().setGroundElevation( mUdf(float) );
    uiString msg = tr("Well Track successfully imported."
		      "\n\nDo you want to import more Well Tracks?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


#define mErrRet(s) { if ( !(s).isEmpty() ) uiMSG().error(s); return false; }

bool uiWellImportAsc::doWork()
{
    wd_.setEmpty();
    wd_.info().setName( outfld_->getInput() );
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    float kbelev = kbelevfld_->getFValue();
    if ( mIsUdf(kbelev) )
	kbelev = 0;
    else if ( zun_ )
	kbelev = zun_->internalValue( kbelev );

    float td = tdfld_->getFValue();
    if ( !mIsUdf(td) && zun_ )
	td = zun_->internalValue( td ) ;

    const int cursel = tracksrcfld_->getIntValue();
    const bool istrack = cursel==0;
    const bool isdir = cursel==1;
    const bool isvert = cursel==2;

    BufferStringSet datasrcnms;
    if ( istrack || isdir )
    {
	const BufferString fnm( trckinpfld_->fileName() );
	datasrcnms.add( fnm );
	od_istream strm( fnm );
	if ( !strm.isOK() )
	    mErrRet( tr("Cannot open track file") )

	if ( !kbelevfld_->isChecked() )
	    kbelev = mUdf(float);

	if ( !tdfld_->isChecked() )
	    td = mUdf(float);

	if ( istrack )
	{
	    Well::TrackAscIO wellascio( fd_, strm );
	    if ( !wellascio.getData(wd_,kbelev,td) )
	    {
		uiString msg = tr("The track file cannot be loaded")
				.addMoreInfo( wellascio.errMsg(), true );
		mErrRet( msg );
	    }

	    if ( !wellascio.warnMsg().isEmpty() )
	    {
		uiString msg = tr("The track file loading issued a warning")
				.addMoreInfo( wellascio.warnMsg(), true );
		uiMSG().warning( msg );
	    }
	}
	else if ( isdir )
	{
	    Well::DirectionalAscIO dirascio( dirfd_, strm );
	    wd_.info().setSurfaceCoord( coordfld_->getCoord() );
	    if ( !dirascio.getData(wd_,kbelev) )
	    {
		uiString msg = tr( "The track file cannot be loaded:\n%1" )
				    .arg( dirascio.errMsg() );
		mErrRet( msg );
	    }
	}
    }
    else if ( isvert )
    {
	datasrcnms.add( "[Vertical]" );

	if ( mIsUdf(td) || td < 1e-6 )
	{
	    float survzstop = SI().zRange().stop;
	    if ( SI().zIsTime() )
		survzstop *= 2000;
	    td = survzstop - kbelev;
	}

	Interval<float> zrg( -kbelev, td-kbelev );
	const Coord c( coordfld_->getCoord() );
	wd_.track().addPoint( c, zrg.start, 0 );
	wd_.track().addPoint( c, zrg.stop, td );
    }

    if ( SI().zIsTime() )
    {
	const bool validd2t = d2tgrp_->getD2T( wd_, false );
	if ( !validd2t )
	{
	    uiStringSet msgs;

	    if ( !d2tgrp_->errMsg().isEmpty() )
		msgs += d2tgrp_->errMsg();

	    msgs += tr("Alternatively, switch off the use of a"
			" Depth to Time model file");
	    uiMSG().errorWithDetails( msgs );
	    return false;
	}

	if ( !d2tgrp_->warnMsg().isEmpty() )
	{
	    uiString msg = tr("The Time-Depth Model file loading issued"
			       " a warning")
			       .addMoreInfo( d2tgrp_->warnMsg(), true );
	    uiMSG().warning( msg );
	}

	datasrcnms.add( d2tgrp_->dataSourceName() );
	if ( d2tgrp_->wantAsCSModel() )
	    wd_.checkShotModel() = wd_.d2TModel();
    }

    SilentTaskRunnerProvider trprov;
    const uiRetVal uirv = Well::MGR().store( wd_, outioobj->key(), trprov );
    if ( !uirv.isOK() )
	mErrRet( uirv );

    outioobj->pars().update( sKey::CrFrom(), datasrcnms.cat("`") );
    outioobj->updateCreationPars();
    outioobj->commitChanges();

    if ( saveButtonChecked() )
	importReady.trigger();

    return false;
}


bool uiWellImportAsc::checkInpFlds()
{
    const int cursel = tracksrcfld_->getIntValue();
    const bool istrack = cursel==0;
    const bool isdir = cursel==1;
    const bool isvert = cursel==2;
    if ( istrack || isdir )
    {
	if ( !*trckinpfld_->fileName() )
	    mErrRet(uiStrings::phrSpecify(tr("a well track file")))

	if ( istrack && !dataselfld_->commit() )
	    return false;
	if ( isdir && !dirselfld_->commit() )
	    return false;
    }

    if ( !istrack && !SI().isReasonable(coordfld_->getCoord()) )
    {
	if ( !uiMSG().askGoOn(
		tr("Well coordinate seems to be far outside the survey."
		   "\nIs this correct?")) )
	    return false;
    }

    if ( isvert )
    {
	if ( !kbelevfld_->isChecked() )
	    mErrRet(uiStrings::phrSpecify(toUiString(Well::Info::sKBElev())))

	if ( !tdfld_->isChecked() )
	    mErrRet(uiStrings::phrSpecify(toUiString(Well::Info::sTD())))
    }

    if ( !outfld_->ioobj() )
	return false;

    return true;
}

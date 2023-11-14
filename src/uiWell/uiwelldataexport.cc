/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/


#include "uiwelldataexport.h"

#include "filepath.h"
#include "ioobj.h"
#include "od_ostream.h"
#include "oddirs.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "wellmarker.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellreader.h"
#include "welltrack.h"

#include "uiapplserv.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipixmap.h"
#include "uiseparator.h"
#include "uitoolbutton.h"
#include "uiwellman.h"
#include "uiwellpartserv.h"
#include "uiwellsel.h"


static const char* sKeyTVD()		{ return "TVD"; }
static const char* sKeyTVDGL()		{ return "TVDGL"; }
static const char* sKeyTVDSD()		{ return "TVDSD"; }
static const char* sKeyVint()		{ return "Vint"; }

static const UnitOfMeasure* getDisplayUnit( uiGenInput* ztype )
{
    return ztype->getBoolValue() ?
			    UoMR().get( "Meter" ) : UoMR().get( "Feet" );
}

#define mConvertVal(val) \
 getConvertedValue( UnitOfMeasure::surveyDefDepthUnit()->userValue(val), \
	    UnitOfMeasure::surveyDefDepthUnit(),getDisplayUnit(ztypefld_) ) \


uiWellExportFacility::uiWellExportFacility( uiParent* p,
						    uiWellPartServer& wps )
    : uiDialog(p,uiDialog::Setup(tr("Well Export Facility"),
				    uiString::empty(),mODHelpKey(mWellExpDlg)))
    , wellpartserver_(wps)
{
    setModal( false );
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );
    wellselfld_ = new uiWellSel( this, true, uiStrings::sWell(), false );
    mAttachCB( wellselfld_->selectionDone, uiWellExportFacility::inputChngCB );

    wellmanbut_ = new uiToolButton( this, "man_wll",
				uiStrings::phrManage(uiStrings::sWells()),
				mCB(this,uiWellExportFacility,updateDataCB) );
    wellmanbut_->attach( rightOf, wellselfld_ );

    refreshbut_ = new uiToolButton( this, "refresh", tr("Refresh"),
				    mCB(this,uiWellExportFacility,refreshCB) );
    refreshbut_->attach( rightOf, wellmanbut_ );

    //Top Group
    auto* topsep = new uiSeparator( this, "Top" );
    topsep->attach( stretchedBelow, wellselfld_ );
    auto* topgrp = new uiGroup( this );
    topgrp->attach( ensureBelow, topsep );

    selobjbox_ = new uiCheckBox( topgrp, tr("Select All") );
    selobjbox_->attach( leftBorder );
    selobjbox_->setChecked( true );
    topgrp->setHAlignObj( selobjbox_ );
    mAttachCB( selobjbox_->activated, uiWellExportFacility::selChngCB );

    auto* label = new uiLabel( topgrp, tr("Objects") );
    welltrack_ = new uiCheckBox( topgrp, sWellTrack() );
    mAttachCB( welltrack_->activated, uiWellExportFacility::updateSelButCB );
    welltrack_->attach( alignedBelow, selobjbox_ );
    label->attach( leftOf, welltrack_ );
    welltrack_->setChecked( true );

    markers_ = new uiCheckBox( topgrp, sMarkerUIKey() );
    mAttachCB( markers_->activated, uiWellExportFacility::updateSelButCB );
    markers_->attach( rightOf, welltrack_ );
    markers_->setChecked( true );

    d2tmodel_ = new uiCheckBox( topgrp, sD2TModel() );
    d2tmodel_->attach( rightOf, markers_ );
    mAttachCB( d2tmodel_->activated, uiWellExportFacility::updateSelButCB );
    mAttachCB( d2tmodel_->activated,
				uiWellExportFacility::updateTravelTimeFld );
    d2tmodel_->setChecked( true );

    checkshotdata_ = new uiCheckBox( topgrp, sCheckShot() );
    checkshotdata_->attach( rightOf, d2tmodel_ );
    mAttachCB( checkshotdata_->activated,
					uiWellExportFacility::updateSelButCB );
    mAttachCB( checkshotdata_->activated,
				uiWellExportFacility::updateTravelTimeFld );
    checkshotdata_->setChecked( true );

    traveltymfld_ = new uiGenInput( topgrp , tr("Travel Time"),
			    BoolInpSpec(true,uiStrings::sTWT(),tr("OWT")) );
    traveltymfld_->attach( alignedBelow, welltrack_ );

    //Log Group
    auto* botsep = new uiSeparator( this, "Bottom" );
    botsep->attach( stretchedBelow, topgrp );

    loggrp_ = new uiGroup( this, "LogGroup" );
    loggrp_->attach( ensureBelow, botsep );

    loglist_ = new uiLabeledListBox( loggrp_, uiStrings::sLogs(),
				    OD::ChoiceMode::ChooseZeroOrMore );
    loglist_->attach( leftBorder );
    loggrp_->setHAlignObj( loglist_ );
    loglist_->primaryCheckBox()->setChecked( true );

    const bool zinft = SI().depthsInFeet();
    const uiString lbl = tr("Log Depth Range %1").
			    arg( uiStrings::sDistUnitString(zinft,true,true) );
    zrangefld_ = new uiGenInput( loggrp_, lbl, FloatInpIntervalSpec(true) );
    zrangefld_->attach( alignedBelow, loglist_ );
    setDefaultRange( zinft );

    auto* logbotsep = new uiSeparator( this, "LogBottomSep" );
    logbotsep->attach( stretchedBelow, loggrp_ );

    //ZRange and Output
    ztypefld_ = new uiGenInput( this,
		    uiStrings::phrOutput(uiStrings::sZUnit()),
	BoolInpSpec(!zinft,uiStrings::sMeter(),uiStrings::sFeet()) );
    ztypefld_->attach( ensureBelow, logbotsep );

    auto* outsep = new uiSeparator( this, "OutputSeparator" );
    outsep->attach( stretchedBelow, ztypefld_ );

    basenmfld_ = new uiGenInput( this, tr("Base Name") );
    basenmfld_->attach( ensureBelow, outsep );

    uiFileInput::Setup fssu;
    fssu.forread( false );
    fssu.directories( true );
    outfilefld_ = new uiFileInput( this,
			uiStrings::phrOutput(uiStrings::sDirectory()), fssu );
    outfilefld_->attach( alignedBelow, basenmfld_ );
    const FilePath expfp( GetDataDir(), "Export" );
    if ( expfp.exists() )
	outfilefld_->setText( expfp.fullPath() );

    mAttachCB( postFinalize(), uiWellExportFacility::inputChngCB );
}


uiWellExportFacility::~uiWellExportFacility()
{
    detachAllNotifiers();
}


void uiWellExportFacility::updateDataCB( CallBacker* )
{
    wellpartserver_.manageWells();
}


void uiWellExportFacility::refreshCB( CallBacker* )
{
    wellselfld_->updateInput();
    inputChngCB( nullptr );
}


void uiWellExportFacility::inputChngCB( CallBacker* )
{
    ConstPtrMan<IOObj> ioobj = wellselfld_->getIOObj( true );
    if ( ioobj )
	wd_ = Well::MGR().get( ioobj->key() );

    if ( wd_ )
    {
	basenmfld_->setText( wd_->name() );
	checkshotdata_->setSensitive( wd_->haveCheckShotModel() );
	checkshotdata_->setChecked( wd_->haveCheckShotModel() );
	d2tmodel_->setSensitive( wd_->haveD2TModel() );
	d2tmodel_->setChecked( wd_->haveD2TModel() );

	const Well::MarkerSet& mrkrs = wd_->markers();
	markers_->setSensitive( !mrkrs.isEmpty() );
	markers_->setChecked( !mrkrs.isEmpty() );
	const Well::LogSet& logs = wd_->logs();
	BufferStringSet lognms;
	logs.getNames( lognms, false );
	if ( lognms.isEmpty() )
	{
	    loggrp_->display( false );
	    loglist_->primaryCheckBox()->setChecked( false );
	}
	else
	{
	    loglist_->setEmpty();
	    loglist_->addItems( lognms );
	    loggrp_->display( true );
	    loglist_->primaryCheckBox()->setChecked( true );
	    setDefaultRange( SI().zInFeet() );
	}
    }
}


void uiWellExportFacility::selChngCB( CallBacker* )
{
    const OD::CheckState checkstate = selobjbox_->getCheckState();
    if ( checkstate == OD::CheckState::PartiallyChecked )
	return;

    const bool selall = selobjbox_->isChecked();
    welltrack_->setChecked( selall );
    markers_->setChecked( selall );
    d2tmodel_->setChecked( selall );
    checkshotdata_->setChecked( selall );
}


void uiWellExportFacility::updateSelButCB( CallBacker* )
{
    if ( !welltrack_ || !markers_ || !d2tmodel_ || !checkshotdata_ )
	return;

    const bool allsel = (welltrack_->isChecked() || !welltrack_->isSensitive())
		    && (markers_->isChecked() || !markers_->isSensitive()) &&
		    (d2tmodel_->isChecked() || !d2tmodel_->isSensitive()) &&
	    (checkshotdata_->isChecked() || !checkshotdata_->isSensitive());
    const bool allunsel = !welltrack_->isChecked() && !markers_->isChecked() &&
		    !d2tmodel_->isChecked() && !checkshotdata_->isChecked();


    if ( allsel )
	selobjbox_->setCheckState( OD::CheckState::Checked );
    else if ( allunsel )
	selobjbox_->setCheckState( OD::CheckState::Unchecked );
    else if ( !allsel && !allunsel )
	selobjbox_->setCheckState( OD::CheckState::PartiallyChecked );
}


void uiWellExportFacility::updateTravelTimeFld( CallBacker* )
{
    if ( !wd_ )
	return;

    const bool eithermodel =
			d2tmodel_->isChecked() || checkshotdata_->isChecked();
    traveltymfld_->setSensitive( eithermodel );
}


void uiWellExportFacility::setDefaultRange( bool zinft )
{
    if ( !wd_ )
	return;

    StepInterval<float> dahintv;
    const Well::LogSet& logs = wd_->logs();
    for ( int idx=0; idx<logs.size(); idx++ )
    {
	const Well::Log& log = logs.getLog(idx);
	const int logsz = log.size();
	if ( logsz==0 )
	    continue;

	dahintv.include( logs.dahInterval() );
	const float width = log.dah(logsz-1) - log.dah(0);
	dahintv.step = width / (logsz-1);
	break;
    }


    StepInterval<float> disprg = dahintv;
    const UnitOfMeasure* storunit = UnitOfMeasure::surveyDefDepthStorageUnit();
    const UnitOfMeasure* outunit = UnitOfMeasure::surveyDefDepthUnit();
    disprg.start = getConvertedValue( dahintv.start, storunit, outunit );
    disprg.stop = getConvertedValue( dahintv.stop, storunit, outunit );
    disprg.step = getConvertedValue( dahintv.step, storunit, outunit );

    zrangefld_->setValue( disprg );
}


od_ostream* uiWellExportFacility::getOutputStream( const char* fp,
						    const BufferString& type )
{
    FilePath outfp( fp );
    const BufferString basenm = basenmfld_->text();
    BufferString filenm;
    if ( basenm.isEmpty() )
	filenm = type;
    else
	filenm = BufferString( basenm, "_", type );

    outfp.add( filenm );
    outfp.setExtension( ".dat" );
    auto* strm = new od_ostream( outfp.fullPath() );
    if ( !strm->isOK() )
    {
	uiMSG().error( uiStrings::phrCannotCreate(toUiString(filenm)) );
	delete strm;
	return nullptr;
    }

    return strm;
}


#define mErrRetStrm \
{ \
    uiMSG().error( strmErrMsg() ); \
    return false; \
} \


bool uiWellExportFacility::exportWellTrack( const char* fp )
{
    od_ostream* strm = getOutputStream( fp, "WellTrack" );
    if ( !strm || strm->isBad() )
	mErrRetStrm

    const BufferString depthunit =
			getDistUnitString( !ztypefld_->getBoolValue(), true );
    *strm << sKey::X() << SI().getXYUnitString() << od_tab;
    *strm << sKey::Y() << SI().getXYUnitString() << od_tab;
    *strm << sKey::Z() << depthunit << od_tab;
    *strm << sKey::TVD() << depthunit << od_tab;
    *strm << sKey::MD() << depthunit << od_newline;

    const Well::Track& track = wd_->track();

    const float kbdepth = -1.f * track.getKbElev();
    for ( int idx=0; idx<track.size(); idx++ )
    {
	const Coord3 coord( track.pos(idx) );
	*strm << coord.x << od_tab;
	*strm << coord.y << od_tab;
	*strm << mConvertVal(coord.z) << od_tab;
	*strm << mConvertVal(coord.z-kbdepth) << od_tab;
	*strm << mConvertVal(track.dah(idx)) << od_newline;
    }

    strm->close();
    delete strm;
    return true;
}


bool uiWellExportFacility::exportWellLogs( const char* fp )
{
    od_ostream* strm = getOutputStream( fp, "WellLogs" );
    if ( !strm || strm->isBad() )
	mErrRetStrm

    writeLogHeader( *strm );
    writeLogs( *strm );
    strm->close();
    delete strm;
    return true;
}


void uiWellExportFacility::writeLogHeader( od_ostream& strm )
{
    BufferString depthstr( "Depth",
	getDistUnitString(!ztypefld_->getBoolValue(),true ) );
    strm << depthstr.quote();
    const Well::LogSet& logs = wd_->logs();
    BufferStringSet logsel;
    loglist_->getChosen( logsel );
    for ( int idx=0; idx<logs.size(); idx++ )
    {
	const Well::Log& log = logs.getLog(idx);
	if ( !logsel.isPresent( log.name() ) )
	    continue;

	BufferString lognm( log.name() );
	lognm.clean();
	lognm.replace( '+', '_' );
	lognm.replace( '-', '_' );
	if ( *log.unitMeasLabel() )
	    lognm.add( "(" ).add( log.unitMeasLabel() ).add( ")" );

	lognm.quote();
	strm << od_tab << lognm;
    }

    strm << od_newline;
}


void uiWellExportFacility::writeLogs( od_ostream& strm )
{
    const StepInterval<float> intv = zrangefld_->getFStepInterval();
    const int nrsteps = intv.nrSteps();

    const UnitOfMeasure* storunit = UnitOfMeasure::surveyDefDepthStorageUnit();
    const UnitOfMeasure* userunit = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* outunit = getDisplayUnit( ztypefld_ );

    const Well::LogSet& logs = wd_->logs();
    BufferStringSet logsel;
    loglist_->getChosen( logsel );
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	const float md = intv.atIndex( idx );
	const float mdstor = getConvertedValue( md, userunit, storunit );
	const float mdout = getConvertedValue( md, userunit, outunit );
	strm << mdout;

	for ( int logidx=0; logidx<logs.size(); logidx++ )
	{
	    const Well::Log& log = logs.getLog( logidx );
	    if ( !logsel.isPresent( log.name() ) )
		continue;

	    const float val = log.getValue( mdstor );
	    if ( mIsUdf(val) )
		strm << od_tab << mUdf(float);
	    else
		strm << od_tab << val;
	}
	strm << od_newline;
    }
}


bool uiWellExportFacility::exportD2TModel( const char* fp, bool ischksht )
{
    od_ostream* strm = getOutputStream( fp,
					ischksht ? "CheckShot" : "D2TModel"  );
    if ( !strm || strm->isBad() )
	mErrRetStrm

    Well::D2TModel* d2t = ischksht ? wd_->checkShotModel() : wd_->d2TModel();
    if ( !d2t || d2t->size() < 2 )
    {
	uiMSG().error( tr("Invalid %1 data")
			    .arg(ischksht ? sCheckShot() : sD2TModel()) );
	return false;
    }

    const float kbelev = mConvertVal(wd_->track().getKbElev());
    const float groundevel = mConvertVal(wd_->info().groundelev_);
    const float srd = mConvertVal(mCast(float,SI().seismicReferenceDatum()));
    const bool hastvdgl = !mIsUdf( groundevel );
    const bool hastvdsd = !mIsZero( srd, 1e-3f );
    BufferStringSet header;
    const bool zinfeet = !ztypefld_->getBoolValue();
    const BufferString depthunit = getDistUnitString( zinfeet, true );
    *strm << BufferString(sKey::MD(),depthunit) << od_tab <<
	BufferString(sKeyTVD(),depthunit) << od_tab;
    if ( hastvdgl )
	*strm << BufferString(sKeyTVDGL(),depthunit) << od_tab;

    *strm << BufferString(sKey::TVDSS(),depthunit) << od_tab;
    if ( hastvdsd )
	*strm << BufferString(sKeyTVDSD(),depthunit) << od_tab;

    const bool istwt = traveltymfld_->getBoolValue();
    *strm <<  BufferString( istwt ? sKey::TWT() : "OWT",
	    UnitOfMeasure::surveyDefTimeUnitAnnot(true,true).getFullString() )
								    << od_tab;
    *strm << BufferString( sKeyVint(), getVelUnitString(zinfeet,true) )
								<< od_newline;
    float vint;
    const Well::Track& track = wd_->track();
    const Well::Info& info = wd_->info();
    for ( int idx=0; idx<d2t->size(); idx++ )
    {
	const float dah = d2t->dah(idx);
	const float tvdss = mConvertVal(mCast(float,track.getPos(dah).z));
	const float tvd = tvdss + kbelev;
	*strm << mConvertVal(dah) << od_tab << tvd << od_tab;
	if ( hastvdgl )
	{
	    const float tvdgl = tvdss + groundevel;
	    *strm << tvdgl << od_tab;
	}

	*strm << tvdss << od_tab;
	if ( hastvdsd )
	{
	    const float tvdsd = tvdss + srd;
	    *strm << tvdsd << od_tab;
	}

	*strm << UnitOfMeasure::surveyDefTimeUnit()->userValue(d2t->t(idx)) /
	     (1.f + (int)!istwt) << od_tab;
	Interval<float> replvellayer( track.getKbElev(), srd );
	replvellayer.widen( 1e-2f, true );
	vint = replvellayer.includes( -1.f * track.getPos(dah).z, true ) &&
	    !mIsUdf(info.replvel_) ?
	    info.replvel_ : mCast(float,d2t->getVelocityForDah(dah,track));
	*strm << mConvertVal(vint) << od_newline;
    }

    strm->close();
    delete strm;
    return true;
}



bool uiWellExportFacility::exportMarkers( const char* fp )
{
    od_ostream* strm = getOutputStream( fp, "Markers" );
    if ( !strm || strm->isBad() )
	mErrRetStrm

    const Well::MarkerSet& mrks = wd_->markers();
    BufferStringSet colnms;
    colnms.add( sKey::Name() );
    const bool zinfeet = !ztypefld_->getBoolValue();
    const BufferString zunituistr = getDistUnitString( zinfeet, true );
    *strm << BufferString(sKey::MD(),zunituistr) << od_tab
	 << BufferString(sKey::TVD(),zunituistr) << od_tab
	 << BufferString(sKey::TVDSS(),zunituistr) << od_tab
	 << sKey::Color() << od_newline;
    const Well::Track& track = wd_->track();
    const float kbelev = track.getKbElev();
    for ( int idx=0; idx<mrks.size(); idx++ )
    {
	const Well::Marker& mrkr = *mrks[idx];
	const float dah = mrkr.dah();
	const float tvdss = mCast(float,track.getPos(dah).z);
	const float tvd = tvdss + kbelev;
	*strm << mConvertVal( dah ) << od_tab
	     << mConvertVal( tvd ) << od_tab
	     << mConvertVal( tvdss ) << od_tab
	     << mrkr.name() << od_newline;
    }

    strm->close();
    delete strm;
    return true;
}


bool uiWellExportFacility::acceptOK( CallBacker* )
{
    if ( !wd_ )
    {
	uiMSG().error( tr("Incorrect input data") );
	return false;
    }

    FilePath basedir( outfilefld_->fileName() );
    if ( basedir.isEmpty() )
    {
	uiMSG().error( tr("Folder location cannot be empty") );
	return false;
    }
    else if ( !File::exists(basedir.fullPath().buf()) )
    {
	uiMSG().error( tr("Location does not exist") );
	return false;
    }

    uiStringSet errobjtype;
    bool somethingexported = false;
    const BufferString& baseafp = basedir.fullPath();
    if ( welltrack_ && welltrack_->isChecked() )
    {
	somethingexported = true;
	if ( !exportWellTrack(baseafp) )
	    errobjtype.add( sWellTrack() );
    }

    if ( checkshotdata_ && checkshotdata_->isChecked() )
    {
	somethingexported = true;
	if ( !exportD2TModel(baseafp,true) )
	    errobjtype.add( sCheckShot() );
    }

    if ( d2tmodel_ && d2tmodel_->isChecked() )
    {
	somethingexported = true;
	if ( !exportD2TModel(baseafp,false) )
	    errobjtype.add( sD2TModel() );
    }

    if ( markers_ && markers_->isChecked() )
    {
	somethingexported = true;
	if ( !exportMarkers(baseafp) )
	    errobjtype.add( sMarkerUIKey() );
    }

    BufferStringSet logsels;
    if ( loglist_->nrChosen() > 0 )
    {
	somethingexported = true;
	if ( !exportWellLogs(baseafp) )
	    errobjtype.add( uiStrings::sWellLog(mPlural) );
    }

    if ( !somethingexported )
	uiMSG().error( tr("No object selected for export") );

    if ( errobjtype.size() > 0 )
    {
	uiMSG().error( tr("Export failed for following objects.\n %1")
	    .arg(errobjtype.createOptionString() ) );
	return false;
    }

    const bool ret = uiMSG().askGoOn(
		tr("Export successful. Do want to export more wells?") );

    return !ret;
}

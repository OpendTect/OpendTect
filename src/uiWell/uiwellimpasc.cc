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
#include "ioman.h"
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
#include "wellimpasc.h"
#include "welltrack.h"
#include "wellwriter.h"

#include "uibutton.h"
#include "uid2tmodelgrp.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiselsurvranges.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"
#include "uiwellsel.h"
#include "od_helpids.h"


uiWellImportAsc::uiWellImportAsc( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import Well Track"),mNoDlgTitle,
			   mODHelpKey(mWellImportAscHelpID)).modal(false))
    , fd_(*Well::TrackAscIO::getDesc())
    , dirfd_(*Well::DirectionalAscIO::getDesc())
    , wd_(*new Well::Data)
    , zun_(UnitOfMeasure::surveyDefDepthUnit())
    , importReady(this)
    , havetrckbox_(0)
    , vertwelllbl_(0)
{
    setVideoKey( mODVideoKey(mWellImportAscHelpID) );
    enableSaveButton( tr("Display after import") );
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiStringSet options;
    options.add( tr("Well Track file (XYZ)") )
	   .add( tr("Directional Well") )
	   .add( tr("Vertical Well") );
    tracksrcfld_ = new uiGenInput( this, tr("Input type"),
				StringListInpSpec(options) );
    tracksrcfld_->valuechanged.notify( mCB(this,uiWellImportAsc,haveTrckSel) );

    trckinpfld_ = new uiASCIIFileInput( this, tr("Well Track file"), true );
    trckinpfld_->valuechanged.notify( mCB(this,uiWellImportAsc,inputChgd) );
    trckinpfld_->attach( alignedBelow, tracksrcfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
			mODHelpKey(mWellImportAscDataSelHelpID) );
    dataselfld_->attach( alignedBelow, trckinpfld_ );
    dataselfld_->descChanged.notify( mCB(this,uiWellImportAsc,trckFmtChg) );

    dirselfld_ = new uiTableImpDataSel( this, dirfd_,
			mODHelpKey(mWellImportAscDataSelHelpID) );
    dirselfld_->attach( alignedBelow, trckinpfld_ );
    dirselfld_->display( false );

    const uiString coordunitslbl(
		tr("First Coordinate %1").arg(SI().getUiXYUnitString()) );
    coordfld_ = new uiGenInput( this, coordunitslbl,
			PositionInpSpec(PositionInpSpec::Setup(true)) );
    coordfld_->attach( alignedBelow, dataselfld_ );

    const uiString zunit = UnitOfMeasure::surveyDefDepthUnitAnnot( true, true );
    const uiString kblbl =
		toUiString( "%1 %2" ).arg(Well::Info::sKBElev()).arg(zunit);
    kbelevfld_ = new uiGenInput( this, kblbl, FloatInpSpec(0) );
    kbelevfld_->setWithCheck();
    kbelevfld_->setChecked( false );
    kbelevfld_->attach( alignedBelow, coordfld_ );

    const uiString tdlbl =
		toUiString("%1 %2").arg(Well::Info::sTD()).arg(zunit);
    tdfld_ = new uiGenInput( this, tdlbl, FloatInpSpec() );
    tdfld_->setWithCheck();
    tdfld_->setChecked( false );
    tdfld_->attach( alignedBelow, kbelevfld_ );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, tdfld_ );

    wd_.ref();
    float dispval = wd_.info().replvel_;
    if ( !mIsUdf(dispval) )
	wd_.info().replvel_ = dispval;

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

    uiButton* but = new uiPushButton( this, tr("Advanced/Optional"),
					mCB(this,uiWellImportAsc,doAdvOpt),
					false );
    but->attach( alignedBelow, zistime ? (uiObject*)d2tgrp_
				       : (uiObject*)dataselfld_ );
    but->attach( ensureBelow, sep );

    outfld_ = new uiWellSel( this, false, uiString::emptyString(), false );
    outfld_->attach( alignedBelow, but );

    postFinalize().notify( mCB(this,uiWellImportAsc,haveTrckSel) );
}


uiWellImportAsc::~uiWellImportAsc()
{
    delete &fd_;
    delete &dirfd_;
    wd_.unRef();
}


MultiID uiWellImportAsc::getWellID() const
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
    FilePath fnmfp( trckinpfld_->fileName() );
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
	if ( ti.name() == Well::Info::sKeyTVDSS() && ti.selection_.isInFile(0) )
	    havez = true;

	if ( ti.name() == "MD" && ti.selection_.isInFile(0) )
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
{ mODTextTranslationClass(uiWellImportAscOptDlg)
public:

uiWellImportAscOptDlg( uiWellImportAsc* impasc )
    : uiDialog(impasc,uiDialog::Setup(tr("Import well: Advanced/Optional"),
				mNoDlgTitle,mODHelpKey(mWellImpPptDlgHelpID)))
    , uwia_(impasc)
    , zun_(UnitOfMeasure::surveyDefDepthUnit())
{
    const Well::Info& info = uwia_->wd_.info();

    PositionInpSpec::Setup possu( true );
    if ( !mIsZero(info.surfacecoord_.x,0.1) )
	possu.coord_ = info.surfacecoord_;
    coordfld_ = new uiGenInput( this,
	tr("Surface Coordinate (if different from first "
	   "coordinate in track file)"),
	PositionInpSpec(possu).setName("X",0).setName("Y",1) );

    const bool zinfeet = SI().depthsInFeet();

    float dispval = info.replvel_;
    if ( zinfeet && zun_ )
	dispval = zun_->userValue( info.replvel_ );

    uiString lbl = toUiString("%1 %2")
		.arg( Well::Info::sReplVel() )
		.arg( UnitOfMeasure::surveyDefVelUnitAnnot(true,true) );
    replvelfld_ = new uiGenInput( this, lbl, FloatInpSpec(dispval) );
    replvelfld_->attach( alignedBelow, coordfld_ );

    dispval = info.groundelev_;
    if ( zinfeet && zun_ )
	dispval = zun_->userValue( info.groundelev_ );
    lbl = toUiString("%1 %2")
		.arg( Well::Info::sGroundElev() )
		.arg( UnitOfMeasure::surveyDefDepthUnitAnnot(true,true) );
    gdelevfld_ = new uiGenInput( this, lbl, FloatInpSpec(dispval) );
    gdelevfld_->attach( alignedBelow, replvelfld_ );

    idfld_ = new uiGenInput( this, Well::Info::sUwid(),
			     StringInpSpec(info.uwid_) );
    idfld_->attach( alignedBelow, gdelevfld_ );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, idfld_ );

    fieldfld_ = new uiGenInput( this, Well::Info::sField(),
				StringInpSpec(info.field_) );
    fieldfld_->attach( ensureBelow, horsep );

    operfld_ = new uiGenInput( this, Well::Info::sOper(),
			       StringInpSpec(info.oper_) );
    operfld_->attach( alignedBelow, idfld_ );
    operfld_->attach( rightTo, fieldfld_ );

    countyfld_ = new uiGenInput( this, Well::Info::sCounty(),
				 StringInpSpec(info.county_) );
    countyfld_->attach( alignedBelow, fieldfld_ );

    statefld_ = new uiGenInput( this, Well::Info::sState(),
				StringInpSpec(info.state_) );
    statefld_->attach( alignedBelow, operfld_ );

    provfld_ = new uiGenInput( this, Well::Info::sProvince(),
				StringInpSpec(info.province_) );
    provfld_->attach( alignedBelow, countyfld_ );

    countryfld_ = new uiGenInput( this, Well::Info::sCountry(),
				StringInpSpec(info.country_) );
    countryfld_->attach( alignedBelow, statefld_ );

    countyfld_->attach( ensureLeftOf, statefld_ );
    provfld_->attach( ensureLeftOf, countryfld_ );
}


static void setWellInfo( BufferString& infostr, const char* fldtxt )
{
    const FixedString fldstr = fldtxt;
    if ( !fldstr.isEmpty() )
	infostr = fldstr;
}


bool acceptOK( CallBacker* ) override
{
    Well::Info& info = uwia_->wd_.info();

    if ( *coordfld_->text() )
	info.surfacecoord_ = coordfld_->getCoord();

    if ( *replvelfld_->text() )
    {
	const float replvel = replvelfld_->getFValue();
	if ( !mIsUdf(replvel) && zun_ )
	    info.replvel_ = zun_->internalValue( replvel );
    }

    if ( *gdelevfld_->text() )
    {
	const float gdevel = gdelevfld_->getFValue();
	if ( !mIsUdf(gdevel)  && zun_ )
	    info.groundelev_ = zun_->internalValue( gdevel );
    }

    setWellInfo( info.uwid_, idfld_->text() );
    setWellInfo( info.field_, fieldfld_->text() );
    setWellInfo( info.oper_, operfld_->text() );
    setWellInfo( info.state_, statefld_->text() );
    setWellInfo( info.county_, countyfld_->text() );
    setWellInfo( info.province_, provfld_->text() );
    setWellInfo( info.country_, countryfld_->text() );

    return true;
}

    const UnitOfMeasure* zun_;
    uiWellImportAsc*	uwia_;
    uiGenInput*		coordfld_;
    uiGenInput*		elevfld_;
    uiGenInput*		replvelfld_;
    uiGenInput*		gdelevfld_;
    uiGenInput*		idfld_;
    uiGenInput*		fieldfld_;
    uiGenInput*		operfld_;
    uiGenInput*		countyfld_;
    uiGenInput*		statefld_;
    uiGenInput*		provfld_;
    uiGenInput*		countryfld_;
};


void uiWellImportAsc::doAdvOpt( CallBacker* )
{
    uiWellImportAscOptDlg dlg( this );
    dlg.go();
}


bool uiWellImportAsc::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() )
	return false;

    doWork();
    wd_.info().surfacecoord_.x = wd_.info().surfacecoord_.y = 0;
    wd_.info().groundelev_ = mUdf(float);
    uiString msg = tr("Well Track successfully imported."
		      "\n\nDo you want to import more Well Tracks?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


#define mErrRet(s) { if ( (s).isSet() ) uiMSG().error(s); return false; }

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
		uiString msg = tr( "The track file cannot be loaded:\n%1" )
				    .arg( wellascio.errMsg() );
		mErrRet( msg );
	    }

	    if ( wellascio.warnMsg().isSet() )
	    {
		uiString msg =
			tr( "The track file loading issued a warning:\n%1" )
			.arg( wellascio.warnMsg() );
		uiMSG().warning( msg );
	    }
	}
	else if ( isdir )
	{
	    Well::DirectionalAscIO dirascio( dirfd_, strm );
	    wd_.info().surfacecoord_ = coordfld_->getCoord();
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
	    float survzstop = SI().zRange(false).stop;
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

	    if ( d2tgrp_->errMsg().isSet() )
		msgs += d2tgrp_->errMsg();

	    msgs += tr("Alternatively, switch off the use of a"
			" Depth to Time model file");
	    uiMSG().errorWithDetails( msgs );
	    return false;
	}

	if ( d2tgrp_->warnMsg().isSet() )
	{
	    uiString msg = tr( "The Time-Depth Model file loading issued"
			       " a warning:\n%1" )
			       .arg( d2tgrp_->warnMsg() );
	    uiMSG().warning( msg );
	}

	datasrcnms.add( d2tgrp_->dataSourceName() );
	if ( d2tgrp_->wantAsCSModel() )
	    wd_.setCheckShotModel( new Well::D2TModel( *wd_.d2TModel() ) );
    }

    Well::Writer wwr( *outioobj, wd_ );
    if ( !wwr.put() )
	mErrRet( wwr.errMsg() );

    outioobj->pars().update( sKey::CrFrom(), datasrcnms.cat("`") );
    outioobj->updateCreationPars();
    IOM().commitChanges( *outioobj );

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
	    mErrRet(tr("Please specify a well track file"))

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
	    mErrRet(tr("Please specify a %1").arg(Well::Info::sKBElev()))

	if ( !tdfld_->isChecked() )
	    mErrRet(tr("Please specify a %1").arg(Well::Info::sTD()))
    }

    if ( !outfld_->ioobj() )
	return false;

    return true;
}

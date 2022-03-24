/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2012
________________________________________________________________________

-*/


#include "uibulkwellimp.h"

#include "ctxtioobj.h"
#include "directionalsurvey.h"
#include "globexpr.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"
#include "veldesc.h"
#include "welld2tmodel.h"
#include "wellimpasc.h"
#include "wellman.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "welltrack.h"
#include "welltransl.h"
#include "wellwriter.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiprogressbar.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uid2tmodelgrp.h"
#include "od_helpids.h"

using namespace Well;


// uiBulkTrackImport
uiBulkTrackImport::uiBulkTrackImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Multi-Well Import: Well Tracks"),
				 mNoDlgTitle,
				 mODHelpKey(mBulkTrackImportHelpID) )
			   .modal(false))
    , fd_(BulkTrackAscIO::getDesc())
    , velocityfld_(0)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );
    origtracks_.allowNull();

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::Table );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				       mODHelpKey(mTableImpDataSelwellsHelpID));
    dataselfld_->attach( alignedBelow, inpfld_ );

    if ( SI().zIsTime() )
    {
	const uiString vellbl = tr("Temporary model velocity %1" )
				  .arg( VelocityDesc::getVelUnit( true ));
	const float vel = uiD2TModelGroup::getDefaultTemporaryVelocity();
	velocityfld_ = new uiGenInput( this, vellbl, FloatInpSpec(vel) );
	velocityfld_->attach( alignedBelow, dataselfld_ );
    }
}


uiBulkTrackImport::~uiBulkTrackImport()
{
    delete fd_;
    deepErase( origtracks_ );
}


static int getWellIdx( RefObjectSet<Data>& wells, const char* nm )
{
    for ( int idx=0; idx<wells.size(); idx++ )
    {
	if ( wells[idx]->info().name() == nm )
	    return idx;
    }

    return -1;
}


static IOObj* mkEntry( const CtxtIOObj& ctio, const char* nm )
{
    CtxtIOObj newctio( ctio );
    newctio.ioobj_ = 0; newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj_;
}


#define mDefEpsZ 1e-3

void uiBulkTrackImport::readFile( od_istream& istrm )
{
    BulkTrackAscIO aio( *fd_, istrm );
    BufferString wellnm, uwi; Coord3 crd;
    float md = mUdf(float);
    const bool depthistvd = aio.depthIsTVD();
    float kb = 0;
    while ( aio.get(wellnm,crd,md,uwi) )
    {
	if ( wellnm.isEmpty() )
	    continue;

	RefMan<Data> wd;
	const int widx = getWellIdx( wells_, wellnm );
	if ( wells_.validIdx(widx) )
	    wd = wells_[widx];
	else
	{
	    PtrMan<IOObj> ioobj = findIOObj( wellnm, uwi );
	    if ( ioobj )
	    {
		LoadReqs reqs( Inf, Trck );
		if ( SI().zIsTime() )
		    reqs.add( D2T );
		wd = MGR().get( ioobj->key(), reqs );
		if ( wd )
		{
		    Track& track = wd->track();
		    kb = track.getKbElev();
		    double sum = 0.;
		    for ( int idx=0; idx<track.size(); idx++ )
			sum += track.pos(idx).coord().abs();
		    if ( mIsZero(sum,1e-1) )
		    {
			origtracks_ += new Track( track );
			mdrgs_ += track.dahRange();
			track.setEmpty();
		    }
		    else
		    {
			origtracks_ += nullptr;
			mdrgs_ += Interval<float>::udf();
		    }
		}
		else
		    kb = 0.f;
	    }

	    if ( !wd )
	    {
		wd = new Data( wellnm );
		kb = 0.f;
		if ( !uwi.isEmpty() )
		    wd->info().uwid_ = uwi;
		origtracks_ += nullptr;
		mdrgs_ += Interval<float>::udf();
	    }

	    wells_ += wd;
	}

	if ( depthistvd )
	    crd.z -= kb;

	if ( !crd.isDefined() )
	    continue;

	Track& track = wd->track();
	if ( mIsUdf(md) )
	    track.insertPoint( crd );
	else
	{
	    if ( track.isEmpty() || md > track.dah(track.size()-1) )
		track.addPoint( crd, md );
	    else if ( md < track.dah(track.size()-1) + mDefEpsZ )
	    {
		const bool inserted = track.insertAtDah( md, float(crd.z) );
		if ( inserted )
		{
		    const int idx = track.indexOf( md );
		    if ( idx > -1 )
			const_cast<Coord3&>( track.pos(idx) ) = crd;
		}
	    }
	}
    }

    for ( int widx=0; widx<wells_.size(); widx++ )
    {
	RefMan<Data> wd = wells_[widx];
	Track& track = wd->track();
	if ( track.isEmpty() )
	{
	    if ( origtracks_[widx] )
		track = *origtracks_[widx];
	    continue;
	}

	LASImporter lasimp;
	lasimp.setData( *wd );
	bool adjusted = false;
	const Interval<float>& origmdrg = mdrgs_[widx];
	Interval<float> mdrg( 0, track.td() );
	if ( !origmdrg.isUdf() && track.td()-mDefEpsZ < origmdrg.stop )
	    mdrg.stop = origmdrg.stop;
	lasimp.adjustTrack( mdrg, false, adjusted );
    }
}


void uiBulkTrackImport::write( uiStringSet& errors )
{
    const bool zistime = SI().zIsTime();
    const double vel = velocityfld_ ? velocityfld_->getDValue() : mUdf(double);

    // TODO: Check if name exists, ask user to overwrite or give new name
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( Well );
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	RefMan<Data> wd = wells_[idx];
	PtrMan<IOObj> ioobj = IOM().get( wd->name(),
					 ctio->ctxt_.trgroup_->groupName() );
	const bool newwell = !ioobj;
	if ( newwell )
	    ioobj = mkEntry( *ctio, wd->name() );
	if ( !ioobj )
	{
	    errors.add( uiStrings::phrCannotCreateDBEntryFor(
						    toUiString(wd->name())) );
	    continue;
	}

	bool writed2t = false;
	if ( zistime )
	{
	    const Track& track = wd->track();
	    const D2TModel* d2torig = wd->d2TModel();
	    if ( newwell || !d2torig || d2torig->size() < 2 ||
		 (d2torig->size()==2 &&
		  mIsEqual(d2torig->getVelocityForDah(track.td(),track),
		  (double)uiD2TModelGroup::getDefaultTemporaryVelocity(),1) ))
	    {
		auto* d2t = new D2TModel;
		d2t->makeFromTrack(  wd->track(), vel, wd->info().replvel_ );
		wd->setD2TModel( d2t );
		writed2t = true;
	    }
	    else if ( !newwell )
	    {
		uiString errmsg;
		writed2t = wd->d2TModel()->ensureValid( *wd, errmsg );
	    }
	}

	Writer ww( *ioobj, *wd );
	if ( !ww.putInfoAndTrack() )
	{
	    uiString msg = uiStrings::phrCannotCreate(
		    toUiString("%1: %2").arg(wd->name()).arg(ww.errMsg()) );
	    errors.add( msg );
	}

	if ( writed2t && !ww.putD2T() )
	{
	    uiString msg = uiStrings::phrCannotEdit(
		    toUiString("%1: %2").arg(wd->name()).arg(ww.errMsg()) );
	    errors.add( msg );
	}

	const bool isloaded = MGR().isLoaded( ioobj->key() );
	if ( isloaded && MGR().reload(ioobj->key()) )
	{
	    RefMan<Data> loadedwd = MGR().get( ioobj->key(), LoadReqs(Trck) );
	    if ( loadedwd )
		loadedwd->trackchanged.trigger();
	}
    }
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiBulkTrackImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrInput(mJoinUiStrs(sFile(),sName())) )

    if ( SI().zIsTime() )
    {
	const double vel = velocityfld_->getDValue();
	if ( vel<=0. || mIsUdf(vel))
	{
	    mErrRet( tr("Please enter a positive velocity "
			"for generating the D2T model") )
	}
    }

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenInpFile() )

    if ( !dataselfld_->commit() )
	return false;

    wells_.erase();
    deepErase( origtracks_ );
    mdrgs_.setEmpty();

    readFile( strm );

    uiStringSet errors;
    write( errors );

    wells_.erase();
    deepErase( origtracks_ );
    mdrgs_.setEmpty();

    if ( errors.isEmpty() )
    {
	uiMSG().message( tr("All tracks imported succesfully") );
	return false;
    }

    uiMSG().errorWithDetails( errors,
		tr("Could not import all wells (see details)") );
    return false;
}


// uiBulkLogImport
uiBulkLogImport::uiBulkLogImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Multi-Well Import: Logs"),
				 mNoDlgTitle,
				 mODHelpKey(mBulkLogImportHelpID))
			   .modal(false))
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, tr("Input LAS files"), true );
    inpfld_->setFilter( Well::LASImporter::fileFilter() );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    mAttachCB( inpfld_->valuechanged, uiBulkLogImport::lasSel );

    ismdfld_ = new uiGenInput( this, tr("Depth values are"),
		    BoolInpSpec(true,tr("MD"),tr("TVDSS")) );
    ismdfld_->attach( alignedBelow, inpfld_ );

    const float defundefval = -999.25;
    udffld_ = new uiGenInput( this, tr("Undefined value in logs"),
		    FloatInpSpec(defundefval));
    udffld_->attach( alignedBelow, ismdfld_ );

    lognmfld_ = new uiGenInput( this, tr("Name log after"),
		BoolInpSpec(false,tr("Curve"),tr("Description")) );
    lognmfld_->attach( alignedBelow, udffld_ );

    welluwinmfld_ = new uiGenInput( this, tr("Using UWI as well name"),
	    BoolInpSpec(false,uiStrings::sYes(),uiStrings::sNo()) );
    welluwinmfld_->attach( alignedBelow, lognmfld_ );
    mAttachCB( welluwinmfld_->valuechanged, uiBulkLogImport::nameSelChg );

    wellstable_ = new uiTable( this, uiTable::Setup(3,3), "Wells" );
    wellstable_->setColumnLabel( 0, tr("Well name in LAS") );
    wellstable_->setColumnLabel( 1, tr("Well UWI in LAS") );
    wellstable_->setColumnLabel( 2, tr("Well name in OpendTect") );
    wellstable_->setColumnReadOnly( 0, true );
    wellstable_->setColumnReadOnly( 1, true );
    wellstable_->attach( ensureBelow, welluwinmfld_ );
}


uiBulkLogImport::uiBulkLogImport( uiParent* p, const BufferStringSet& filenms )
    : uiBulkLogImport(p)
{
    BufferString fnm;
    uiFileDialog::list2String( filenms, fnm );
    inpfld_->setFileName( fnm );
    lasSel( nullptr );
}

uiBulkLogImport::~uiBulkLogImport()
{
    detachAllNotifiers();
}


static void getWellNames( BufferStringSet& wellnms, bool withuwi )
{
    const IOObjContext ctxt = mIOObjContext( Well );
    const IODir iodir( ctxt.getSelKey() );
    IODirEntryList list( iodir, ctxt );
    list.sort();
    if ( !withuwi )
    {
	list.getIOObjNames( wellnms );
	return;
    }

    for ( int idx=0; idx<list.size(); idx++ )
    {
	const IOObj* ioobj = list[idx]->ioobj_;
	RefMan<Data> wd = Well::MGR().get( ioobj->key(), Well::Inf );
	if ( wd )
	{
	    const StringPair sp( wd->name(), wd->info().uwid_ );
	    wellnms.add( sp.getCompString(true) );
	}
    }
}


void uiBulkLogImport::lasSel( CallBacker* )
{
    BufferStringSet filenms;
    inpfld_->getFileNames( filenms );
    wellstable_->setNrRows( filenms.size() );

    BufferStringSet wellnms;
    wellnms.add( "-- (Do not import)" );
    getWellNames( wellnms, true );
    const bool useuwiasnm = welluwinmfld_->getBoolValue();
    for ( int idx=0; idx<filenms.size(); idx++ )
    {
	const BufferString& fnm = filenms.get( idx );
	LASImporter lasimp;
	LASImporter::FileInfo info;
	info.undefval_ = udffld_->getFValue();
	BufferString errmsg = lasimp.getLogInfo( fnm, info );

	wellstable_->setText( RowCol(idx,0), info.wellnm_ );
	wellstable_->setText( RowCol(idx,1), info.uwi_ );

	BufferStringSet listwellnms( wellnms );
	if ( !info.wellnm_.isEmpty() )
	    listwellnms.addIfNew( info.wellnm_ );
	if ( !info.uwi_.isEmpty() )
	    listwellnms.addIfNew( info.uwi_ );
	uiComboBox* wellsbox = new uiComboBox( 0, "Select Well" );
	wellsbox->addItems( listwellnms );
	wellstable_->setCellObject( RowCol(idx,2), wellsbox );

	const BufferString& welllasnm = useuwiasnm ? info.uwi_ : info.wellnm_;
	const BufferString& othwellnm = useuwiasnm ? info.wellnm_ : info.uwi_;
	const BufferString wellnm = welllasnm.isEmpty() ? othwellnm : welllasnm;
	const int selidx = listwellnms.nearestMatch( wellnm );
	wellsbox->setCurrentItem( selidx<0 ? 0 : selidx );
    }

    wellstable_->resizeColumnToContents( 0 );
}


void uiBulkLogImport::nameSelChg( CallBacker* )
{
    const bool useuwiasnm = welluwinmfld_->getBoolValue();
    for ( int idx=0; idx<wellstable_->nrRows(); idx++ )
    {
	uiObject* uiobj = wellstable_->getCellObject( RowCol(idx,2) );
	mDynamicCastGet(uiComboBox*,wellsbox,uiobj)
	if ( !wellsbox )
	    continue;

	const BufferString wellnm = wellstable_->text(RowCol(idx,0));
	const BufferString uwinm = wellstable_->text(RowCol(idx,1));
	if ( (useuwiasnm && uwinm.isEmpty()) ||
	     (!useuwiasnm && wellnm.isEmpty()) )
	    continue;

	wellsbox->setCurrentItem( useuwiasnm ? uwinm.str() : wellnm.str() );
    }
}


static bool createNewWell( const Well::LASImporter::FileInfo& info,
			   const char* wellnm, uiStringSet& errors )
{
    RefMan<Data> wd = new Data( wellnm );
    Track& track = wd->track();
    Coord3 wellhead( 0., 0., 0. );
    if ( info.loc_.isDefined() )
	wellhead.coord() = info.loc_;
    if ( !mIsUdf(info.kbelev_) )
	wellhead.z = -1. * info.kbelev_;
    Coord3 welltd( wellhead );
    welltd.z = double(info.zrg_.stop) - welltd.z;

    track.addPoint( wellhead, 0.f );
    track.addPoint( welltd, info.zrg_.stop );
    wd->info().uwid_ = info.uwi_;

    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( Well );
    PtrMan<IOObj> ioobj = mkEntry( *ctio, wd->name() );
    if ( !ioobj )
    {
	errors.add( uiStrings::phrCannotCreateDBEntryFor(
		    toUiString(wd->name())) );
	return false;
    }

    Writer ww( *ioobj.ptr(), *wd );
    if ( !ww.putInfoAndTrack() )
    {
	uiString msg = uiStrings::phrCannotCreate(
		toUiString("%1: %2").arg(wd->name()).arg(ww.errMsg()) );
	errors.add( msg );
	return false;
    }

    if ( SI().zIsTime() )
    {
	auto* d2tmodel = new D2TModel;
	d2tmodel->makeFromTrack( track,
				 uiD2TModelGroup::getDefaultTemporaryVelocity(),
				 wd->info().replvel_ );
	wd->setD2TModel( d2tmodel );
	if ( !ww.putD2T() )
	{
	    uiString msg = uiStrings::phrCannotCreate(
		    toUiString("%1: %2").arg(wd->name()).arg(ww.errMsg()) );
	    errors.add( msg );
	    return false;
	}
    }

    return true;
}


bool uiBulkLogImport::acceptOK( CallBacker* )
{
    BufferStringSet filenms;
    inpfld_->getFileNames( filenms );
    if ( filenms.isEmpty() )
    {
	uiMSG().error( tr("Please select at least one file") );
	return false;
    }

    const bool zistvdss = !ismdfld_->getBoolValue();
    const bool usecurvenms = lognmfld_->getBoolValue();
    const bool useuwiasnm = welluwinmfld_->getBoolValue();
    uiStringSet errors;
    for ( int idx=0; idx<filenms.size(); idx++ )
    {
	const BufferString& fnm = filenms.get( idx );
	LASImporter lasimp;
	LASImporter::FileInfo info;
	info.undefval_ = udffld_->getFValue();
	BufferString errmsg = lasimp.getLogInfo( fnm, info );
	if ( !errmsg.isEmpty() )
	{
	    errors.add( toUiString("%1: %2").arg(toUiString(fnm))
					    .arg(toUiString(errmsg)) );
	    continue;
	}

	const uiObject* uiobj = wellstable_->getCellObject( RowCol(idx,2) );
	mDynamicCastGet(const uiComboBox*,cb,uiobj)
	if ( cb && cb->currentItem()==0 )
	    continue;

	const BufferString wellnm = cb ? cb->text()
				       : (useuwiasnm ? info.uwi_.buf()
						     : info.wellnm_.buf() );
	PtrMan<IOObj> ioobj = findIOObj( wellnm, info.uwi_ );
	if ( !ioobj )
	{
	    if ( createNewWell(info,wellnm,errors) )
		ioobj = findIOObj( wellnm, info.uwi_ );

	    if ( !ioobj )
	    {
		errors.add(tr("%1: Cannot find %2").arg(fnm).arg(wellnm));
		continue;
	    }
	}

	RefMan<Data> wd = Well::MGR().get( ioobj->key() );
	if ( !wd )
	{
	    errors.add(tr("%1: Cannot find well information in database")
		     .arg(wellnm));
	    continue;
	}

	lasimp.setData( *wd );
	bool newwellinfo = false, adjustedwelltrack = false;
	lasimp.copyInfo( info, newwellinfo );
	lasimp.adjustTrack( info.zrg_, zistvdss, adjustedwelltrack );
	if ( usecurvenms )
	    info.lognms_ = info.logcurves_; // Hack
	errmsg = lasimp.getLogs( fnm, info, zistvdss, usecurvenms );
	if ( !errmsg.isEmpty() )
	    errors.add( toUiString("%1: %2").arg(toUiString(fnm))
					    .arg(toUiString(errmsg)) );

	Writer wtr( *ioobj, *wd );
	if ( newwellinfo || adjustedwelltrack )
	    wtr.putInfoAndTrack();
	wtr.putLogs();
    }

    if ( errors.isEmpty() )
    {
	uiMSG().message( tr("All logs imported succesfully") );
	return false;
    }

    uiMSG().errorWithDetails( errors,
		tr("Could not import all LAS files (See details)") );
    return false;
}


// uiBulkMarkerImport
uiBulkMarkerImport::uiBulkMarkerImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Multi-Well Import: Markers"),
				 mNoDlgTitle,
				 mODHelpKey(mBulkMarkerImportHelpID))
			   .modal(false))
    , fd_(BulkMarkerAscIO::getDesc())
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiFileInput( this, uiStrings::phrInput(
			       mJoinUiStrs(sMarker(),sFile())),
			       uiFileInput::Setup().withexamine(true)
			       .examstyle(File::Table) );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				       mODHelpKey(mTableImpDataSelwellsHelpID));
    dataselfld_->attach( alignedBelow, inpfld_ );
}


uiBulkMarkerImport::~uiBulkMarkerImport()
{}


bool uiBulkMarkerImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::sInvInpFile() )

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenInpFile() )

    if ( !dataselfld_->commit() )
	return false;

    BufferStringSet wellnms;
    ObjectSet<MarkerSet> markersets;
    readFile( strm, wellnms, markersets );
    if ( wellnms.isEmpty() )
	mErrRet( uiStrings::phrCannotRead(uiStrings::sFile()) )

    const ObjectSet<Table::TargetInfo>& tis = fd_->bodyinfos_;
    const bool doconv = tis.validIdx(1) && tis[1]->selection_.form_==1;

    uiStringSet errors;
    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	const BufferString& wellnm = wellnms.get(idx);
	if ( wellnm.isEmpty() ) continue;

	const PtrMan<IOObj> ioobj = findIOObj( wellnm, wellnm );
	if ( !ioobj )
	{
	    errors.add( tr("Cannot find %1 in database").arg(wellnm) );
	    continue;
	}

	RefMan<Data> wd = MGR().get( ioobj->key(), LoadReqs(Inf, Trck, Mrkrs) );
	if ( !wd )
	{
	    errors.add(tr("%1: Cannot load well").arg(wellnm));
	    continue;
	}

	if ( doconv )
	{
	    MarkerSet& ms = *markersets[idx];
	    for ( int ids=0; ids<ms.size(); ids++ )
	    {
		float dah = ms[ids]->dah();
		dah = wd->track().getDahForTVD( dah );
		ms[ids]->setDah( dah );
	    }
	}

	wd->markers() = *markersets[idx];
	Writer ww( *ioobj, *wd );
	if ( !ww.putMarkers() )
	{
	    errors.add( toUiString("%1: %2").arg(toUiString(wellnm))
					    .arg(toUiString(ww.errMsg())) );
	    continue;
	}

	wd->markerschanged.trigger();
    }

    if ( errors.isEmpty() )
    {
	uiMSG().message( tr("All markers imported succesfully") );
	return false;
    }

    uiMSG().errorWithDetails( errors,
		tr("Could not import all marker files (See details)") );
    return false;
}


void uiBulkMarkerImport::readFile( od_istream& istrm,
				   BufferStringSet& wellnms,
				   ObjectSet<MarkerSet>& markersets )
{
    BulkMarkerAscIO aio( *fd_, istrm );
    BufferString markernm, wellnm; // wellnm can be UWI as well
    float md = mUdf(float);
    while ( aio.get(wellnm,md,markernm) )
    {
	int wellidx = wellnms.indexOf( wellnm );
	if ( wellidx<0 )
	{
	    wellnms.add( wellnm );
	    markersets += new MarkerSet;
	    wellidx = wellnms.size()-1;
	}

	MarkerSet* mset = markersets[wellidx];
	Marker* marker = new Marker( markernm, md );
	mset->insertNew( marker );
    }
}


class D2TModelData
{
public:
D2TModelData( const char* wellnm )
    : wellnm_(wellnm)	{}

    BufferString	wellnm_; // can be UWI as well
    TypeSet<double>	tvds_;
    TypeSet<double>	twts_;
};

// uiBulkD2TModelImport
uiBulkD2TModelImport::uiBulkD2TModelImport( uiParent* p )
    : uiDialog(p,Setup(tr("Multi-Well Import: D2TModel"),mNoDlgTitle,
		       mODHelpKey(mBulkD2TModelImportHelpID)).modal(false))
    , fd_(BulkD2TModelAscIO::getDesc())
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiFileInput::Setup fs;
    fs.withexamine(true).examstyle(File::Table);
    inpfld_ = new uiFileInput( this,
			 uiStrings::phrInput(tr("Depth/Time Model file")), fs );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
	mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, inpfld_ );
}


uiBulkD2TModelImport::~uiBulkD2TModelImport()
{}


bool uiBulkD2TModelImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(mJoinUiStrs(sInputFile(),sName())))

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenInpFile() )

    if ( !dataselfld_->commit() )
	return false;

    ObjectSet<D2TModelData> d2tdata;
    readFile( strm, d2tdata );
    if ( d2tdata.isEmpty() )
	mErrRet( uiStrings::phrCannotRead(uiStrings::sFile()) )

    uiStringSet errors;
    for ( int idx=d2tdata.size()-1; idx>=0; idx-- )
    {
	const BufferString& wellnm = d2tdata[idx]->wellnm_;
	if ( wellnm.isEmpty() )
	{
	    delete d2tdata.removeSingle(idx,true);
	    continue;
	}

	const PtrMan<IOObj> ioobj = findIOObj( wellnm, wellnm );
	if ( !ioobj )
	{
	    delete d2tdata.removeSingle(idx,true);
	    errors.add( tr("Cannot find %1 in database").arg(wellnm) );
	    continue;
	}

	RefMan<Data> wd = MGR().get( ioobj->key(), LoadReqs(Inf, Trck, D2T) );
	if ( !wd )
	{
	    delete d2tdata.removeSingle(idx,true);
	    errors.add(tr("%1: Cannot load well").arg(wellnm));
	    continue;
	}

	uiString msg;
	D2TModel* d2t = new D2TModel( d2tdata[idx]->wellnm_.buf() );
	d2t->ensureValid( *wd, msg, &d2tdata[idx]->tvds_, &d2tdata[idx]->twts_);
	wd->setD2TModel( d2t );

	const BufferString wellfnm = ioobj->fullUserExpr();
	Writer ww( *ioobj, *wd );
	if ( !ww.putD2T() )
	{
	    delete d2tdata.removeSingle(idx,true);
	    errors.add( toUiString("%1: %2").arg(toUiString(wellnm))
					    .arg(toUiString(ww.errMsg())) );
	    continue;
	}

	wd->d2tchanged.trigger();
    }

    if ( errors.isEmpty() )
    {
	uiMSG().message( tr("All models imported succesfully") );
	return false;
    }

    uiMSG().errorWithDetails( errors,
		tr("Could not import all model files (See details)") );
    return false;
}


static int getIndex( const ObjectSet<D2TModelData>& data,
		     const BufferString& wellnm )
{
    if ( wellnm.isEmpty() ) return -1;

    for ( int idx=0; idx<data.size(); idx++ )
	if ( data[idx]->wellnm_ == wellnm )
	    return idx;

    return -1;
}


void uiBulkD2TModelImport::readFile( od_istream& istrm,
				     ObjectSet<D2TModelData>& data )
{
    BulkD2TModelAscIO aio( *fd_, istrm );
    BufferString wellnm;
    float tvd = mUdf(float);
    float twt = mUdf(float);
    while ( aio.get(wellnm,tvd,twt) )
    {
	if ( wellnm.isEmpty() )
	    continue;

	int wellidx = getIndex( data, wellnm );
	if ( wellidx<0 )
	{
	    D2TModelData* d2t = new D2TModelData( wellnm );
	    data += d2t;
	    wellidx = data.size()-1;
	}

	D2TModelData* d2t = data[wellidx];
	d2t->tvds_ += mCast(double,tvd);
	d2t->twts_ += mCast(double,twt);
    }
}


// uiBulkDirectionalImport
class DirData
{
public:
DirData( const char* wellnm )
    : wellnm_(wellnm)	{}

    BufferString	wellnm_; // can be UWI as well
    TypeSet<double>	mds_;
    TypeSet<double>	azis_;
    TypeSet<double>	incls_;
};


uiBulkDirectionalImport::uiBulkDirectionalImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Multi-Well Import: Directional Surveys"),
				 mNoDlgTitle,mTODOHelpKey)
				// mODHelpKey(mBulkDirectionalImportHelpID))
			.modal(false))
    , fd_(BulkDirectionalAscIO::getDesc())
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::Table );
    mAttachCB( inpfld_->valuechanged, uiBulkDirectionalImport::fileCB );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, inpfld_ );

    uiPushButton* applybut =
	new uiPushButton( this, uiStrings::sApply(), true );
    applybut->setIcon( "apply" );
    applybut->attach( alignedBelow, dataselfld_ );
    mAttachCB( applybut->activated, uiBulkDirectionalImport::applyCB );

    wellstable_ = new uiTable( this, uiTable::Setup(5,2), "Wells" );
    wellstable_->setColumnLabel( 0, uiStrings::sEmptyString() );
    wellstable_->setColumnLabel( 1, tr("Well name | UWI in OpendTect") );
    wellstable_->setColumnReadOnly( 0, true );
    wellstable_->attach( ensureBelow, applybut );

    mAttachCB( postFinalise(), uiBulkDirectionalImport::finalizeCB );
}


uiBulkDirectionalImport::~uiBulkDirectionalImport()
{
    detachAllNotifiers();
    deepErase( dirdatas_ );
}


void uiBulkDirectionalImport::finalizeCB( CallBacker* )
{
    uiButton* okbut = button(uiDialog::OK);
    progressbar_ = new uiProgressBar( okbut->parent(), "Importing Data" );
    progressbar_->setStretch( 2, 0 );
    progressbar_->attach( leftOf, okbut );
}


void uiBulkDirectionalImport::reset()
{
    deepErase( dirdatas_ );
    wellstable_->clearTable();
    wellstable_->setNrRows( 5 );
    progressbar_->setProgress( 0 );
}


void uiBulkDirectionalImport::fileCB( CallBacker* )
{
    reset();
}


void uiBulkDirectionalImport::applyCB( CallBacker* )
{
    MouseCursorChanger mcc( MouseCursor::Wait );
    readFile();
    fillTable();
}


bool uiBulkDirectionalImport::acceptOK( CallBacker* )
{
    if ( dirdatas_.isEmpty() )
    {
	uiString msg = tr("No data available to import.\n"
		"Please select file and press Apply");
	return false;
    }

    progressbar_->setTotalSteps( dirdatas_.size() );
    uiStringSet errors;
    for ( int idx=0; idx<dirdatas_.size(); idx++ )
    {
	progressbar_->setProgress( idx+1 );

	const DirData* dd = dirdatas_[idx];
	const uiObject* uiobj = wellstable_->getCellObject( RowCol(idx,1) );
	mDynamicCastGet(const uiComboBox*,cb,uiobj)
	if ( !cb || cb->currentItem()==0 )
	    continue;

	const StringPair cbtxt( cb->text() );
	BufferString wellnm = cbtxt.first(); wellnm.trimBlanks();
	BufferString uwi = cbtxt.second(); uwi.trimBlanks();
	const PtrMan<IOObj> ioobj =
		findIOObj( wellnm, uwi.isEmpty() ? wellnm : uwi );
	if ( !ioobj )
	{
	    errors.add( tr("Cannot find %1 in database").arg(wellnm) );
	    continue;
	}

	RefMan<Data> wd = MGR().get( ioobj->key(), LoadReqs(Inf, Trck) );
	if ( !wd )
	{
	    errors.add(tr("%1: Cannot load well").arg(wellnm));
	    continue;
	}

	TypeSet<Coord3> track;
	const float kb = wd->track().getKbElev();
	DirectionalSurvey dirsurvey( wd->info().surfacecoord_, kb );
	dirsurvey.calcTrack( dd->mds_, dd->incls_, dd->azis_, track );

	wd->track().setEmpty();
	for ( int idz=0; idz<dd->mds_.size(); idz++ )
	    wd->track().addPoint( track[idz], mCast(float,dd->mds_[idz]) );

	Writer ww( *ioobj, *wd );
	if ( !ww.putInfoAndTrack() )
	    errors.add( ww.errMsg() );
    }

    if ( errors.isEmpty() )
    {
	uiMSG().message( tr("All surveys imported succesfully") );
	return false;
    }

    uiMSG().errorWithDetails( errors,
		tr("Could not import all surveys (See details)") );
    return false;
}


static int getIndex( const ObjectSet<DirData>& data,
		     const BufferString& wellnm )
{
    if ( wellnm.isEmpty() ) return -1;

    for ( int idx=0; idx<data.size(); idx++ )
	if ( data[idx]->wellnm_ == wellnm )
	    return idx;

    return -1;
}


bool uiBulkDirectionalImport::readFile()
{
    reset();

    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(mJoinUiStrs(sInputFile(),sName())))

    od_istream istrm( fnm );
    if ( !istrm.isOK() )
	mErrRet( uiStrings::sCantOpenInpFile() )

    if ( !dataselfld_->commit() )
	return false;

    BulkDirectionalAscIO aio( *fd_, istrm );
    BufferString wellnm;
    double md = mUdf(double);
    double incl = mUdf(double);
    double azi = mUdf(double);
    while ( aio.get(wellnm,md,incl,azi) )
    {
	if ( wellnm.isEmpty() )
	    continue;

	DirData* dd = nullptr;
	const int wellidx = getIndex( dirdatas_, wellnm );
	if ( wellidx<0 )
	{
	    dd = new DirData( wellnm );
	    dirdatas_ += dd;
	}
	else
	    dd = dirdatas_[wellidx];

	dd->mds_ += md;
	dd->incls_ += incl;
	dd->azis_ += azi;
    }

    fromuwi_ = aio.identifierIsUWI();
    BufferString lbl = fromuwi_ ? "UWI" : "Well name";
    lbl.add( " in file" );
    wellstable_->setColumnLabel( 0, toUiString(lbl) );

    return true;
}


void uiBulkDirectionalImport::fillTable()
{
    BufferStringSet wellnms;
    wellnms.add( "-- (Do not import)" );
    getWellNames( wellnms, true );

    wellstable_->setNrRows( dirdatas_.size() );
    for ( int idx=0; idx<dirdatas_.size(); idx++ )
    {
	const BufferString& wellnm = dirdatas_[idx]->wellnm_;
	wellstable_->setText( RowCol(idx,0), wellnm );

	uiComboBox* wellsbox = new uiComboBox( 0, "Select Well" );
	wellsbox->addItems( wellnms );
	wellstable_->setCellObject( RowCol(idx,1), wellsbox );

	GlobExpr ge( BufferString("*",wellnm,"*").buf() );
	const int selidx = wellnms.indexOf( ge );
	wellsbox->setCurrentItem( selidx<0 ? 0 : selidx );
    }

    wellstable_->resizeColumnToContents( 0 );
}

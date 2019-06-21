/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2012
________________________________________________________________________

-*/


#include "uibulkwellimp.h"

#include "ctxtioobj.h"
#include "dbdir.h"
#include "ioobj.h"
#include "ptrman.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"
#include "veldesc.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "wellmanager.h"
#include "wellmarker.h"
#include "welltrack.h"
#include "wellinfo.h"
#include "welltransl.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uiconstvel.h"
#include "uifiledlg.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uid2tmodelgrp.h"
#include "od_helpids.h"


static DBKey getDBKey( const char* wellnm, const char* welluwi=0 )
{
    const bool nmpassed = wellnm && *wellnm;
    const bool uwipassed = welluwi && *welluwi;
    DBKey dbky;
    if ( nmpassed )
	dbky = Well::MGR().getIDByName( wellnm );
    if ( dbky.isInvalid() )
    {
	if ( uwipassed )
	    dbky = Well::MGR().getIDByUWI( welluwi );
	else if ( nmpassed )
	    dbky = Well::MGR().getIDByUWI( wellnm );
    }
    return dbky;
}


// uiBulkTrackImport
uiBulkTrackImport::uiBulkTrackImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Multi-Well Import: Well Tracks"),
				 mNoDlgTitle,
				 mODHelpKey(mBulkTrackImportHelpID) )
			   .modal(false))
    , fd_(Well::BulkTrackAscIO::getDesc())
    , velocityfld_(0)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiFileSel( this, uiStrings::sInputFile(), uiFileSel::Setup()
		      .withexamine(true).examstyle(File::Table) );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				       mODHelpKey(mTableImpDataSelwellsHelpID));
    dataselfld_->attach( alignedBelow, inpfld_ );

    if ( SI().zIsTime() )
    {
	const float vel = uiD2TModelGroup::getDefaultTemporaryVelocity();
	const uiString vellbl( uiD2TModelGroup::sKeyTemporaryVel() );
	velocityfld_ = new uiConstantVel( this, vel, vellbl );
	velocityfld_->attach( alignedBelow, dataselfld_ );
    }
}


uiBulkTrackImport::~uiBulkTrackImport()
{
    delete fd_;
    deepUnRef( wells_ );
}


static int getWellIdx( ObjectSet<Well::Data>& wells, const char* nm )
{
    for ( int idx=0; idx<wells.size(); idx++ )
    {
	if ( wells[idx]->info().hasName(nm) )
	    return idx;
    }

    return -1;
}


static bool getInfo( const char* wellnm, float& kb, Well::Info& info )
{
    DBKey dbky = getDBKey( wellnm, info.UWI() );
    if ( !dbky.isValid() )
	return false;

    ConstRefMan<Well::Data> wd =
	Well::MGR().fetch( dbky, Well::LoadReqs(Well::Inf,Well::Trck) );
    if ( !wd )
	return false;

    kb = wd->track().getKbElev();
    if ( mIsUdf(kb) )
	kb = 0;

    IOPar pars;
    wd->info().fillPar( pars );
    info.usePar( pars );
    return true;
}


void uiBulkTrackImport::readFile( od_istream& istrm )
{
    Well::BulkTrackAscIO aio( *fd_, istrm );
    BufferString wellnm, uwi; Coord3 crd;
    float md = mUdf(float);
    const bool depthistvd = aio.depthIsTVD();
    float kb = 0;
    while ( aio.get(wellnm,crd,md,uwi) )
    {
	if ( wellnm.isEmpty() )
	    continue;

	Well::Data* wd = 0;
	const int widx = getWellIdx( wells_, wellnm );
	if ( wells_.validIdx(widx) )
	    wd = wells_[widx];
	else
	{
	    wd = new Well::Data( wellnm );
	    wd->ref();
	    getInfo( wellnm, kb, wd->info() );
	    if ( !uwi.isEmpty() )
		wd->info().setUWI( uwi );

	    wells_ += wd;
	}

	if ( depthistvd )
	    crd.z_ -= kb;

	if ( crd.isDefined() )
	    wd->track().addPoint( crd.getXY(), (float)crd.z_, md );
    }
}


uiRetVal uiBulkTrackImport::addD2T()
{
    if ( !SI().zIsTime() )
	return uiRetVal::OK();

    const float vel = velocityfld_->getInternalVelocity();
    if ( vel<=0 || mIsUdf(vel) )
	return uiStrings::phrEnter(tr("a positive velocity "
			    "for generating the D2T model"));

    const float twtvel = vel * .5f;
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	Well::Data& wd = *wells_[idx];
	const Well::Track& track = wd.track();

	const float srd = mCast(float,SI().seismicReferenceDatum());
	const float zstart = track.zRange().start;
	const float zstop = track.zRange().stop;
	const float dahstart = track.dahRange().start;
	const float dahstop = track.dahRange().stop;

	Well::D2TModel& d2t = wd.d2TModel();
	d2t.setValueAt( dahstart, (zstart+srd)/twtvel );
	d2t.setValueAt( dahstop, (zstop+srd)/twtvel );
    }

    return uiRetVal::OK();
}


uiRetVal uiBulkTrackImport::write()
{
    uiRetVal uirv;
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	Well::Data* wd = wells_[idx];
	const DBKey dbky = getDBKey( wd->name() );
	SilentTaskRunnerProvider trprov;
	if ( dbky.isValid() )
	    uirv.add( uiString( tr("A well named '%1' already exists.")
				.arg(wd->name()) ) );
	else
	    uirv.add( Well::MGR().save(*wd,trprov) );
    }
    return uirv;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiBulkTrackImport::acceptOK()
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrInput(uiStrings::sFileName()) )

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::phrCannotOpenInpFile() )

    if ( !dataselfld_->commit() )
	return false;

    readFile( strm );
    if ( wells_.isEmpty() )
	{ uiMSG().message( tr("No valid lines found") ); return true; }

    uiRetVal uirv = addD2T();
    if ( uirv.isError() )
	mErrRet( uirv );

    uirv = write();
    if ( !uirv.isOK() )
	mErrRet( uirv );

    uiMSG().message( tr("All tracks imported succesfully") );
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

    uiFileSel::Setup fssu;
    fssu.selectMultiFile()
	.setFormat( tr("LAS files"), "las", "dat" );
    inpfld_ = new uiFileSel( this, uiStrings::phrInput(tr("LAS files")), fssu );
    inpfld_->newSelection.notify( mCB(this,uiBulkLogImport,lasSel) );

    istvdfld_ = new uiGenInput( this, tr("Depth values are"),
		    BoolInpSpec(false,uiStrings::sTVDSS(),uiStrings::sMD()) );
    istvdfld_->attach( alignedBelow, inpfld_ );

    const float defundefval = -999.25;
    udffld_ = new uiGenInput( this, tr("Undefined value in logs"),
		    FloatInpSpec(defundefval));
    udffld_->attach( alignedBelow, istvdfld_ );

    lognmfld_ = new uiGenInput( this, tr("Name log after"),
	    BoolInpSpec(true,uiStrings::sCurve(),uiStrings::sDescription()) );
    lognmfld_->attach( alignedBelow, udffld_ );

    uiStringSet colnms;
    colnms.add( tr("Well name in LAS") ).add( tr("Well name in OpendTect") );
    wellstable_ = new uiTable( this, uiTable::Setup(3,2), "Wells" );
    wellstable_->setColumnLabels( colnms );
    wellstable_->setColumnReadOnly( 0, true );
    wellstable_->attach( ensureBelow, lognmfld_ );
}


uiBulkLogImport::~uiBulkLogImport()
{}


static void getWellNames( BufferStringSet& wellnms )
{
    const IOObjContext ctxt = mIOObjContext( Well );
    DBDirEntryList list( ctxt );
    for ( int idx=0; idx<list.size(); idx++ )
	wellnms.add( list.name(idx) );
}


void uiBulkLogImport::lasSel( CallBacker* )
{
    BufferStringSet filenms;
    inpfld_->getFileNames( filenms );
    wellstable_->setNrRows( filenms.size() );

    BufferStringSet wellnms;
    wellnms.add( "-- (Do not import)" );
    getWellNames( wellnms );
    for ( int idx=0; idx<filenms.size(); idx++ )
    {
	const BufferString& fnm = filenms.get( idx );
	Well::LASImporter lasimp;
	Well::LASImporter::FileInfo info;
	info.undefval = udffld_->getFValue();
	BufferString errmsg = lasimp.getLogInfo( fnm, info );

	wellstable_->setText( RowCol(idx,0), info.wellnm );

	uiComboBox* wellsbox = new uiComboBox( 0, "Select Well" );
	wellsbox->addItems( wellnms );
	wellstable_->setCellObject( RowCol(idx,1), wellsbox );
	const int selidx = wellnms.nearestMatch( info.wellnm );
	wellsbox->setCurrentItem( selidx<0 ? 0 : selidx );
    }
}


bool uiBulkLogImport::acceptOK()
{
    BufferStringSet filenms;
    inpfld_->getFileNames( filenms );
    if ( filenms.isEmpty() )
    {
	uiMSG().error( uiStrings::phrPlsSelectAtLeastOne(uiStrings::sFile()) );
	return false;
    }

    const bool zistvd = istvdfld_->getBoolValue();
    const bool usecurvenms = lognmfld_->getBoolValue();
    uiRetVal uirv;
    for ( int idx=0; idx<filenms.size(); idx++ )
    {
	const BufferString& fnm = filenms.get( idx );
	Well::LASImporter lasimp;
	Well::LASImporter::FileInfo info;
	info.undefval = udffld_->getFValue();
	BufferString errmsg = lasimp.getLogInfo( fnm, info );
	if ( !errmsg.isEmpty() )
	{
	    uirv.add( toUiString("%1: %2").arg(toUiString(fnm))
					    .arg(toUiString(errmsg)) );
	    continue;
	}

	const uiObject* uiobj = wellstable_->getCellObject( RowCol(idx,1) );
	mDynamicCastGet(const uiComboBox*,cb,uiobj)
	if ( cb && cb->currentItem()==0 )
	    continue;

	const BufferString wellnm = cb ? cb->text() : info.wellnm.buf();
	DBKey dbky = getDBKey( wellnm, info.uwi );
	if ( dbky.isInvalid() )
	{
	    uirv.add( tr("%1: Cannot find %2").arg(fnm).arg(
			wellnm.isEmpty() ? info.uwi : info.wellnm));
	    continue;
	}

	uiRetVal newuirv;
	RefMan<Well::Data> wd = Well::MGR().fetchForEdit( dbky,
				    Well::LoadReqs(Well::Logs), newuirv );
	if ( !wd )
	    { uirv.add( newuirv ); continue; }

	lasimp.setData( *wd );
	if ( usecurvenms )
	    info.lognms = info.logcurves; // Hack
	errmsg = lasimp.getLogs( fnm, info, zistvd, usecurvenms );
	SilentTaskRunnerProvider trprov;
	if ( !errmsg.isEmpty() )
	    uirv.add( toUiString("%1: %2").arg(toUiString(fnm))
					    .arg(toUiString(errmsg)) );
	else
	    uirv.add( Well::MGR().save(dbky,trprov) );
    }

    if ( uirv.isEmpty() )
    {
	uiMSG().message( tr("All logs imported succesfully") );
	return false;
    }

    uiMSG().errorWithDetails( uirv,
		uiStrings::phrCannotImport(tr("all LAS files (See details)")) );
    return false;
}


// uiBulkMarkerImport
uiBulkMarkerImport::uiBulkMarkerImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Multi-Well Import: Markers"),
				 mNoDlgTitle,
				 mODHelpKey(mBulkMarkerImportHelpID))
			   .modal(false))
    , fd_(Well::BulkMarkerAscIO::getDesc())
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiFileSel::Setup fssu;
    fssu.withexamine(true).examstyle(File::Table);
    inpfld_ = new uiFileSel( this, uiStrings::phrInput(tr("Marker File")),
									fssu );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				       mODHelpKey(mTableImpDataSelwellsHelpID));
    dataselfld_->attach( alignedBelow, inpfld_ );
}


uiBulkMarkerImport::~uiBulkMarkerImport()
{}


bool uiBulkMarkerImport::acceptOK()
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::sInvInpFile() )

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::phrCannotOpenInpFile() )

    if ( !dataselfld_->commit() )
	return false;

    BufferStringSet wellnms;
    ObjectSet<Well::MarkerSet> markersets;
    readFile( strm, wellnms, markersets );
    if ( wellnms.isEmpty() )
	mErrRet( uiStrings::phrCannotRead(uiStrings::sFile()) )

    const ObjectSet<Table::TargetInfo>& tis = fd_->bodyinfos_;
    const bool doconv = tis.validIdx(1) && tis[1]->selection_.form_==1;

    uiRetVal uirv;
    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	const BufferString& wellnm = wellnms.get(idx);
	if ( wellnm.isEmpty() )
	    continue;

	const DBKey dbky = getDBKey( wellnm );
	if ( dbky.isInvalid() )
	    { uirv.add( tr("Cannot find well '%1'").arg(wellnm) ); continue; }

	uiRetVal fetchres;
	RefMan<Well::Data> wd = Well::MGR().fetchForEdit( dbky,
					Well::LoadReqs(), fetchres );
	if ( !wd )
	    { uirv.add( fetchres ); continue; }

	if ( !doconv )
	    wd->markers() = *markersets[idx];
	else
	{
	    RefMan<Well::MarkerSet> convms = new Well::MarkerSet(
						    *markersets[idx] );
	    Well::MarkerSetIter4Edit msiter( *convms );
	    while( msiter.next() )
	    {
		float dah = msiter.getDah();
		dah = wd->track().getDahForTVD( dah );
		convms->setDah( msiter.ID(), dah );
	    }
	    wd->markers() = *convms;
	}

	SilentTaskRunnerProvider trprov;
	uirv.add( Well::MGR().save(dbky,trprov) );
    }

    if ( uirv.isError() )
	uiMSG().error( uirv );
    else
	uiMSG().message( tr("All markers imported succesfully") );

    return false;
}


void uiBulkMarkerImport::readFile( od_istream& istrm,
				   BufferStringSet& wellnms,
				   ObjectSet<Well::MarkerSet>& markersets )
{
    Well::BulkMarkerAscIO aio( *fd_, istrm );
    BufferString markernm, wellnm; // wellnm can be UWI as well
    float md = mUdf(float);
    while ( aio.get(wellnm,md,markernm) )
    {
	int wellidx = wellnms.indexOf( wellnm );
	if ( wellidx<0 )
	{
	    wellnms.add( wellnm );
	    markersets += new Well::MarkerSet;
	    wellidx = wellnms.size()-1;
	}

	Well::MarkerSet* mset = markersets[wellidx];
	Well::Marker marker( markernm, md );
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
    : uiDialog(p,uiDialog::Setup(tr("Multi-Well Import: D2TModel"),
    mNoDlgTitle, mODHelpKey(mBulkD2TModelImportHelpID)).modal(false))
    , fd_(Well::BulkD2TModelAscIO::getDesc())
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    uiFileSel::Setup fssu;
    fssu.withexamine(true).examstyle(File::Table);
    inpfld_ = new uiFileSel( this,
		     uiStrings::phrInput(tr("Depth/Time Model file")), fssu );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
	mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, inpfld_ );
}


uiBulkD2TModelImport::~uiBulkD2TModelImport()
{}


bool uiBulkD2TModelImport::acceptOK()
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("Input File Name")) )

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::phrCannotOpenInpFile() )

    if ( !dataselfld_->commit() )
	return false;

    ObjectSet<D2TModelData> d2tdata;
    readFile( strm, d2tdata );
    if ( d2tdata.isEmpty() )
	mErrRet( uiStrings::phrCannotRead(uiStrings::sFile()) )

    uiRetVal uirv;
    for ( int idx=d2tdata.size()-1; idx>=0; idx-- )
    {
	const BufferString& wellnm = d2tdata[idx]->wellnm_;
	if ( wellnm.isEmpty() )
	    { delete d2tdata.removeSingle(idx,true); continue; }

	const DBKey dbky = getDBKey( wellnm );
	if ( dbky.isInvalid() )
	{
	    uirv.add( tr("Cannot find well '%1'").arg(wellnm) );
	    delete d2tdata.removeSingle(idx,true);
	    continue;
	}

	RefMan<Well::Data> wd = Well::MGR().fetchForEdit( dbky );
	if ( !wd )
	{
	    delete d2tdata.removeSingle(idx,true);
	    uirv.add(tr("%1: Cannot load well").arg(wellnm));
	    continue;
	}

	Well::D2TModel& d2t = wd->d2TModel();
	uiString msg;
	d2t.ensureValid( *wd, msg, &d2tdata[idx]->tvds_, &d2tdata[idx]->twts_ );
	SilentTaskRunnerProvider trprov;
	uirv.add( Well::MGR().save(dbky,trprov) );
    }

    if ( uirv.isError() )
	uiMSG().error( uirv );
    else
	uiMSG().message( tr("All models imported succesfully") );

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
    Well::BulkD2TModelAscIO aio( *fd_, istrm );
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

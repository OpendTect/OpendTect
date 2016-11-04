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
#include "dbman.h"
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
#include "uifiledlg.h"
#include "uifileinput.h"
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
    inpfld_ = new uiFileInput( this,
		      uiStrings::sInputFile(),
		      uiFileInput::Setup()
		      .withexamine(true).examstyle(File::Table) );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
                                       mODHelpKey(mTableImpDataSelwellsHelpID));
    dataselfld_->attach( alignedBelow, inpfld_ );

    if ( SI().zIsTime() )
    {
	const uiString vellbl = tr("Temporary model velocity %1" )
				  .arg( VelocityDesc::getVelUnit( true ));
	const float vel = getGUIDefaultVelocity();
	velocityfld_ = new uiGenInput( this, vellbl, FloatInpSpec(vel) );
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
	if ( wells[idx]->info().name() == nm )
	    return idx;
    }

    return -1;
}


void uiBulkTrackImport::readFile( od_istream& istrm )
{
    Well::BulkTrackAscIO aio( *fd_, istrm );
    BufferString wellnm, uwi; Coord3 crd;
    float md = mUdf(float);
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
	    wd->info().setUWI( uwi );
	    wd->ref();
	    wells_ += wd;
	}

	if ( crd.isDefined() )
	    wd->track().addPoint( crd.getXY(), (float) crd.z_, md );
    }
}


uiRetVal uiBulkTrackImport::addD2T()
{
    if ( !SI().zIsTime() )
	return uiRetVal::OK();

    const float vel = velocityfld_->getFValue();
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
	if ( dbky.isValid() )
	    uirv.add( uiString( tr("A well named '%1' already exists.")
				.arg(wd->name()) ) );
	else
	    uirv.add( Well::MGR().save(*wd) );
    }
    return uirv;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiBulkTrackImport::acceptOK()
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrInput(mJoinUiStrs(sFile(),sName())) )
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenInpFile() )
    if ( !dataselfld_->commit() )
	return false;

    readFile( strm );
    if ( wells_.isEmpty() )
    {
	uiMSG().message( tr("No valid lines found") );
	return true;
    }

    uiRetVal uirv = addD2T();
    if ( !uirv.isError() )
	mErrRet( uirv );

    uirv = write();
    if ( !uirv.isOK() )
	{ uiMSG().error( uirv ); return false; }

    uiMSG().message( tr("All tracks imported succesfully") );
    return true;
}


// uiBulkLogImport
uiBulkLogImport::uiBulkLogImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Multi-Well Import: Logs"),
				 mNoDlgTitle,
				 mODHelpKey(mBulkLogImportHelpID))
			   .modal(false))
{
    inpfld_ = new uiFileInput( this, uiStrings::phrInput(tr("LAS files")),
		  uiFileInput::Setup() );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );

    istvdfld_ = new uiGenInput( this, tr("Depth values are"),
		    BoolInpSpec(false,tr("TVDSS"),tr("MD")) );
    istvdfld_->attach( alignedBelow, inpfld_ );

    const float defundefval = -999.25;
    udffld_ = new uiGenInput( this, tr("Undefined value in logs"),
		    FloatInpSpec(defundefval));
    udffld_->attach( alignedBelow, istvdfld_ );
}


uiBulkLogImport::~uiBulkLogImport()
{}


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

	DBKey dbky = getDBKey( info.wellnm, info.uwi );
	if ( dbky.isInvalid() )
	{
	    uirv.add( tr("%1: Cannot find %2").arg(fnm).arg(
			info.wellnm.isEmpty() ? info.uwi : info.wellnm));
	    continue;
	}

	uiRetVal newuirv;
	RefMan<Well::Data> wd = Well::MGR().fetchForEdit( dbky,
				    Well::LoadReqs(Well::Logs), uirv );
	if ( !wd )
	    { uirv.add( uirv ); continue; }

	lasimp.setData( *wd );
	errmsg = lasimp.getLogs( fnm, info, zistvd );
	if ( !errmsg.isEmpty() )
	    uirv.add( toUiString("%1: %2").arg(toUiString(fnm))
					    .arg(toUiString(errmsg)) );
	else
	    uirv.add( Well::MGR().save(dbky) );
    }

    if ( uirv.isEmpty() )
    {
	uiMSG().message( tr("All logs imported succesfully") );
	return true;
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


bool uiBulkMarkerImport::acceptOK()
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

	uirv.add( Well::MGR().save( dbky ) );
    }

    if ( uirv.isError() )
	uiMSG().error( uirv );
    else
	uiMSG().message( tr("All markers imported succesfully") );

    return uirv.isOK();
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
    TypeSet<double>	mds_;
    TypeSet<double>	twts_;
};

// uiBulkD2TModelImport
uiBulkD2TModelImport::uiBulkD2TModelImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Multi-Well Import: D2TModel"),
    mNoDlgTitle, mODHelpKey(mBulkD2TModelImportHelpID)).modal(false))
    , fd_(Well::BulkD2TModelAscIO::getDesc())
{
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


bool uiBulkD2TModelImport::acceptOK()
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
	d2t.ensureValid( *wd, msg, &d2tdata[idx]->mds_, &d2tdata[idx]->twts_ );
	uirv.add( Well::MGR().save( dbky ) );
    }

    if ( uirv.isError() )
	uiMSG().error( uirv );
    else
	uiMSG().message( tr("All models imported succesfully") );

    return uirv.isOK();
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
    float md = mUdf(float);
    float twt = mUdf(float);
    while ( aio.get(wellnm,md,twt) )
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
	d2t->mds_ += mCast(double,md);
	d2t->twts_ += mCast(double,twt);
    }
}

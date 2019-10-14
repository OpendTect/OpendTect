/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2012
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "uibulkwellimp.h"

#include "ctxtioobj.h"
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
#include "welldata.h"
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


static IOObj* mkEntry( const CtxtIOObj& ctio, const char* nm )
{
    CtxtIOObj newctio( ctio );
    newctio.ioobj_ = 0; newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj_;
}


static bool getInfo( const char* wellnm, float& kb, Well::Info& info )
{
    PtrMan<IOObj> ioobj = findIOObj( wellnm, wellnm );
    if ( !ioobj ) return false;

    RefMan<Well::Data> wd = new Well::Data;
    Well::Reader wr( *ioobj, *wd );
    const bool res = wr.getInfo() && wr.getTrack();
    if ( !res ) return false;

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
    BulkTrackAscIO aio( *fd_, istrm );
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
		wd->info().uwid = uwi;

	    wells_ += wd;
	}

	if ( depthistvd )
	    crd.z -= kb;

	if ( crd.isDefined() )
	    wd->track().addPoint( crd.coord(), (float) crd.z, md );
    }
}


void uiBulkTrackImport::addD2T( uiString& errmsg )
{
    if ( !SI().zIsTime() ) return;

    const float vel = velocityfld_->getfValue();
    if ( vel<=0 || mIsUdf(vel) )
    {
	errmsg = tr("Please enter a positive velocity "
		    "for generating the D2T model");
	return;
    }

    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	RefMan<Well::Data> wd = wells_[idx];
	Well::D2TModel* d2t = new Well::D2TModel;
	d2t->makeFromTrack(  wd->track(), vel, wd->info().replvel );
	wd->setD2TModel( d2t );
    }
}


void uiBulkTrackImport::write( uiStringSet& errors )
{
    // TODO: Check if name exists, ask user to overwrite or give new name
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( Well );
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	RefMan<Well::Data> wd = wells_[idx];
	PtrMan<IOObj> ioobj = IOM().getLocal(wd->name(),
					     ctio->ctxt_.trgroup_->groupName());
	if ( !ioobj )
	    ioobj = mkEntry( *ctio, wd->name() );
	if ( !ioobj )
	{
	    errors.add( uiStrings::phrCannotCreateDBEntryFor(
						    toUiString(wd->name())) );
	    continue;
	}

	Well::Writer ww( *ioobj, *wd );
	if ( !ww.put() )
	{
	    uiString msg = uiStrings::phrCannotCreate(
		    toUiString("%1: %2").arg(wd->name()).arg(ww.errMsg()) );
	    errors.add( msg );
	}

	const bool isloaded = MGR().isLoaded( ioobj->key() );
	if ( isloaded && MGR().reload(ioobj->key()) )
	{
	    Data* loadedwd = MGR().get( ioobj->key() );
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

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenInpFile() )

    if ( !dataselfld_->commit() )
	return false;

    readFile( strm );
    uiString errmsg;
    addD2T( errmsg );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg );

    uiStringSet errors;
    write( errors );

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

    inpfld_ = new uiFileInput( this, uiStrings::phrInput(tr("LAS files")),
		  uiFileInput::Setup() );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    inpfld_->valuechanged.notify( mCB(this,uiBulkLogImport,lasSel) );

    istvdfld_ = new uiGenInput( this, tr("Depth values are"),
		    BoolInpSpec(false,tr("TVDSS"),tr("MD")) );
    istvdfld_->attach( alignedBelow, inpfld_ );

    const float defundefval = -999.25;
    udffld_ = new uiGenInput( this, tr("Undefined value in logs"),
		    FloatInpSpec(defundefval));
    udffld_->attach( alignedBelow, istvdfld_ );

    lognmfld_ = new uiGenInput( this, tr("Name log after"),
		BoolInpSpec(false,tr("Curve"),tr("Description")) );
    lognmfld_->attach( alignedBelow, udffld_ );

    BufferStringSet colnms;
    colnms.add( "Well name in LAS" ).add( "Well name in OpendTect" );
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
    const IODir iodir( ctxt.getSelKey() );
    IODirEntryList list( iodir, ctxt );
    list.sort();
    list.getIOObjNames( wellnms );
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
	info.undefval = udffld_->getfValue();
	BufferString errmsg = lasimp.getLogInfo( fnm, info );

	wellstable_->setText( RowCol(idx,0), info.wellnm );

	uiComboBox* wellsbox = new uiComboBox( 0, "Select Well" );
	wellsbox->addItems( wellnms );
	wellstable_->setCellObject( RowCol(idx,1), wellsbox );
	const int selidx = wellnms.nearestMatch( info.wellnm );
	wellsbox->setCurrentItem( selidx<0 ? 0 : selidx );
    }
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

    const bool zistvd = istvdfld_->getBoolValue();
    const bool usecurvenms = lognmfld_->getBoolValue();
    uiStringSet errors;
    for ( int idx=0; idx<filenms.size(); idx++ )
    {
	const BufferString& fnm = filenms.get( idx );
	Well::LASImporter lasimp;
	Well::LASImporter::FileInfo info;
	info.undefval = udffld_->getfValue();
	BufferString errmsg = lasimp.getLogInfo( fnm, info );
	if ( !errmsg.isEmpty() )
	{
	    errors.add( toUiString("%1: %2").arg(toUiString(fnm))
					    .arg(toUiString(errmsg)) );
	    continue;
	}

	const uiObject* uiobj = wellstable_->getCellObject( RowCol(idx,1) );
	mDynamicCastGet(const uiComboBox*,cb,uiobj)
	if ( cb && cb->currentItem()==0 )
	    continue;

	const BufferString wellnm = cb ? cb->text() : info.wellnm.buf();
	const IOObj* ioobj = findIOObj( wellnm, info.uwi );
	if ( !ioobj )
	{
	    errors.add(tr("%1: Cannot find %2").arg(fnm).arg(wellnm));
	    continue;
	}

	RefMan<Well::Data> wd = Well::MGR().get( ioobj->key() );
	if ( !wd )
	{
	    errors.add(tr("%1: Cannot find well information in database")
		     .arg(info.wellnm));
	    continue;
	}

	lasimp.setData( wd );
	if ( usecurvenms )
	    info.lognms = info.logcurves; // Hack
	errmsg = lasimp.getLogs( fnm, info, zistvd, usecurvenms );
	if ( !errmsg.isEmpty() )
	    errors.add( toUiString("%1: %2").arg(toUiString(fnm))
					    .arg(toUiString(errmsg)) );

	Well::Writer wtr( *ioobj, *wd );
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

	RefMan<Well::Data> wd = MGR().get( ioobj->key() );
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
	Well::Writer ww( *ioobj, *wd );
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

	const IOObj* ioobj = findIOObj( wellnm, wellnm );
	if ( !ioobj )
	{
	    delete d2tdata.removeSingle(idx,true);
	    errors.add( tr("Cannot find %1 in database").arg(wellnm) );
	    continue;
	}

	RefMan<Well::Data> wd = MGR().get( ioobj->key() );
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

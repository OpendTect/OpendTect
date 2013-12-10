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
#include "hiddenparam.h"
#include "iodir.h"
#include "iodirentry.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"
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
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uid2tmodelgrp.h"

using namespace Well;

HiddenParam<uiBulkTrackImport,uiGenInput*> velflds( 0 );

// uiBulkTrackImport
uiBulkTrackImport::uiBulkTrackImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Well Track Import",mNoDlgTitle,"107.0.10"))
    , fd_(BulkTrackAscIO::getDesc())
{
    inpfld_ = new uiFileInput( this, "Input file", uiFileInput::Setup()
		      .withexamine(true).examstyle(uiFileInput::Setup::Table) );

    dataselfld_ = new uiTableImpDataSel( this, *fd_, "107.0.9" );
    dataselfld_->attach( alignedBelow, inpfld_ );

    velflds.setParam( this, 0 );
    if ( SI().zIsTime() )
    {
	const BufferString zlbl( SI().depthsInFeet() ? " (ft" : " (m", "/s)" );
	const BufferString vellbl( "Temporary model velocity", zlbl );
	const float vel = getGUIDefaultVelocity();
	uiGenInput* velocityfld = new uiGenInput( this, vellbl,
							FloatInpSpec(vel) );
	velocityfld->attach( alignedBelow, dataselfld_ );
	velflds.setParam( this, velocityfld );
    }
}


uiBulkTrackImport::~uiBulkTrackImport()
{
    delete fd_;
    deepErase( wells_ );
}


static int getWellIdx( ObjectSet<Data>& wells, const char* nm )
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
    newctio.ioobj = 0; newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj;
}


void uiBulkTrackImport::readFile( std::istream& istrm )
{
    BulkTrackAscIO aio( *fd_, istrm );
    BufferString wellnm, uwi; Coord3 crd;
    float md = mUdf(float);
    while ( aio.get(wellnm,crd,md,uwi) )
    {
	if ( wellnm.isEmpty() )
	    continue;

	Data* wd = 0;
	const int widx = getWellIdx( wells_, wellnm );
	if ( wells_.validIdx(widx) )
	    wd = wells_[widx];
	else
	{
	    wd = new Data( wellnm );
	    wd->info().uwid = uwi;
	    wells_ += wd;
	}

	if ( crd.isDefined() )
	    wd->track().addPoint( crd.coord(), (float) crd.z, md );
    }
}


void uiBulkTrackImport::addD2T( BufferString& errmsg )
{
    if ( !SI().zIsTime() ) return;

    uiGenInput* velocityfld = velflds.getParam( this );
    const float vel = velocityfld ? velocityfld->getfValue() : mUdf(float);
    if ( vel<=0 || mIsUdf(vel) )
    {
	errmsg =
	    "Please enter a positive velocity for generating the D2T model";
	return;
    }

    const float twtvel = vel * .5f;
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	Data* wd = wells_[idx];
	const Track& track = wd->track();

	const float srd = mCast(float,SI().seismicReferenceDatum());
	const float zstart = track.zRange().start;
	const float zstop = track.zRange().stop;
	const float dahstart = track.dahRange().start;
	const float dahstop = track.dahRange().stop;

	D2TModel* d2t = new D2TModel;
	d2t->add( dahstart, (zstart+srd)/twtvel );
	d2t->add( dahstop, (zstop+srd)/twtvel );
	wd->setD2TModel( d2t );
    }
}


void uiBulkTrackImport::write( BufferStringSet& errors )
{
    // TODO: Check if name exists, ask user to overwrite or give new name
    PtrMan<CtxtIOObj> ctxt = mMkCtxtIOObj( Well );
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	Data* wd = wells_[idx];
	PtrMan<IOObj> ioobj = IOM().getLocal( wd->name() );
	if ( !ioobj ) ioobj = mkEntry( *ctxt, wd->name() );
	if ( !ioobj )
	{
	    errors.add(
		BufferString("Cannot create Database entry for: ",wd->name()) );
	    continue;
	}

	PtrMan<Translator> t = ioobj->createTranslator();
	mDynamicCastGet(WellTranslator*,wtr,t.ptr())
	if ( wtr && wtr->write(*wd,*ioobj) )
	    continue;

	BufferString msg( wd->name() );
	msg.add( "to file:\n" ).add( ioobj->fullUserExpr(false) );
	errors.add( msg );
    }
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiBulkTrackImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter input file name" )

    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    if ( !dataselfld_->commit() )
	return false;

    readFile( *sd.istrm );
    BufferString errmsg;
    addD2T( errmsg );
    if ( !errmsg.isEmpty() )
	mErrRet( errmsg );

    BufferStringSet errors;
    write( errors );

    if ( errors.isEmpty() )
    {
	uiMSG().message( "All tracks imported succesfully" );
	return true;
    }

    uiMSG().errorWithDetails( errors,
		"Could not import all wells (see details)" );
    return false;
}


static IOObj* getWellIOObj( const char* nm, const char* uwi )
{
    IOM().to( MultiID("100050") );
    IODir iodir( MultiID("100050") );
    if ( nm && *nm )
    {
	const IOObj* ioobj = iodir[nm];
	if ( ioobj ) return ioobj->clone();
    }

    if ( uwi && *uwi )
    {
	PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( Well );
	IODirEntryList del( IOM().dirPtr(), ctio->ctxt );
	Data data;
	for ( int idx=0; idx<del.size(); idx++ )
	{
	    const IOObj* ioobj = del[idx]->ioobj;
	    if ( !ioobj ) continue;

	    Reader rdr( ioobj->fullUserExpr(), data );
	    if ( !rdr.getInfo() ) continue;

	    if ( data.info().uwid == uwi )
		return ioobj->clone();
	}
    }

    return 0;
}


// uiBulkLogImport
HiddenParam<uiBulkLogImport,uiGenInput*> istvdflds( 0 );
HiddenParam<uiBulkLogImport,uiGenInput*> udfflds( 0 );

uiBulkLogImport::uiBulkLogImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Bulk Log Import",mNoDlgTitle,"107.0.11"))
{
    inpfld_ = new uiFileInput( this, "Input LAS files",	uiFileInput::Setup() );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );

    uiGenInput* istvdfld = new uiGenInput( this, "Depth values are",
					   BoolInpSpec(false,"TVDSS","MD") );
    istvdfld->attach( alignedBelow, inpfld_ );
    istvdflds.setParam( this, istvdfld );

    const float defundefval = -999.25;
    uiGenInput* udffld = new uiGenInput( this, "Undefined value in logs",
			     FloatInpSpec(defundefval));
    udffld->attach( alignedBelow, istvdfld );
    udfflds.setParam( this, udffld );
}


uiBulkLogImport::~uiBulkLogImport()
{}


bool uiBulkLogImport::acceptOK( CallBacker* )
{
    BufferStringSet filenms;
    inpfld_->getFileNames( filenms );
    if ( filenms.isEmpty() )
    {
	uiMSG().error( "Please select at least one file" );
	return false;
    }

    const bool zistvd = istvdflds.getParam(this)->getBoolValue();
    BufferStringSet errors;
    for ( int idx=0; idx<filenms.size(); idx++ )
    {
	const BufferString& fnm = filenms.get( idx );
	LASImporter lasimp;
	LASImporter::FileInfo info;
	info.undefval = udfflds.getParam(this)->getfValue();
	BufferString errmsg = lasimp.getLogInfo( fnm, info );
	if ( !errmsg.isEmpty() )
	{
	    errors.add( BufferString(fnm,": ",errmsg) );
	    continue;
	}

	const PtrMan<IOObj> ioobj = getWellIOObj( info.wellnm, info.uwi );
	if ( !ioobj )
	{
	    errors.add( BufferString(fnm,": Cannot find ",info.wellnm) );
	    continue;
	}

	const bool isloaded = MGR().isLoaded( ioobj->key() );
	Data* wd = MGR().get( ioobj->key() );
	if ( !wd )
	{
	    errors.add( BufferString(info.wellnm,
			": Cannot find well information in database") );
	    continue;
	}

	lasimp.setData( wd );
	errmsg = lasimp.getLogs( fnm, info, zistvd );
	if ( !errmsg.isEmpty() )
	    errors.add( BufferString(fnm,": ",errmsg) );

	BufferString wellfnm = ioobj->fullUserExpr( true );
	Writer wtr( wellfnm, *wd );
	wtr.putLogs();
	if ( !isloaded )
	    delete MGR().release( ioobj->key() );
    }

    if ( errors.isEmpty() )
    {
	uiMSG().message( "All logs imported succesfully" );
	return true;
    }

    uiMSG().errorWithDetails( errors,
		"Could not import all LAS files (See details)" );
    return false;
}


HiddenParam<uiBulkMarkerImport,uiTableImpDataSel*> dataselflds( 0 );
HiddenParam<uiBulkMarkerImport,Table::FormatDesc*> fds( 0 );

// uiBulkMarkerImport
uiBulkMarkerImport::uiBulkMarkerImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Bulk Marker Import",mNoDlgTitle,"107.0.12"))
{
    inpfld_ = new uiFileInput( this, "Input Marker file", uiFileInput::Setup()
		.withexamine(true).examstyle(uiFileInput::Setup::Table) );

    Table::FormatDesc* fd = BulkMarkerAscIO::getDesc();
    fds.setParam( this, fd );

    uiTableImpDataSel* dataselfld =
	new uiTableImpDataSel( this, *fd, "107.0.9" );
    dataselfld->attach( alignedBelow, inpfld_ );
    dataselflds.setParam( this, dataselfld  );
}


uiBulkMarkerImport::~uiBulkMarkerImport()
{}


bool uiBulkMarkerImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter input file name" )

    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    uiTableImpDataSel* dataselfld = dataselflds.getParam( this );
    if ( !dataselfld || !dataselfld->commit() )
	return false;

    BufferStringSet wellnms;
    ObjectSet<MarkerSet> markersets;
    readFile( *sd.istrm, wellnms, markersets );
    if ( wellnms.isEmpty() )
	mErrRet( "No information read from file" );

    Table::FormatDesc* fd = fds.getParam( this );
    const ObjectSet<Table::TargetInfo>& tis = fd->bodyinfos_;
    const bool doconv = tis.validIdx(1) && tis[1]->selection_.form_==1;

    BufferStringSet errors;
    for ( int idx=0; idx<wellnms.size(); idx++ )
    {
	const BufferString& wellnm = wellnms.get(idx);
	if ( wellnm.isEmpty() ) continue;

	const PtrMan<IOObj> ioobj = getWellIOObj( wellnm, wellnm );
	if ( !ioobj )
	{
	    errors.add( BufferString("Cannot find ",wellnm," in database") );
	    continue;
	}

	const bool isloaded = MGR().isLoaded( ioobj->key() );
	Data* wd = MGR().get( ioobj->key() );
	if ( !wd )
	{
	    errors.add( BufferString(wellnm,": Cannot load well") );
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
	const BufferString wellfnm = ioobj->fullUserExpr();
	Writer wtr( wellfnm, *wd );
	if ( !wtr.putMarkers() )
	{
	    errors.add( BufferString(wellnm,
			": Cannot write new markers to disk") );
	    continue;
	}

	wd->markerschanged.trigger();
	if ( !isloaded )
	    delete MGR().release( ioobj->key() );
    }

    if ( errors.isEmpty() )
    {
	uiMSG().message( "All markers imported succesfully" );
	return true;
    }

    uiMSG().errorWithDetails( errors,
		"Could not import all marker files (See details)" );
    return false;
}


void uiBulkMarkerImport::readFile( std::istream& istrm,
				   BufferStringSet& wellnms,
				   ObjectSet<MarkerSet>& markersets )
{
    Table::FormatDesc* fd = fds.getParam( this );
    if ( !fd ) return;

    BulkMarkerAscIO aio( *fd, istrm );
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
    TypeSet<float>	mds_;
    TypeSet<float>	twts_;
};

// uiBulkD2TModelImport
uiBulkD2TModelImport::uiBulkD2TModelImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Bulk D2TModel Import",
				 mNoDlgTitle,mTODOHelpID))
    , fd_(BulkD2TModelAscIO::getDesc())
{
    uiFileInput::Setup fs;
    fs.withexamine(true).examstyle(uiFileInput::Setup::Table);
    inpfld_ = new uiFileInput( this, "Input Depth/Time Model file", fs );

    dataselfld_ = new uiTableImpDataSel( this, *fd_, "107.0.9" );
    dataselfld_->attach( alignedBelow, inpfld_ );
}


uiBulkD2TModelImport::~uiBulkD2TModelImport()
{}


bool uiBulkD2TModelImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter input file name" )

    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    if ( !dataselfld_->commit() )
	return false;

    ObjectSet<D2TModelData> d2tdata;
    readFile( *sd.istrm, d2tdata );
    if ( d2tdata.isEmpty() )
	mErrRet( "No information read from file" );

    BufferStringSet errors;
    IOM().to( MultiID("100050") );
    IODir iodir( MultiID("100050") );
    for ( int idx=0; idx<d2tdata.size(); idx++ )
    {
	const BufferString& wellnm = d2tdata[idx]->wellnm_;
	if ( wellnm.isEmpty() ) continue;

	const IOObj* ioobj = iodir[wellnm];
	if ( !ioobj )
	{
	    errors.add( BufferString("Cannot find ",wellnm," in database") );
	    continue;
	}

	const bool isloaded = MGR().isLoaded( ioobj->key() );
	Data* wd = MGR().get( ioobj->key() );
	if ( !wd )
	{
	    errors.add( BufferString(wellnm,": Cannot load well") );
	    continue;
	}

	// D2TModel* d2t = new D2TModel();
	// fill d2t
	const BufferString wellfnm = ioobj->fullUserExpr();
	Writer wtr( wellfnm, *wd );
	if ( !wtr.putD2T() )
	{
	    errors.add( BufferString(wellnm,
			": Cannot write new model to disk") );
	    continue;
	}

	wd->d2tchanged.trigger();
	if ( !isloaded )
	    delete MGR().release( ioobj->key() );
    }

    if ( errors.isEmpty() )
    {
	uiMSG().message( "All models imported succesfully" );
	return true;
    }

    uiMSG().errorWithDetails( errors,
		"Could not import all model files (See details)" );
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

void uiBulkD2TModelImport::readFile( std::istream& istrm,
				     ObjectSet<D2TModelData>& data )
{
    BulkD2TModelAscIO aio( *fd_, istrm );
    BufferString wellnm;
    float md = mUdf(float);
    float twt = mUdf(float);
    while ( aio.get(wellnm,md,twt) )
    {
	int wellidx = getIndex( data, wellnm );
	if ( wellidx<0 )
	{
	    D2TModelData* d2t = new D2TModelData( wellnm );
	    data += d2t;
	    wellidx = data.size()-1;
	}

	D2TModelData* d2t = data[wellidx];
	d2t->mds_ += md;
	d2t->twts_ += twt;
    }
}

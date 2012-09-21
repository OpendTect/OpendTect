/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "uibulkwellimp.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "wellman.h"
#include "welltrack.h"
#include "welltransl.h"
#include "wellwriter.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"


class BulkTrackAscIO : public Table::AscIO
{
public:
BulkTrackAscIO( const Table::FormatDesc& fd, std::istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


static Table::FormatDesc* getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkWellTrack" );
    fd->bodyinfos_ += new Table::TargetInfo( "Well name", Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += Table::TargetInfo::mkDepthPosition( true );
    fd->bodyinfos_ +=
	new Table::TargetInfo( "MD", FloatInpSpec(), Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( "Well ID (UWI)", Table::Optional );
    return fd;
}


bool getData( BufferString& wellnm, Coord3& crd, float md, BufferString& uwi )
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    wellnm = text( 0 );
    crd.x = getdValue( 1 );
    crd.y = getdValue( 2 );
    crd.z = getfValue( 3 );
    md = getfValue( 4 );
    uwi = text( 5 );
    return true;
}

    std::istream&	strm_;

};


uiBulkTrackImport::uiBulkTrackImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Well Track Import",mNoDlgTitle,mTODOHelpID))
    , fd_(BulkTrackAscIO::getDesc())
{
    inpfld_ = new uiFileInput( this, "Input file", uiFileInput::Setup()
		      .withexamine(true).examstyle(uiFileInput::Setup::Table) );

    dataselfld_ = new uiTableImpDataSel( this, *fd_, "107.0.9" );
    dataselfld_->attach( alignedBelow, inpfld_ );
}


uiBulkTrackImport::~uiBulkTrackImport()
{
    delete fd_;
    deepErase( wells_ );
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
    newctio.ioobj = 0; newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj;
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiBulkTrackImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter the input file name" )
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    if ( !dataselfld_->commit() )
	return false;

    BulkTrackAscIO aio( *fd_, *sd.istrm );
    BufferString wellnm, uwi; Coord3 crd;
    float md = mUdf(float);
    while ( aio.getData(wellnm,crd,md,uwi) )
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
	    wd->info().uwid = uwi;
	    wells_ += wd;
	}

	if ( crd.isDefined() )
	    wd->track().addPoint( crd.coord(), (float) crd.z, md );
    }

    // TODO: Check if name exists, ask user to overwrite or give new name
    BufferStringSet errors;
    PtrMan<CtxtIOObj> ctxt = mMkCtxtIOObj( Well );
    for ( int idx=0; idx<wells_.size(); idx++ )
    {
	Well::Data* wd = wells_[idx];
	PtrMan<IOObj> ioobj = IOM().getLocal( wd->name() );
	if ( !ioobj ) ioobj = mkEntry( *ctxt, wd->name() );
	if ( !ioobj )
	{
	    errors.add(
		BufferString("Cannot create Database entry for: ",wd->name()) );
	    continue;
	}

	PtrMan<Translator> t = ioobj->getTranslator();
	mDynamicCastGet(WellTranslator*,wtr,t.ptr())
	if ( wtr && wtr->write(*wd,*ioobj) )
	    continue;

	BufferString msg( wd->name() );
	msg.add( "to file:\n" ).add( ioobj->fullUserExpr(false) );
	errors.add( msg );
    }

    if ( errors.isEmpty() )
    {
	uiMSG().message( "All tracks imported succesfully" );
	return true;
    }

    uiMSG().errorWithDetails( errors,
		"Could not import all wells (see details)" );
    return false;
}


uiBulkLogImport::uiBulkLogImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Bulk Log Import",mNoDlgTitle,mTODOHelpID))
{
    inpfld_ = new uiFileInput( this, "Input LAS files",	uiFileInput::Setup() );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
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

    BufferStringSet errors;
    IOM().to( MultiID("100050") );
    IODir iodir( MultiID("100050") );
    for ( int idx=0; idx<filenms.size(); idx++ )
    {
	const BufferString& fnm = filenms.get( idx );
	Well::LASImporter lasimp;
	Well::LASImporter::FileInfo info;
	BufferString errmsg = lasimp.getLogInfo( fnm, info );
	if ( !errmsg.isEmpty() )
	{
	    errors.add( BufferString(fnm,": ",errmsg) );
	    continue;
	}

	const IOObj* ioobj = iodir[info.wellnm];
	if ( !ioobj )
	{
	    errors.add( BufferString(fnm,": Cannot find ",info.wellnm) );
	    continue;
	}

	const bool isloaded = Well::MGR().isLoaded( ioobj->key() );
	Well::Data* wd = Well::MGR().get( ioobj->key() );
	if ( !wd )
	{
	    errors.add( BufferString(info.wellnm,
			": Cannot find well information in database") );
	    continue;
	}

	lasimp.setData( wd );
	errmsg = lasimp.getLogs( fnm, info );
	if ( !errmsg.isEmpty() )
	    errors.add( BufferString(fnm,": ",errmsg) );

	BufferString wellfnm = ioobj->fullUserExpr( true );
	Well::Writer wtr( wellfnm, *wd );
	wtr.putLogs();
	if ( !isloaded )
	    delete Well::MGR().release( ioobj->key() );
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



uiBulkMarkerImport::uiBulkMarkerImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Bulk Marker Import",mNoDlgTitle,mTODOHelpID))
{
    inpfld_ = new uiFileInput( this, "Input Marker file", uiFileInput::Setup());
}


uiBulkMarkerImport::~uiBulkMarkerImport()
{}


bool uiBulkMarkerImport::acceptOK( CallBacker* )
{
    return true;
}

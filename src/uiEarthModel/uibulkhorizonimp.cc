/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2012
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "uibulkhorizonimp.h"

#include "binidvalset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "executor.h"
#include "posinfodetector.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"


class BulkHorizonAscIO : public Table::AscIO
{
public:
BulkHorizonAscIO( const Table::FormatDesc& fd, std::istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
{}


static Table::FormatDesc* getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkHorizon" );
    fd->bodyinfos_ += new Table::TargetInfo( "Horizon name", Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    return fd;
}


bool isXY() const
{ return formOf( false, 1 ) == 0; }


bool getData( BufferString& hornm, Coord3& crd )
{
    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    hornm = text( 0 );
    crd.x = getdValue( 1 );
    crd.y = getdValue( 2 );
    crd.z = getfValue( 3 );
    return true;
}

    std::istream&	strm_;
};


uiBulkHorizonImport::uiBulkHorizonImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Horizon Import",mNoDlgTitle,mTODOHelpID))
    , fd_(BulkHorizonAscIO::getDesc())
{
    inpfld_ = new uiFileInput( this, "Input file", uiFileInput::Setup()
		      .withexamine(true).examstyle(uiFileInput::Setup::Table) );

    dataselfld_ = new uiTableImpDataSel( this, *fd_, "107.0.9" );
    dataselfld_->attach( alignedBelow, inpfld_ );
}


uiBulkHorizonImport::~uiBulkHorizonImport()
{
    delete fd_;
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiBulkHorizonImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( "Please enter the input file name" )
    StreamData sd( StreamProvider(fnm).makeIStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    if ( !dataselfld_->commit() )
	return false;

    ObjectSet<BinIDValueSet> data;
    BufferStringSet hornms;
    BulkHorizonAscIO aio( *fd_, *sd.istrm );
    BufferString hornm; Coord3 crd;
    while ( aio.getData(hornm,crd) )
    {
	if ( hornm.isEmpty() )
	    continue;

	BinIDValueSet* bidvs = 0;
	const int hidx = hornms.indexOf( hornm );
	if ( data.validIdx(hidx) )
	    bidvs = data[hidx];
	else
	{
	    bidvs = new BinIDValueSet( 1, false );
	    data += bidvs;
	    hornms.add( hornm );
	}

	if ( !crd.isDefined() )
	    continue;

	if ( aio.isXY() )
	    bidvs->add( SI().transform(crd.coord()), crd.z );
	else
	{
	    BinID bid( mNINT32(crd.x), mNINT32(crd.y) );
	    bidvs->add( bid, crd.z );
	}
    }

    // TODO: Check if name exists, ask user to overwrite or give new name
    BufferStringSet errors;
    uiTaskRunner dlg( this );
    for ( int idx=0; idx<hornms.size(); idx++ )
    {
	RefMan<EM::Horizon3D> hor3d = EM::Horizon3D::create( hornms.get(idx) );
	if ( !hor3d )
	{
	    pErrMsg( "Huh?" );
	    continue;
	}

	BinIDValueSet* bidvs = data[idx];
	BinID bid;
	BinIDValueSet::Pos pos;
	PosInfo::Detector detector( PosInfo::Detector::Setup(false) );
	while ( bidvs->next(pos) )
	{
	    bid = bidvs->getBinID( pos );
	    detector.add( SI().transform(bid), bid );
	}
	detector.finish();

	HorSampling hs;
	detector.getHorSampling( hs );
	ObjectSet<BinIDValueSet> curdata; curdata += bidvs;
	PtrMan<Executor> importer = hor3d->importer( curdata, hs );
	if ( !importer || !TaskRunner::execute( &dlg, *importer ) )
	    continue;

	PtrMan<Executor> saver = hor3d->saver();
	if ( !saver || !TaskRunner::execute( &dlg, *saver ) )
	    continue;
    }

    return true;
}


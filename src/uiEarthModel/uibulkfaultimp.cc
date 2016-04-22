/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2012
________________________________________________________________________

-*/


#include "uibulkfaultimp.h"

#include "emfault3d.h"
#include "emmanager.h"
#include "executor.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"


class BulkFaultAscIO : public Table::AscIO
{
public:
BulkFaultAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
    , finishedreadingheader_(false)
{}


static Table::FormatDesc* getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkFault" );

    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Fault name", Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    fd->bodyinfos_ += new Table::TargetInfo( "Stick number", IntInpSpec(),
					     Table::Required );
    return fd;
}


bool isXY() const
{ return formOf( false, 1 ) == 0; }


bool getData( BufferString& fltnm, Coord3& crd, int& sticknr )
{
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return false;

	udfval_ = getFValue( 0 );
	finishedreadingheader_ = true;
    }


    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    fltnm = text( 0 );
    if ( isXY() )
    {
	crd.x = getDValue( 1, udfval_ );
	crd.y = getDValue( 2, udfval_ );
	crd.z = getFValue( 3, udfval_ );
    }
    else
    {
	BinID bid( (int)getDValue(1,udfval_), (int)getDValue(2,udfval_) );
	crd = Coord3( SI().transform(bid), getFValue(3,udfval_) );
    }

    sticknr = getIntValue( 4 );
    return true;
}

    od_istream&		strm_;
    float		udfval_;
    bool		finishedreadingheader_;
};


uiBulkFaultImport::uiBulkFaultImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrImport(tr("Multiple Faults")),
				 mNoDlgTitle,
				 mODHelpKey(mBulkFaultImportHelpID))
				.modal(false))
    , fd_(BulkFaultAscIO::getDesc())
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiFileInput( this,
		      uiStrings::sInputASCIIFile(),
		      uiFileInput::Setup().withexamine(true)
		      .examstyle(File::Table) );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
                                    mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, inpfld_ );
}


uiBulkFaultImport::~uiBulkFaultImport()
{
    delete fd_;
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

bool uiBulkFaultImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("the input file name")) )
    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet(uiStrings::phrCannotOpen(uiStrings::sInputFile().toLower()))

    if ( !dataselfld_->commit() )
	return false;

    BufferStringSet fltnms;
    TypeSet< TypeSet<Coord3> > nodes;
    TypeSet< TypeSet<int> > sticks;

    BulkFaultAscIO aio( *fd_, strm );
    BufferString fltnm; Coord3 crd; int sticknr;
    while ( aio.getData(fltnm,crd,sticknr) )
    {
	if ( fltnm.isEmpty() || !crd.isDefined() || mIsUdf(sticknr)  )
	    continue;

	const int fltidx = fltnms.indexOf( fltnm );
	if ( fltidx==-1 )
	{
	    fltnms.add( fltnm );
	    TypeSet<Coord3> poses; poses += crd; nodes += poses;
	    TypeSet<int> stickids; stickids += sticknr; sticks += stickids;
	}
	else
	{
	    nodes[fltidx] += crd;
	    sticks[fltidx] += sticknr;
	}
    }

    // TODO: Check if name exists, ask user to overwrite or give new name
    BufferStringSet errors;
    uiTaskRunner taskr( this );
    const Coord3 normal( 0, 0, 1 );
    for ( int idx=0; idx<fltnms.size(); idx++ )
    {
	EM::ObjectID eid = EM::EMM().createObject(
		EM::Fault3D::typeStr(), fltnms.get(idx) );
	EM::EMObject* emobj = EM::EMM().getObject( eid );
	emobj->ref();
	mDynamicCastGet( EM::Fault3D*, emflt, emobj );
	if ( !emflt )
	{
	    emobj->unRef();
	    continue;
	}

	Geometry::FaultStickSet& flt = *emflt->geometry().sectionGeometry(0);
	TypeSet<int> usedstickids;

	for ( int sidx=0; sidx<sticks[idx].size(); sidx++ )
	{
	    const int cursidx = usedstickids.indexOf( sticks[idx][sidx] );
	    if ( cursidx < 0 )
	    {
		const int curstickidx = usedstickids.size();
		usedstickids += sticks[idx][sidx];
		flt.insertStick( nodes[idx][sidx], normal, curstickidx );
	    }
	    else
	    {
		const TypeSet<Coord3>* poses = flt.getStick( cursidx );
		const int lastidx = poses ? poses->size() : 0;
		flt.insertKnot( RowCol(cursidx,lastidx), nodes[idx][sidx] );
	    }
	}

	PtrMan<Executor> saver = emflt->saver();
	if ( saver )
	    TaskRunner::execute( &taskr, *saver );
	emobj->unRef();
    }

    uiMSG().message( tr("Imported all faults from file %1").arg(fnm) );
    return true;
}

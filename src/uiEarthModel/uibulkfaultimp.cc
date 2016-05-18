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
    fd->bodyinfos_ += new Table::TargetInfo( "Stick index", IntInpSpec(),
					     Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( "Node index", IntInpSpec(),
					     Table::Optional );
    return fd;
}


bool isXY() const
{ return formOf( false, 1 ) == 0; }


bool getData( BufferString& fltnm, Coord3& crd, int& stickidx, int& nodeidx )
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

    stickidx = getIntValue( 4 );
    nodeidx = getIntValue( 5 );
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

struct FaultPars
{
FaultPars( const char* nm )
    : name_(nm)	    {}

void add( const Coord3& crd, int stickidx, int nodeidx )
{
    nodes_ += crd;
    stickidxs_ += stickidx;
    nodeidxs_ += nodeidx;
}

    BufferString	name_;
    TypeSet<Coord3>	nodes_;
    TypeSet<int>	stickidxs_;
    TypeSet<int>	nodeidxs_;

};

static int getFaultIndex( const char* nm, const ObjectSet<FaultPars>& pars )
{
    for ( int idx=0; idx<pars.size(); idx++ )
    {
	if ( pars[idx]->name_ == nm )
	    return idx;
    }

    return -1;
}


static void readInput( BulkFaultAscIO& aio, ObjectSet<FaultPars>& pars )
{
    BufferString fltnm; Coord3 crd; int stickidx; int nodeidx;
    while ( aio.getData(fltnm,crd,stickidx,nodeidx) )
    {
	if ( fltnm.isEmpty() || !crd.isDefined() )
	    continue;

	int fltidx = getFaultIndex( fltnm, pars );
	if ( fltidx == -1 )
	{
	    pars += new FaultPars( fltnm );
	    fltidx = pars.size() - 1;
	}

	pars[fltidx]->add( crd, stickidx, nodeidx );
    }
}


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

    ManagedObjectSet<FaultPars> pars;
    BulkFaultAscIO aio( *fd_, strm );
    readInput( aio, pars );

    // TODO: Check if name exists, ask user to overwrite or give new name
    uiTaskRunner taskr( this );
    const Coord3 normal( 0, 0, 1 );
    for ( int idx=0; idx<pars.size(); idx++ )
    {
	EM::ObjectID eid = EM::EMM().createObject(
		EM::Fault3D::typeStr(), pars[idx]->name_.buf() );
	EM::EMObject* emobj = EM::EMM().getObject( eid );
	emobj->ref();
	mDynamicCastGet( EM::Fault3D*, emflt, emobj );
	if ( !emflt )
	{
	    emobj->unRef();
	    continue;
	}

	const TypeSet<Coord3>& nodes = pars[idx]->nodes_;
	const TypeSet<int>& stickidxs = pars[idx]->stickidxs_;
	const TypeSet<int>& nodeidxs = pars[idx]->nodeidxs_;
	const bool hasstickidx =
		!stickidxs.isEmpty() && !mIsUdf( stickidxs.first() );
	const bool hasnodeidx =
		!nodeidxs.isEmpty() && !mIsUdf( nodeidxs.first() );

	Geometry::FaultStickSet& flt = *emflt->geometry().sectionGeometry(0);

	int prevstickidx = -mUdf(int);
	int prevnodeidx = mUdf(int);
	double prevz = mUdf(double);
	const int nrnodes = nodes.size();
	int mystickidx = -1;
	int mynodeidx = -1;
	for ( int nidx=0; nidx<nrnodes; nidx++ )
	{
	    const Coord3& crd = nodes[nidx];
	    const int stickidx = stickidxs[nidx];
	    const int nodeidx = nodeidxs[nidx];
	    bool addnewstick = false;
	    if ( hasstickidx )
		addnewstick = stickidx>prevstickidx;
	    else if ( hasnodeidx )
		addnewstick = nodeidx<prevnodeidx;
	    else
		addnewstick = crd.z < prevz;

	    if ( addnewstick )
	    {
		mynodeidx = 0;
		mystickidx++;
		flt.insertStick( crd, normal, mystickidx );
	    }
	    else
	    {
		mynodeidx++;
		const RowCol rc( mystickidx, mynodeidx );
		flt.insertKnot( rc, crd );
	    }

	    prevstickidx = stickidx;
	    prevnodeidx = nodeidx;
	}

	PtrMan<Executor> saver = emflt->saver();
	if ( saver )
	    TaskRunner::execute( &taskr, *saver );
	emobj->unRef();
    }

    uiMSG().message( tr("Imported all faults from file %1").arg(fnm) );
    return false;
}

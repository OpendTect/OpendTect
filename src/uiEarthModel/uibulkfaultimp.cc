/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2012
________________________________________________________________________

-*/


#include "uibulkfaultimp.h"

#include "emfault3d.h"
#include "emfaultstickset.h"
#include "emfsstofault3d.h"
#include "emmanager.h"
#include "executor.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"

#include "uidialog.h"

#include "emsurfacetr.h"
#include "survgeom.h"


class BulkFaultAscIO : public Table::AscIO
{
public:
BulkFaultAscIO( const Table::FormatDesc& fd, od_istream& strm, bool is2d )
    : Table::AscIO(fd)
    , strm_(strm)
    , finishedreadingheader_(false)
    , is2d_(is2d)
{}


static Table::FormatDesc* getDesc( bool isfss, bool is2d )
{
    BufferString descnm = isfss ? "BulkFaultStickSet" : "BulkFault";
    Table::FormatDesc* fd = new Table::FormatDesc( "descnm" );
    BufferString dispnm = isfss ? "FaultStickSet name" : "Fault name";
    fd->bodyinfos_ += new Table::TargetInfo( uiStrings::sFaultName(),
							    Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    fd->bodyinfos_ += new Table::TargetInfo( uiStrings::sStickIndex(),
					    IntInpSpec(), Table::Optional );
    fd->bodyinfos_ += new Table::TargetInfo( uiStrings::sNodeIndex(),
					    IntInpSpec(), Table::Optional );
    if ( is2d )
	fd->bodyinfos_ += new Table::TargetInfo( uiStrings::sLineName(),
					StringInpSpec(), Table::Required );
    return fd;
}


bool isXY() const
{ return formOf( false, 1 ) == 0; }


bool getData( BufferString& fltnm, Coord3& crd, int& stickidx, int& nodeidx,
		BufferString& lnnm )
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
	crd.x_ = getDValue( 1, udfval_ );
	crd.y_ = getDValue( 2, udfval_ );
	crd.z_ = getFValue( 3, udfval_ );
    }
    else
    {
	BinID bid( (int)getDValue(1,udfval_), (int)getDValue(2,udfval_) );
	crd = Coord3( SI().transform(bid), getFValue(3,udfval_) );
    }

    stickidx = getIntValue( 4 );
    nodeidx = getIntValue( 5 );
    if ( is2d_ )
	lnnm = text( 6 );
    return true;
}

    od_istream&		strm_;
    float		udfval_;
    bool		finishedreadingheader_;
    bool		is2d_;
};


static const char* sKeyGeometric()	{ return "Geometric"; }
static const char* sKeyIndexed()	{ return "Indexed"; }
static const char* sKeyFileOrder()	{ return "File order"; }

#define mGet( tp, fss, f3d ) \
    FixedString(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? fss : f3d

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D) )

#define mGetHelpKey(tp) \
    mGet( tp, (is2d ? mODHelpKey(mImportFaultStick2DHelpID) \
		 : mODHelpKey(mImportFaultStick3DHelpID) ), \
    mODHelpKey(mImportFaultHelpID) )

uiBulkFaultImport::uiBulkFaultImport( uiParent* p, const char* type, bool is2d )
    : uiDialog(p,uiDialog::Setup(mGet( type, (is2d
			    ? tr("Import Multiple FaultStickSets 2D")
			 : tr("Import Multiple FaultStickSets")),
			      tr("Import Multiple Faults") ),mNoDlgTitle,
			      mGetHelpKey(type)).modal(false))
    , isfss_(mGet(type,true,false))
    , is2d_(is2d)
    , fd_(BulkFaultAscIO::getDesc(mGet(type,true,false),is2d))
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiFileSel( this,
		      uiStrings::sInputASCIIFile(),
		      uiFileSel::Setup().withexamine(true)
		      .examstyle(File::Table) );

    BufferStringSet sticksortopt;
    sticksortopt.add( sKeyGeometric() )
		.add( sKeyIndexed() ).add( sKeyFileOrder() );
    sortsticksfld_ = new uiGenInput( this, tr("Stick sorting"),
				     StringListInpSpec(sticksortopt) );
    sortsticksfld_->attach( alignedBelow, inpfld_ );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, sortsticksfld_ );
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

void add( const Coord3& crd, int stickidx, int nodeidx, BufferString lnnm )
{
    nodes_ += crd;
    stickidxs_ += stickidx;
    nodeidxs_ += nodeidx;
    lnnms_.add( lnnm );
}

    BufferString	name_;
    TypeSet<Coord3>	nodes_;
    TypeSet<int>	stickidxs_;
    TypeSet<int>	nodeidxs_;
    BufferStringSet	lnnms_;

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
    BufferString lnnm;
    while ( aio.getData(fltnm, crd, stickidx, nodeidx, lnnm) )
    {
	if ( fltnm.isEmpty() || !crd.isDefined() )
	    continue;

	int fltidx = getFaultIndex( fltnm, pars );
	if ( fltidx == -1 )
	{
	    pars += new FaultPars( fltnm );
	    fltidx = pars.size() - 1;
	}

	pars[fltidx]->add( crd, stickidx, nodeidx, lnnm );
    }
}


static void fillFaultSticks( FaultPars& pars, ObjectSet<EM::FaultStick>& sticks)
{
    const TypeSet<Coord3>& nodes = pars.nodes_;
    const TypeSet<int>& stickidxs = pars.stickidxs_;
    const TypeSet<int>& nodeidxs = pars.nodeidxs_;
    const bool hasstickidx =
	    !stickidxs.isEmpty() && !mIsUdf( stickidxs.first() );
    const bool hasnodeidx =
	    !nodeidxs.isEmpty() && !mIsUdf( nodeidxs.first() );

    int prevstickidx = -mUdf(int);
    int prevnodeidx = mUdf(int);
    double prevz = mUdf(double);
    const int nrnodes = nodes.size();
    int mystickidx = -1;
    for ( int nidx=0; nidx<nrnodes; nidx++ )
    {
	const Coord3& crd = nodes[nidx];
	const int stickidx = stickidxs[nidx];
	const int nodeidx = nodeidxs[nidx];
	bool addnewstick = false;
	if ( hasstickidx )
	    addnewstick = stickidx != prevstickidx;
	else if ( hasnodeidx )
	    addnewstick = nodeidx < prevnodeidx;
	else
	    addnewstick = crd.z_ < prevz;

	if ( addnewstick )
	{
	    mystickidx++;
	    sticks += new EM::FaultStick( hasstickidx ? stickidx : mystickidx );
	}
	sticks[mystickidx]->crds_ += crd;

	if ( !pars.lnnms_.isEmpty() )
	    sticks[ mystickidx ]->lnm_ = pars.lnnms_.get(nidx);

	prevstickidx = stickidx;
	prevnodeidx = nodeidx;
    }
}


static void updateFaultStickSet( EM::Fault* flt,
				ObjectSet<EM::FaultStick>& sticks, bool is2d )
{
    if ( !flt ) return;

    int sticknr = !sticks.isEmpty() ? sticks[0]->stickidx_ : 0;

    for ( int idx=0; idx<sticks.size(); idx++ )
    {
	EM::FaultStick* stick = sticks[idx];
	if ( stick->crds_.isEmpty() )
	    continue;
	if ( is2d )
	{
	    mDynamicCastGet(EM::FaultStickSet*,emfss,flt)
	    const Pos::GeomID geomid = Survey::GM().getGeomID( stick->lnm_ );
	    emfss->geometry().insertStick( sticknr, 0,
		    stick->crds_[0], stick->getNormal(true), geomid, false );

	}
	else
	    flt->geometry().insertStick( sticknr, 0,
			stick->crds_[0], stick->getNormal(false), false );

	for ( int crdidx=1; crdidx<stick->crds_.size(); crdidx++ )
	{
	    const RowCol rc( stick->stickidx_, crdidx );
	    flt->geometry().insertKnot( EM::PosID::getFromRowCol(rc),
				       stick->crds_[crdidx], false );
	}
	sticknr++;
    }
}


bool uiBulkFaultImport::acceptOK()
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
    BulkFaultAscIO aio( *fd_, strm, is2d_ );
    readInput( aio, pars );
    // TODO: Check if name exists, ask user to overwrite or give new name
    uiTaskRunner taskr( this );

    const char* typestr = isfss_ ? EM::FaultStickSet::typeStr()
			       : EM::Fault3D::typeStr();
    BufferString savernm = isfss_ ? "Saving FaultStickSets" : "Saving Faults";
    ExecutorGroup saver( savernm );
    for ( int idx=0; idx<pars.size(); idx++ )
    {
	EM::ObjectManager& emm = isfss_ ? EM::FSSMan() : EM::Flt3DMan();
	EM::Object* emobj = emm.createObject( typestr,
						pars[idx]->name_.buf() );
	emobj->ref();
	mDynamicCastGet( EM::Fault*, flt, emobj );
	if ( !flt )
	{
	    emobj->unRef();
	    continue;
	}

	ManagedObjectSet<EM::FaultStick> faultsticks;
	fillFaultSticks( *pars[idx], faultsticks );
	updateFaultStickSet( flt, faultsticks, is2d_ );

	saver.add( emobj->saver() );
	emobj->unRef();
    }

    if ( TaskRunner::execute( &taskr, saver ) )
    {
	uiMSG().message( tr("Imported all %1 from file %2").arg(isfss_ ?
	  uiStrings::sFaultStickSet(mPlural) : uiStrings::sFault(mPlural))
	  .arg(fnm) );
	return true;
    }
    else
    {
	saver.errorWithDetails();
	return false;
    }
}

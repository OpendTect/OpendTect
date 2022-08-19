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
#include "emsurfacetr.h"
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
#include "emfsstofault3d.h"
#include "emfaultset3d.h"
#include "uiioobjsel.h"

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
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkFault" );

    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    BufferString typnm = isfss ? "FaultStickSet name" : "Fault name";
    fd->bodyinfos_ += new Table::TargetInfo( typnm , Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    fd->bodyinfos_ += new Table::TargetInfo( "Stick number", IntInpSpec(),
					     Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Node index", IntInpSpec(),
					     Table::Optional );
    if ( is2d )
	fd->bodyinfos_ += new Table::TargetInfo( "Line name", StringInpSpec(),
						 Table::Required );
    return fd;
}


bool isXY() const
{
    return formOf( false, 1 ) == 0;
}


bool getData( BufferString& fltnm, Coord3& crd, int& stickidx, int& nodeidx,
		BufferString& lnnm )
{
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return false;

	udfval_ = getDValue( 0 );
	finishedreadingheader_ = true;
    }


    const int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return false;

    fltnm = getText( 0 );

    if ( !isXY() )
    {
	const Coord xycrd = SI().transform( getBinID(1,2,udfval_) );
	crd.setXY( xycrd.x, xycrd.y );
	crd.z = getDValue( 3 );
    }
    else
	crd = getPos3D( 1, 2, 3, udfval_ );

    stickidx = getIntValue( 4 );
    nodeidx = getIntValue( 5 );
    if ( is2d_ )
	lnnm = getText( 6 );
    return true;
}

    od_istream&		strm_;
    double		udfval_;
    bool		finishedreadingheader_;
    bool		is2d_;
};


static const char* sKeyGeometric()	{ return "Geometric"; }
static const char* sKeyIndexed()	{ return "Indexed"; }
static const char* sKeyFileOrder()	{ return "File order"; }


#define mGet( tp, fss, f3d, fset ) \
    FixedString(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? fss : \
    (FixedString(tp) == EMFaultSet3DTranslatorGroup::sGroupName() ? fset : f3d)

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D), \
					*mMkCtxtIOObj(EMFaultSet3D) )

#define mGetHelpKey(tp) \
    mGet( tp, (is2d ? mODHelpKey(mImportFaultStick2DHelpID) \
		    : mODHelpKey(mImportFaultStick3DHelpID) ), \
    mODHelpKey(mImportFaultHelpID), mTODOHelpKey )

uiBulkFaultImport::uiBulkFaultImport(uiParent* p)
	: uiDialog(p, uiDialog::Setup(tr("Import Multiple Faults"), mNoDlgTitle,
		mODHelpKey(mBulkFaultImportHelpID))
		.modal(false))
	, fd_(BulkFaultAscIO::getDesc(false, false))
	, isfss_(false)
	, isfltset_(false)
	, is2dfss_(false)
{
    init();
}


uiBulkFaultImport::uiBulkFaultImport( uiParent* p, const char* type, bool is2d )
    : uiDialog(p,uiDialog::Setup(mGet( type, (is2d
			      ? tr("Import Multiple FaultStickSets 2D")
			      : tr("Import Multiple FaultStickSets")),
				tr("Import Multiple Faults"),
				tr("Import FaultSet")),mNoDlgTitle,
				mGetHelpKey(type)).modal(false))
    , fd_(BulkFaultAscIO::getDesc(mGet(type,true,false,false),is2d))
    , isfss_(mGet(type,true,false,false))
    , isfltset_(mGet(type,false,false,true))
	, is2dfss_(is2d)
{
    init();
}


void uiBulkFaultImport::init()
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::Table );

    BufferStringSet sticksortopt;
    sticksortopt.add( sKeyGeometric() )
		.add( sKeyIndexed() ).add( sKeyFileOrder() );
    sortsticksfld_ = new uiGenInput( this, tr("Stick sorting"),
				     StringListInpSpec(sticksortopt) );
	sortsticksfld_->attach( alignedBelow, inpfld_ );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
		mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, (uiObject*)sortsticksfld_ );

    if ( isfltset_ )
    {
	IOObjContext ctxt = mIOObjContext(EMFaultSet3D);
	ctxt.forread_ = false;
	fltsetnmfld_ = new uiIOObjSel( this, ctxt,
				uiStrings::phrOutput(uiStrings::sFaultSet()) );
	fltsetnmfld_->attach( alignedBelow, dataselfld_ );

	mAttachCB( inpfld_->valuechanged, uiBulkFaultImport::inpChangedCB );
    }
}


uiBulkFaultImport::~uiBulkFaultImport()
{
    detachAllNotifiers();
    delete fd_;
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }

struct FaultPars
{
FaultPars( const char* nm )
    : name_(nm)     {}

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
    BufferStringSet	lnnms_;
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
	    addnewstick = crd.z < prevz;

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
	const EM::SectionID sid = flt->sectionID( 0 );
	EM::FaultStick* stick = sticks[idx];
	if ( stick->crds_.isEmpty() )
	    continue;
	if ( is2d )
	{
	    mDynamicCastGet(EM::FaultStickSet*,emfss,flt)
	    const Pos::GeomID geomid = Survey::GM().getGeomID( stick->lnm_ );
	    emfss->geometry().insertStick( sid, sticknr, 0,
		    stick->crds_[0], stick->getNormal(true), geomid, false );

	}
	else
	    flt->geometry().insertStick( sid, sticknr, 0,
			stick->crds_[0], stick->getNormal(false), false );

	for ( int crdidx=1; crdidx<stick->crds_.size(); crdidx++ )
	{
	    const RowCol rc( stick->stickidx_, crdidx );
	    flt->geometry().insertKnot( sid, rc.toInt64(),
				       stick->crds_[crdidx], false );
	}
	sticknr++;
    }
}


void uiBulkFaultImport::inpChangedCB( CallBacker* )
{
    if ( fltsetnmfld_ )
		fltsetnmfld_->setInput( MultiID(inpfld_->baseName()) );
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
    BulkFaultAscIO aio( *fd_, strm, is2dfss_ );
    readInput( aio, pars );

    // TODO: Check if name exists, ask user to overwrite or give new name
    uiTaskRunner taskr( this );

    const char* typestr = isfss_ ? EM::FaultStickSet::typeStr()
						    : EM::Fault3D::typeStr();
    BufferString savernm = isfss_ ? "Saving FaultStickSets" :
		isfltset_ ? "Saving FaultSet" :"Saving Faults";

    EM::FaultSet3D* fltset = nullptr;

    if (isfltset_)
    {
	const IOObj* ioobj = fltsetnmfld_->ioobj();
	if ( !ioobj )
	    return false;
	EM::ObjectID oid = EM::EMM().createObject( EM::FaultSet3D::typeStr(),
								ioobj->name() );
	mDynamicCast( EM::FaultSet3D*, fltset, EM::EMM().getObject( oid ) );
	if (isfltset_ && !fltset )
	    return false;
    }

    ExecutorGroup saver( savernm );
    for ( int idx=0; idx<pars.size(); idx++ )
    {
	EM::EMManager& em = EM::EMM();
	RefMan<EM::EMObject> emobj = nullptr;
	EM::ObjectID emid;
	if (isfltset_)
	    emobj = em.createTempObject( typestr );
	else
	{
	    emid = em.createObject( typestr, pars[idx]->name_.buf() );
	    emobj = em.getObject( emid );
	}

	mDynamicCastGet( EM::Fault*, flt, emobj.ptr() );

	if ( !flt )
	    continue;

	ManagedObjectSet<EM::FaultStick> faultsticks;
	fillFaultSticks( *pars[idx], faultsticks );
	updateFaultStickSet( flt, faultsticks, is2dfss_ );

	if ( !isfltset_)
	    saver.add( emobj->saver() );
	else
	{
	    mDynamicCastGet( EM::Fault3D*, flt3d, emobj.ptr() );
	    fltset->addFault( flt3d );
	}
    }

    if (isfltset_)
	saver.add( fltset->saver() );

    if ( TaskRunner::execute( &taskr, saver ) )
    {
	uiString msg;
	if ( isfltset_ )
	{
	    msg = tr("%1 successfully imported.\n"
		     "Do you want to import more %2?")
		  .arg( uiStrings::sFaultSet() )
		  .arg( uiStrings::sFaultSet(mPlural) );
	}
	else
	{
	    msg = tr("%1 successfully imported.\n"
		     "Do you want to import more %1?")
		  .arg( isfss_ ? uiStrings::sFaultStickSet(mPlural)
			       : uiStrings::sFault(mPlural) );
	}

	const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				    tr("No, close window") );
	return !ret;
    }
    else
    {
	saver.uiMessage();
	return false;
    }
}

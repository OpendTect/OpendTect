/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibulkfaultimp.h"

#include "emfault3d.h"
#include "emfaultset3d.h"
#include "emfaultstickset.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioman.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"
#include "unitofmeasure.h"

#include "uifileinput.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"
#include "emfsstofault3d.h"
#include "emfaultset3d.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"

class BulkFaultAscIO : public Table::AscIO
{
public:
BulkFaultAscIO( const Table::FormatDesc& fd, od_istream& strm, bool is2d )
    : Table::AscIO(fd)
    , strm_(strm)
    , finishedreadingheader_(false)
    , is2d_(is2d)
{}


static Table::FormatDesc* getDesc( EM::EMObjectType type, bool is2d,
						const ZDomain::Def& def )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkFault" );

    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    createDesc( fd, type, is2d, def );
    return fd;
}


static void createDesc( Table::FormatDesc* fd, EM::EMObjectType type,
					bool is2d, const ZDomain::Def& def )
{
    BufferString typnm;
    if ( EM::isFaultStickSet(type) )
	typnm = "FaultStickSet name";
    else if ( type == EM::EMObjectType::Flt3D )
	typnm = "Fault name";
    else
	typnm = "FaultSet name";

    fd->bodyinfos_ += new Table::TargetInfo( typnm , Table::Required );
    fd->bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    if ( def.isTime() )
	fd->bodyinfos_ += Table::TargetInfo::mkTimePosition( true );
    else
	fd->bodyinfos_ += Table::TargetInfo::mkDepthPosition( true );

    fd->bodyinfos_ += new Table::TargetInfo( "Stick number", IntInpSpec(),
	Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Node index", IntInpSpec(),
	Table::Optional );
    if ( is2d )
	fd->bodyinfos_ += new Table::TargetInfo( "Line name", StringInpSpec(),
	    Table::Required );
}


static void updateDesc( Table::FormatDesc& fd, EM::EMObjectType type,
					bool is2d, const ZDomain::Def& def )
{
    fd.bodyinfos_.erase();
    createDesc( &fd, type, is2d, def );
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
    double		udfval_			= mUdf(double);
    bool		finishedreadingheader_;
    bool		is2d_;
};


static const char* sKeyGeometric()	{ return "Geometric"; }
static const char* sKeyIndexed()	{ return "Indexed"; }
static const char* sKeyFileOrder()	{ return "File order"; }
static const char* sKeyAutoStickSel()	{ return "Auto"; }
static const char* sKeyInlCrlSep()	{ return "Inl/Crl separation"; }
static const char* sKeySlopeThres()	{ return "Slope threshold"; }


#define mGet( tp, fss, f3d, fset ) \
    StringView(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? fss : \
    (StringView(tp) == EMFaultSet3DTranslatorGroup::sGroupName() ? fset : f3d)

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D), \
					*mMkCtxtIOObj(EMFaultSet3D) )

#define mGetHelpKey(tp) \
    mGet( tp, (is2d ? mODHelpKey(mImportFaultStick2DHelpID) \
		    : mODHelpKey(mImportFaultStick3DHelpID) ), \
    mODHelpKey(mImportFaultHelpID), mTODOHelpKey )

#define mGetType(tp) \
    StringView(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? \
	EM::EMObjectType::FltSS3D : \
    StringView(tp).isEqual("FaultStickSet 2D") ? EM::EMObjectType::FltSS2D : \
    StringView(tp).isEqual("FaultStickSet 2D and 3D") ? \
	EM::EMObjectType::FltSS2D3D : \
    StringView(tp) == EMFaultSet3DTranslatorGroup::sGroupName() ? \
	EM::EMObjectType::FltSet : EM::EMObjectType::Flt3D \


uiBulkFaultImport::uiBulkFaultImport(uiParent* p)
    : uiDialog(p, uiDialog::Setup(tr("Import Multiple Faults"), mNoDlgTitle,
	    mODHelpKey(mBulkFaultImportHelpID))
	    .modal(false))
    , fd_(BulkFaultAscIO::getDesc(EM::EMObjectType::Flt3D,false,SI().zDomain()))
    , isfss_(false)
    , isfltset_(false)
    , is2dfss_(false)
{
    init();
}


uiBulkFaultImport::uiBulkFaultImport( uiParent* p, const char* type,
								    bool is2d )
    : uiDialog(p,uiDialog::Setup(mGet( type, (is2d
			      ? tr("Import Multiple FaultStickSets 2D")
			      : tr("Import Multiple FaultStickSets")),
				tr("Import Multiple Faults"),
				tr("Import FaultSet")),mNoDlgTitle,
				mGetHelpKey(type)).modal(false))
    , fd_(BulkFaultAscIO::getDesc(mGetType(type),is2d,SI().zDomain()))
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
    uiString zdomlbl;
    if ( isfltset_ )
	zdomlbl = tr("FaultSet is in");
    else if ( isfss_ )
	zdomlbl = tr("Fault Stickset is in");
    else
	zdomlbl = tr("Fault is in");

    zdomselfld_ = new uiGenInput( this, zdomlbl,
		    BoolInpSpec(true,uiStrings::sTime(),uiStrings::sDepth()) );
    zdomselfld_->attach( alignedBelow, inpfld_ );
    zdomselfld_->setValue( SI().zIsTime() );
    mAttachCB( zdomselfld_->valueChanged, uiBulkFaultImport::zDomainCB );

    BufferStringSet stickselopt; stickselopt.add( sKeyAutoStickSel() )
						.add( sKeyInlCrlSep() )
						.add( sKeySlopeThres() );
    stickselfld_ = new uiGenInput( this, tr("Stick selection"),
				   StringListInpSpec(stickselopt) );
    stickselfld_->attach( alignedBelow, zdomselfld_ );
    mAttachCB( stickselfld_->valueChanged, uiBulkFaultImport::stickSelCB );

    thresholdfld_ = new uiGenInput( this, uiString::emptyString(),
				    DoubleInpSpec(1.0).setName("Threshold") );
    thresholdfld_->attach( rightOf, stickselfld_ );

    BufferStringSet sticksortopt;
    sticksortopt.add( sKeyGeometric() )
		.add( sKeyIndexed() ).add( sKeyFileOrder() );
    sortsticksfld_ = new uiGenInput( this, tr("Stick sorting"),
				     StringListInpSpec(sticksortopt) );
    sortsticksfld_->attach( alignedBelow, stickselfld_ );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
		mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, (uiObject*)sortsticksfld_ );

    if ( isfltset_ )
    {
	const ZDomain::Info& depthzinfo = SI().zInFeet() ? ZDomain::DepthFeet()
						    : ZDomain::DepthMeter();
	const EM::EMObjectType type = EM::EMObjectType::FltSet;
	fltsettimefld_ = new uiFaultSel( this, type, &ZDomain::TWT(), false );
	fltsetdepthfld_ = new uiFaultSel( this, type, &depthzinfo, false );
	fltsettimefld_->attach( alignedBelow, dataselfld_ );
	fltsetdepthfld_->attach( alignedBelow, dataselfld_ );

	mAttachCB( inpfld_->valueChanged, uiBulkFaultImport::inpChangedCB );
    }

    stickSelCB( nullptr );
    if ( isfltset_ )
	zDomainCB( nullptr );
}


uiBulkFaultImport::~uiBulkFaultImport()
{
    detachAllNotifiers();
    delete fd_;
}


const ZDomain::Info& uiBulkFaultImport::zDomain() const
{
    const UnitOfMeasure* selunit = fd_->bodyinfos_.validIdx(2) ?
			fd_->bodyinfos_[2]->selection_.unit_ : nullptr;
    bool isimperial = false;
    if ( selunit )
	isimperial = selunit->isImperial();

    if ( isASCIIFileInTime() )
	return ZDomain::TWT();
    else if ( isimperial )
	return ZDomain::DepthFeet();

    return ZDomain::DepthMeter();
}


bool uiBulkFaultImport::isASCIIFileInTime() const
{
    return zdomselfld_->getBoolValue();
}


void uiBulkFaultImport::zDomainCB( CallBacker* )
{
    const bool istime = isASCIIFileInTime();
    if ( isfltset_ )
    {
	fltsettimefld_->display( istime );
	fltsetdepthfld_->display( !istime );
    }

    EM::EMObjectType type = EM::EMObjectType::Flt3D;
    if ( isfss_ )
	type = EM::EMObjectType::FltSS3D;
    else if ( isfltset_ )
	type = EM::EMObjectType::FltSet;

    BulkFaultAscIO::updateDesc( *fd_, type, is2dfss_,
				istime ? ZDomain::Time() : ZDomain::Depth() );
}


void uiBulkFaultImport::stickSelCB( CallBacker* )
{
    if ( !stickselfld_ )
	return;

    const bool showthresfld
	= StringView(stickselfld_->text()) == sKeySlopeThres();
    const bool stickseldisplayed = stickselfld_->attachObj()->isDisplayed();
    thresholdfld_->display( stickseldisplayed && showthresfld );
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
	    flt->geometry().insertKnot( rc.toInt64(),
				       stick->crds_[crdidx], false );
	}
	sticknr++;
    }
}


void uiBulkFaultImport::inpChangedCB( CallBacker* )
{
    if ( isfltset_ )
    {
	if ( isASCIIFileInTime() )
	    fltsettimefld_->setInput( MultiID(inpfld_->baseName()) );
	else
	    fltsetdepthfld_->setInput( MultiID(inpfld_->baseName()) );
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
    BulkFaultAscIO aio( *fd_, strm, is2dfss_ );
    readInput( aio, pars );

    // TODO: Check if name exists, ask user to overwrite or give new name
    uiTaskRunner taskr( this );

    const char* typestr = isfss_ ? EM::FaultStickSet::typeStr()
						    : EM::Fault3D::typeStr();
    BufferString savernm = isfss_ ? "Saving FaultStickSets" :
		isfltset_ ? "Saving FaultSet" :"Saving Faults";

    EM::FaultSet3D* fltset = nullptr;

    if ( isfltset_ )
    {
	const IOObj* ioobj = isASCIIFileInTime() ? fltsettimefld_->ioobj()
						 : fltsetdepthfld_->ioobj();
	if ( !ioobj )
	    return false;

	EM::ObjectID oid = EM::EMM().createObject(
				EM::FaultSet3D::typeStr(), ioobj->name() );
	mDynamicCast(EM::FaultSet3D*,fltset,EM::EMM().getObject(oid))
	if ( isfltset_ && !fltset )
	    return false;
    }

    EM::FSStoFault3DConverter::Setup convsu;
    convsu.sortsticks_ = sortsticksfld_ &&
			StringView(sortsticksfld_->text()) == sKeyGeometric();
    if ( stickselfld_ && StringView(stickselfld_->text()) == sKeyInlCrlSep() )
	convsu.useinlcrlslopesep_ = true;

    if ( stickselfld_ && StringView(stickselfld_->text()) == sKeySlopeThres() )
	convsu.stickslopethres_ = thresholdfld_->getDValue();

    ExecutorGroup saver( savernm );
    TypeSet<MultiID> mids;
    const ZDomain::Info& zdominfo = zDomain();
    for ( int idx=0; idx<pars.size(); idx++ )
    {
	EM::EMManager& em = EM::EMM();
	RefMan<EM::EMObject> emobj = nullptr;
	EM::ObjectID emid;
	if ( isfltset_ )
	    emobj = em.createTempObject( typestr );
	else
	{
	    emid = em.createObject( typestr, pars[idx]->name_.buf() );
	    emobj = em.getObject( emid );
	}

	if ( emobj )
	    emobj->setZDomain( zdominfo );

	mDynamicCastGet( EM::Fault*, flt, emobj.ptr() );

	if ( !flt )
	    continue;

	ManagedObjectSet<EM::FaultStick> faultsticks;
	fillFaultSticks( *pars[idx], faultsticks );
	if ( isfss_ )
	    updateFaultStickSet( flt, faultsticks, is2dfss_ );
	else
	{
	    RefMan<EM::EMObject> emobj_fss = em.createTempObject(
						EM::FaultStickSet::typeStr() );
	    mDynamicCastGet(EM::Fault3D*, fault3d, emobj.ptr())
	    mDynamicCastGet(EM::FaultStickSet*, interfss, emobj_fss.ptr() );
	    updateFaultStickSet( interfss, faultsticks, is2dfss_ );
	    EM::FSStoFault3DConverter fsstof3d( convsu, *interfss, *fault3d );
	    fsstof3d.convert( true );
	}

	if ( !isfltset_)
	{
	    saver.add( emobj->saver() );
	    mids.add( emobj->multiID() );
	}
	else
	{
	    mDynamicCastGet( EM::Fault3D*, flt3d, emobj.ptr() );
	    fltset->addFault( flt3d );
	}
    }

    if ( isfltset_ )
    {
	saver.add( fltset->saver() );
	mids.add( fltset->multiID() );
    }

    if ( TaskRunner::execute( &taskr, saver ) )
    {
	for ( const auto& mid : mids )
	{
	    PtrMan<IOObj> obj = IOM().get( mid );
	    zdominfo.fillPar( obj->pars() );
	    IOM().commitChanges( *obj );
	}

	uiString msg = tr("%1 successfully imported.");
	if ( isfltset_ )
	{
	    msg.arg( uiStrings::sFaultSet() );
	    msg.addNewLine();
	    msg.append( tr("Do you want to continue importing FaultSet"),
									true );
	}
	else
	{
	    msg.append( tr("Do you want to import more %1?").arg(
		isfss_ ? uiStrings::sFaultStickSet(mPlural)
					: uiStrings::sFault(mPlural)), true );
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

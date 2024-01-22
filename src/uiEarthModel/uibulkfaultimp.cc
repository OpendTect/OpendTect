/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibulkfaultimp.h"

#include "emfault3d.h"
#include "emioobjinfo.h"
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
#include "uiiosurface.h"

#include "hiddenparam.h"

class uiBulkFaultImportHP
{
public:
    uiBulkFaultImportHP()
    {}

    ~uiBulkFaultImportHP()
    {}



    void setStickSelFld( uiGenInput* fld )
    {
	stickselfld_ = fld;
    }

    void setThresholdFld( uiGenInput* fld )
    {
	thresholdfld_ = fld;
    }

    void setZDomSelFld( uiGenInput* fld )
    {
	zdomselfld_ = fld;
    }

    void setFltSetDepthFld( uiIOObjSel* fld )
    {
	fltsetdepthfld_ = fld;
    }

    uiGenInput*		zdomselfld_	= nullptr;
    uiGenInput*		stickselfld_	= nullptr;
    uiGenInput*		thresholdfld_	= nullptr;
    uiIOObjSel*		fltsetdepthfld_ = nullptr;
};


static HiddenParam<uiBulkFaultImport,uiBulkFaultImportHP*> bfi_hpmgr_(nullptr);

uiGenInput* uiBulkFaultImport::stickselfld_()
{
    return bfi_hpmgr_.getParam( this )->stickselfld_;
}


uiGenInput* uiBulkFaultImport::zdomselfld_() const
{
    return bfi_hpmgr_.getParam( this )->zdomselfld_;
}


uiGenInput* uiBulkFaultImport::thresholdfld_()
{
    return bfi_hpmgr_.getParam( this )->thresholdfld_;
}


uiIOObjSel* uiBulkFaultImport::fltsetdepthfld_() const
{
    return isfltset_ ? bfi_hpmgr_.getParam(this)->fltsetdepthfld_ : nullptr;
}


class BulkFaultAscIO : public Table::AscIO
{
public:
BulkFaultAscIO( const Table::FormatDesc& fd, od_istream& strm, bool is2d )
    : Table::AscIO(fd)
    , strm_(strm)
    , finishedreadingheader_(false)
    , is2d_(is2d)
{}


static Table::FormatDesc* getDesc( EM::IOObjInfo::ObjectType type, bool is2d,
					    const ZDomain::Def& zdef )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "BulkFault" );

    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    createDescBody( *fd, type, is2d, zdef );
    return fd;
}


static void createDescBody( Table::FormatDesc& fd,
	EM::IOObjInfo::ObjectType type, bool is2d, const ZDomain::Def& zdef )
{
    BufferString typnm;
    if ( type == EM::IOObjInfo::FaultStickSet )
	typnm = "FaultStickSet name";
    else if ( type == EM::IOObjInfo::Fault )
	typnm = "Fault name";
    else
	typnm = "FaultSet name";

    fd.bodyinfos_ += new Table::TargetInfo( typnm , Table::Required );
    fd.bodyinfos_ += Table::TargetInfo::mkHorPosition( true );
    if ( zdef.isTime() )
	fd.bodyinfos_ += Table::TargetInfo::mkTimePosition( true );
    else
	fd.bodyinfos_ += Table::TargetInfo::mkDepthPosition( true );

    fd.bodyinfos_ += new Table::TargetInfo( "Stick number", IntInpSpec(),
	Table::Required );
    fd.bodyinfos_ += new Table::TargetInfo( "Node index", IntInpSpec(),
	Table::Optional );
    if ( is2d )
	fd.bodyinfos_ += new Table::TargetInfo( "Line name", StringInpSpec(),
	    Table::Required );
}


static void updateDescBody( Table::FormatDesc& fd,
	EM::IOObjInfo::ObjectType type, bool is2d, const ZDomain::Def& zdef )
{
    fd.bodyinfos_.erase();
    createDescBody( fd, type, is2d, zdef );
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
    double		udfval_;
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
	EM::IOObjInfo::FaultStickSet : \
    StringView(tp) == EMFaultSet3DTranslatorGroup::sGroupName() ? \
	EM::IOObjInfo::FaultSet : EM::IOObjInfo::Fault \

uiBulkFaultImport::uiBulkFaultImport(uiParent* p)
    : uiDialog(p, uiDialog::Setup(tr("Import Multiple Faults"), mNoDlgTitle,
	    mODHelpKey(mBulkFaultImportHelpID))
	    .modal(false))
    , fd_(BulkFaultAscIO::getDesc(EM::IOObjInfo::Fault,false,SI().zDomain()))
    , isfss_(false)
    , isfltset_(false)
    , is2dfss_(false)
{
    bfi_hpmgr_.setParam( this, new uiBulkFaultImportHP );

    init();
}


uiBulkFaultImport::uiBulkFaultImport( uiParent* p, const char* type, bool is2d )
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
    bfi_hpmgr_.setParam( this, new uiBulkFaultImportHP );
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

    bfi_hpmgr_.getParam(this)->setZDomSelFld( new uiGenInput( this, zdomlbl,
	BoolInpSpec(true,uiStrings::sTime(),uiStrings::sDepth())) );
    zdomselfld_()->attach( alignedBelow, inpfld_ );
    mAttachCB( zdomselfld_()->valuechanged,
					uiBulkFaultImport::zDomTypeChnagedCB );

    BufferStringSet stickselopt;
    stickselopt.add( sKeyAutoStickSel() )
	       .add( sKeyInlCrlSep() )
	       .add( sKeySlopeThres() );
    bfi_hpmgr_.getParam(this)->setStickSelFld( new uiGenInput(this,
		       tr("Stick selection"), StringListInpSpec(stickselopt)) );
    stickselfld_()->attach( alignedBelow, zdomselfld_() );
    mAttachCB( stickselfld_()->valuechanged, uiBulkFaultImport::stickSelCB );

    bfi_hpmgr_.getParam(this)->setThresholdFld( new uiGenInput(this,
					       uiString::emptyString(),
				   DoubleInpSpec(1.0).setName("Threshold")) );
    thresholdfld_()->attach( rightOf, stickselfld_() );

    BufferStringSet sticksortopt;
    sticksortopt.add( sKeyGeometric() )
		.add( sKeyIndexed() ).add( sKeyFileOrder() );
    sortsticksfld_ = new uiGenInput( this, tr("Stick sorting"),
				     StringListInpSpec(sticksortopt) );
    sortsticksfld_->attach( alignedBelow, stickselfld_() );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
		mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, (uiObject*)sortsticksfld_ );

    if ( isfltset_ )
    {
	const ZDomain::Info& depthzinfo = SI().zInFeet() ? ZDomain::DepthFeet()
	    : ZDomain::DepthMeter();
	const EM::IOObjInfo::ObjectType type = EM::IOObjInfo::FaultSet;
	fltsetnmfld_ = new uiFaultSel( this, type, &ZDomain::TWT(), false );
	auto* fltsetdepthfld = new uiFaultSel( this, type, &depthzinfo, false );
	fltsetnmfld_->attach( alignedBelow, dataselfld_ );
	fltsetdepthfld->attach( alignedBelow, dataselfld_ );

	mAttachCB( inpfld_->valueChanged, uiBulkFaultImport::inpChangedCB );
	bfi_hpmgr_.getParam(this)->setFltSetDepthFld( fltsetdepthfld );
    }

    mAttachCB( postFinalize(), uiBulkFaultImport::initGrpCB );
}


uiBulkFaultImport::~uiBulkFaultImport()
{
    detachAllNotifiers();
    delete fd_;
    bfi_hpmgr_.removeAndDeleteParam( this );
}


void uiBulkFaultImport::initGrpCB( CallBacker* )
{
    stickSelCB( nullptr );
    zDomTypeChnagedCB( nullptr );
}


void uiBulkFaultImport::zDomTypeChnagedCB( CallBacker* )
{
    const bool istime = zdomselfld_()->getBoolValue();
    if ( isfltset_ )
    {
	fltsetnmfld_->display( istime );
	fltsetdepthfld_()->display(!istime);
    }

    const ZDomain::Info& zinfo = istime ? ZDomain::TWT() :
	SI().depthsInFeet() ? ZDomain::DepthFeet() : ZDomain::DepthMeter();
    EM::IOObjInfo::ObjectType type = EM::IOObjInfo::Fault;
    if ( isfss_ )
	type = EM::IOObjInfo::FaultStickSet;
    else if ( isfltset_ )
	type = EM::IOObjInfo::FaultSet;

    BulkFaultAscIO::updateDescBody( *fd_, type, is2dfss_, zinfo.def_ );
}


void uiBulkFaultImport::stickSelCB( CallBacker* )
{
    if ( !stickselfld_() )
	return;

    const bool showthresfld
	= StringView(stickselfld_()->text()) == sKeySlopeThres();
    const bool stickseldisplayed = stickselfld_()->attachObj()->isDisplayed();
    thresholdfld_()->display( stickseldisplayed && showthresfld );
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
    if ( !flt )
	return;

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
	if ( zdomselfld_()->getBoolValue() )
	    fltsetnmfld_->setInput( MultiID(inpfld_->baseName()) );
	else
	    fltsetdepthfld_()->setInput(MultiID(inpfld_->baseName()));
    }
}


const ZDomain::Info& uiBulkFaultImport::zDomain() const
{
    const UnitOfMeasure* selunit = fd_->bodyinfos_.validIdx(2) ?
	fd_->bodyinfos_[2]->selection_.unit_ : nullptr;
    bool isimperial = false;
    if ( selunit )
	isimperial = selunit->isImperial();

    const bool istime = zdomselfld_()->getBoolValue();
    if ( istime )
	return ZDomain::TWT();
    else if ( isimperial )
	return ZDomain::DepthFeet();

    return ZDomain::DepthMeter();
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
	const IOObj* ioobj = zdomselfld_()->getBoolValue() ?
			fltsetnmfld_->ioobj() : fltsetdepthfld_()->ioobj();
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
    if ( stickselfld_() &&
			StringView(stickselfld_()->text()) == sKeyInlCrlSep() )
	convsu.useinlcrlslopesep_ = true;
    if ( stickselfld_() &&
			StringView(stickselfld_()->text()) == sKeySlopeThres() )
	convsu.stickslopethres_ = thresholdfld_()->getDValue();

    ExecutorGroup saver( savernm );
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

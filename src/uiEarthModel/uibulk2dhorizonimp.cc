/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibulk2dhorizonimp.h"


#include "binidvalset.h"
#include "emhorizon2d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "ioman.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "randcolor.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"

#include "uifileinput.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"

class Horizon2DBulkImporter : public Executor
{ mODTextTranslationClass(Horizon2DBulkImporter);
public:

    enum UndefTreat		{ Skip, Adopt, Interpolate };

Horizon2DBulkImporter( EM::Horizon2D& hor,
		       const BinIDValueSet* valset, UndefTreat udftreat )
    : Executor("2D Horizon Importer")
    , hor2d_(hor)
    , bvalset_(valset)
    , udftreat_(udftreat)
{
}


uiString uiMessage() const override
{ return tr("Horizon Import"); }

od_int64 totalNr() const override
{ return bvalset_ ? bvalset_->totalSize() : 0; }

uiString uiNrDoneText() const override
{ return tr("Positions written:"); }

od_int64 nrDone() const override
{ return nrdone_; }

int nextStep() override
{
    if ( !bvalset_ )
	return Executor::ErrorOccurred();

    if ( !bvalset_->next(pos_) )
	return Executor::Finished();

    BinID bid;
    const int nrvals = bvalset_->nrVals();
    mAllocVarLenArr( float, vals, nrvals )
    for ( int idx=0; idx<nrvals; idx++ )
	vals[idx] = mUdf(float);

    bvalset_->get( pos_, bid, mVarLenArr(vals) );
    if ( bid.isUdf() )
	return Executor::ErrorOccurred();

    const Pos::GeomID geomid( bid.inl() );
    if ( bid.inl() != prevlineidx_ )
    {
	prevlineidx_ = bid.inl();
	prevtrcnr_ =  -1;
	prevtrcval_ = mUdf(float);

	mDynamicCast( const Survey::Geometry2D*, curlinegeom_,
		      Survey::GM().getGeometry(geomid) );
	if ( !curlinegeom_ )
	    return Executor::ErrorOccurred();

	hor2d_.geometry().addLine( geomid );
    }

    const int curtrcnr = bid.crl();

    const float curval = vals[0];
    if ( mIsUdf(curval) && udftreat_==Skip )
	return MoreToDo();

    hor2d_.setPos( geomid, curtrcnr, curval, false );

    const int prevtrcnr = prevtrcnr_;

    if ( udftreat_==Interpolate && prevtrcnr>=0
				&& abs(curtrcnr-prevtrcnr)>1 )
    {
	interpolateAndSetVals( geomid, curtrcnr, prevtrcnr,
			       curval, prevtrcval_ );
    }

    prevtrcnr_ = curtrcnr;
    prevtrcval_ = curval;

    nrdone_++;
    return Executor::MoreToDo();
}


void interpolateAndSetVals( const Pos::GeomID& geomid, int curtrcnr,
			    int prevtrcnr, float curval, float prevval )
{
    if ( !curlinegeom_ )
	return;

    const int nrpos = abs( curtrcnr - prevtrcnr ) - 1;
    const bool isrev = curtrcnr < prevtrcnr;
    PosInfo::Line2DPos curpos, prevpos;
    if ( !curlinegeom_->data().getPos(curtrcnr,curpos)
	|| !curlinegeom_->data().getPos(prevtrcnr,prevpos) )
	return;

    const Coord vec = curpos.coord_ - prevpos.coord_;
    for ( int idx=1; idx<=nrpos; idx++ )
    {
	const int trcnr = isrev ? prevtrcnr - idx : prevtrcnr + idx;
	PosInfo::Line2DPos pos;
	if ( !curlinegeom_->data().getPos(trcnr,pos) )
	    continue;

	const Coord newvec = pos.coord_ - prevpos.coord_;
	const float sq = mCast(float,vec.sqAbs());
	const float prod = mCast(float,vec.dot(newvec));
	const float factor = mIsZero(sq,mDefEps) ? 0 : prod / sq;
	const float val = prevval + factor * ( curval - prevval );
	hor2d_.setPos( geomid, trcnr, val, false );
    }
}

protected:

    EM::Horizon2D&		hor2d_;
    const BinIDValueSet*	bvalset_;
    TypeSet<Pos::GeomID>	geomids_;
    const Survey::Geometry2D*	curlinegeom_		= nullptr;
    int				nrdone_			= 0;
    int				prevtrcnr_		= -1;
    float			prevtrcval_		= mUdf(float);
    int				prevlineidx_		= -1;
    BinIDValueSet::SPos		pos_;
    UndefTreat			udftreat_;
};


class BulkHorizon2DAscIO : public Table::AscIO
{
public:
BulkHorizon2DAscIO( const Table::FormatDesc& fd, od_istream& strm )
    : Table::AscIO(fd)
    , strm_(strm)
    , finishedreadingheader_(false)
{}


static Table::FormatDesc* getDesc( const ZDomain::Def& def )
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Bulk 2D Horizon" );

    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );

    createDescBody( fd, def );
    return fd;
}


static void createDescBody( Table::FormatDesc* fd, const ZDomain::Def& def )
{
    fd->bodyinfos_ += new Table::TargetInfo( "Horizon name", Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Line name", Table::Required );
    auto* ti = Table::TargetInfo::mkHorPosition( false, false );
    fd->bodyinfos_ += ti;
    auto* trcspti = new Table::TargetInfo( "Position", Table::Optional );
    trcspti->form(0).setName( "Trace Nr" );
    auto* spform = new Table::TargetInfo::Form( "SP Nr", IntInpSpec() );
    trcspti->add( spform );
    fd->bodyinfos_ += trcspti;
    auto* zti = new Table::TargetInfo( def.key(), FloatInpSpec(),
							    Table::Required );
    const Mnemonic::StdType type = def.isTime() ? Mnemonic::Time
						: Mnemonic::Dist;
    zti->setPropertyType( type );
    fd->bodyinfos_ += zti;
}


static void updateDesc( Table::FormatDesc& fd, const ZDomain::Def& def )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, def );
}


bool isTrNr() const
{ return formOf( false, 3 ) == 0; }


bool getData( BufferString& hornm, BufferString& linenm, Coord3& crd,
								int& trcnr )
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

    hornm = getText( 0 );
    linenm = getText(1);
    crd = getPos3D( 2, 3, 5, udfval_ );
    trcnr = mCast(int,getDValue(4, udfval_));
    return true;
}

    od_istream&		strm_;
    float		udfval_;
    bool		finishedreadingheader_;
};


uiBulk2DHorizonImport::uiBulk2DHorizonImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrImport(
				 tr("Multiple 2D Horizons")), mNoDlgTitle,
				 mODHelpKey(mBulkHorizonImportHelpID) )
			    .modal(false))
    , fd_(BulkHorizon2DAscIO::getDesc(SI().zDomain()))
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::ViewStyle::Table );

    zdomselfld_ = new uiGenInput( this, tr("Horizon is in"),
	BoolInpSpec(true,uiStrings::sTime(),uiStrings::sDepth()) );
    zdomselfld_->attach( alignedBelow, inpfld_ );
    zdomselfld_->setValue( SI().zIsTime() );
    mAttachCB( zdomselfld_->valueChanged, uiBulk2DHorizonImport::zDomainCB );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				    mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, zdomselfld_ );
    BufferStringSet udftreatments;
    udftreatments.add( "Skip" ).add( "Adopt" ).add( "Interpolate" );
    udftreatfld_ = new uiGenInput( this, tr("Undefined Z values"),
				   StringListInpSpec(udftreatments) );
    udftreatfld_->attach( alignedBelow, dataselfld_ );

    overwritefld_ = new uiGenInput( this, tr("When horizon exists"),
				BoolInpSpec(true,tr("Skip"),tr("Overwrite")) );
    overwritefld_->attach( alignedBelow, udftreatfld_ );
}


uiBulk2DHorizonImport::~uiBulk2DHorizonImport()
{
    delete fd_;
}


void uiBulk2DHorizonImport::zDomainCB( CallBacker* )
{
    BufferStringSet attrnms;
    BulkHorizon2DAscIO::updateDesc( *fd_, zdomselfld_->getBoolValue() ?
					ZDomain::Time() : ZDomain::Depth() );
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }


bool uiBulk2DHorizonImport::acceptOK( CallBacker* )
{
    const BufferString fnm( inpfld_->fileName() );
    if ( fnm.isEmpty() )
	mErrRet( uiStrings::phrEnter(tr("the input file name")) )

    od_istream strm( fnm );
    if ( !strm.isOK() )
	mErrRet(uiStrings::phrCannotOpen(uiStrings::sInputFile().toLower()))

    if ( !dataselfld_->commit() )
	return false;

    uiRetVal ret;
    const ZDomain::Info& zdominfo = Table::AscIO::zDomain( *fd_, 4, ret );
    if ( ret.isError() )
    {
	uiMSG().error( ret.messages().cat() );
	return false;
    }

    const bool doskip = overwritefld_->getBoolValue();
    BufferStringSet existinghorizons;
    EM::IOObjInfo::getObjectNames( EM::ObjectType::Hor2D, existinghorizons );

    ObjectSet<BinIDValueSet> alldata;
    BufferStringSet horizonstoimport;
    BufferStringSet skippedhorizons;

    BulkHorizon2DAscIO aio( *fd_, strm );
    BufferString hornm;
    BufferString linenm;
    Coord3 crd;
    int trnr=0;

    MouseCursorChanger mcc( MouseCursor::Wait );
    while ( aio.getData(hornm,linenm,crd,trnr) )
    {
	const Pos::GeomID geomid = Survey::GM().getGeomID( linenm );
        if ( mIsUdf(geomid) || mIsUdf(crd.z_) || hornm.isEmpty() )
	    continue;

	if ( existinghorizons.isPresent(hornm) && doskip )
	{
	    skippedhorizons.addIfNew( hornm );
	    continue;
	}

	BinIDValueSet* hordata;
	const int horidx = horizonstoimport.indexOf( hornm );
	if ( horidx < 0 )
	{
	    horizonstoimport.add( hornm );
	    hordata = new BinIDValueSet( 1, true );
	    alldata += hordata;
	}
	else
	    hordata = alldata[horidx];

	int nr = 0;
	if ( aio.isTrNr() )
	    nr = trnr;
	else if ( crd.isDefined() )
	{
	    mDynamicCastGet(const Survey::Geometry2D*,curlinegeom,
			    Survey::GM().getGeometry(geomid) );
	    if ( !curlinegeom )
		continue;

	    PosInfo::Line2DPos pos;
	    if ( !curlinegeom->data().getPos(crd.coord(),pos,
					     SI().inlDistance()) )
		continue;

	    nr = pos.nr_;
	}

	const BinID bid( geomid.asInt(), nr );
        hordata->add( bid, crd.z_ );
    }

    mcc.restore();

    uiTaskRunner impdlg( this );
    uiStringSet errors;
    for ( int idx=0; idx<horizonstoimport.size(); idx++ )
    {
	const BufferString& nm = horizonstoimport.get( idx );
	PtrMan<IOObj> ioobj = IOM().getLocal( nm,
				EMHorizon2DTranslatorGroup::sGroupName() );
	EM::ObjectID id = ioobj ? EM::EMM().getObjectID( ioobj->key() )
				: EM::ObjectID::udf();
	RefMan<EM::EMObject> emobj = EM::EMM().getObject( id );
	if ( emobj )
	    emobj->setBurstAlert( true );
	else
	{
	    id = EM::EMM().createObject( EM::Horizon2D::typeStr(), nm );
	    emobj = EM::EMM().getObject( id );
	}

	mDynamicCastGet(EM::Horizon2D*,hor,emobj.ptr())
	if ( ioobj )
	    hor->setMultiID( ioobj->key() );

	hor->setPreferredColor( OD::getRandomColor() );
	hor->setZDomain( zdominfo );
	hor->setBurstAlert( true );

	const auto udftype = sCast(Horizon2DBulkImporter::UndefTreat,
				   udftreatfld_->getIntValue());
	PtrMan<Horizon2DBulkImporter> importer =
		new Horizon2DBulkImporter( *hor, alldata[idx], udftype );
	if ( !TaskRunner::execute(&impdlg,*importer) )
	{
	    errors.add( tr("Could not import %1. %2.")
			.arg(nm).arg(importer->uiMessage()) );
	    continue;
	}

	PtrMan<Executor> saver = hor->saver();
	if ( !saver->execute() )
	{
	    errors.add( uiStrings::phrCannotSave(toUiString(hornm)) );
	    continue;
	}

	const MultiID& mid = hor->multiID();
	PtrMan<IOObj> horobj = IOM().get( mid );
	if ( horobj )
	{
	    zdominfo.fillPar( horobj->pars() );
	    IOM().commitChanges( *horobj );
	}
    }

    const uiString msg = tr("Import finished. Press 'Show Details' "
			    "for information.\n\n"
			    "Do you want to import more 2D Horizons?" );
    uiStringSet details;
    details.add( tr("The following horizons were imported:") );
    for ( const auto* nm : horizonstoimport )
	details.add( toUiString(nm->buf()) );
    details.add( toUiString("\n") );

    details.add( tr("The following horizons were skipped:") );
    for ( const auto* nm : skippedhorizons )
	details.add( toUiString(nm->buf()) );

    const bool retval = uiMSG().askGoOnWithDetails( msg, details );
    return !retval;
}

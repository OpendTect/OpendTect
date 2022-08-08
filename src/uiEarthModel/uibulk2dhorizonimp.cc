/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		July 2017
________________________________________________________________________

-*/


#include "uibulk2dhorizonimp.h"

#include "binidvalset.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "executor.h"
#include "posinfodetector.h"
#include "od_istream.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"
#include "uifileinput.h"
#include "emhorizon2d.h"
#include "survgeom2d.h"
#include "ioman.h"
#include "emsurfacetr.h"
#include "od_ostream.h"
#include "randcolor.h"

class Horizon2DBulkImporter : public Executor
{ mODTextTranslationClass(Horizon2DBulkImporter);
public:

    enum UndefTreat		{ Skip, Adopt, Interpolate };

Horizon2DBulkImporter( const BufferStringSet& lnms,
		       ObjectSet<EM::Horizon2D>& hors,
		       const BinIDValueSet* valset, UndefTreat udftreat )
    : Executor("2D Horizon Importer")
    , curlinegeom_(0)
    , hors_(hors)
    , bvalset_(valset)
    , prevlineidx_(-1)
    , nrdone_(0)
    , udftreat_(udftreat)
{
    for ( int lineidx=0; lineidx<lnms.size(); lineidx++ )
	geomids_ += Survey::GM().getGeomID( lnms.get(lineidx).buf() );
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
    if ( !bvalset_ ) return Executor::ErrorOccurred();
    if ( !bvalset_->next(pos_) ) return Executor::Finished();

    BinID bid;
    const int nrvals = bvalset_->nrVals();
    mAllocVarLenArr( float, vals, nrvals )
    for ( int idx=0; idx<nrvals; idx++ )
	vals[idx] = mUdf(float);

    bvalset_->get( pos_, bid, vals );
    if ( bid.inl() < 0 ) return Executor::ErrorOccurred();

    const Pos::GeomID geomid( bid.inl() );
    if ( bid.inl() != prevlineidx_ )
    {
	prevlineidx_ = bid.inl();
	prevtrcnrs_ = TypeSet<int>( nrvals, -1);
	prevtrcvals_ = TypeSet<float>( nrvals, mUdf(float) );

	mDynamicCast( const Survey::Geometry2D*, curlinegeom_,
		      Survey::GM().getGeometry(geomid) );
	if ( !curlinegeom_ )
	    return Executor::ErrorOccurred();

	for ( int hdx=0; hdx<hors_.size(); hdx++ )
	    hors_[hdx]->geometry().addLine( geomid );
    }

    const int curtrcnr = bid.crl();
    for ( int validx=0; validx<nrvals; validx++ )
    {
	if ( validx>=hors_.size() )
	    break;

	if ( !hors_[validx] )
	    continue;

	const float curval = vals[validx];
	if ( mIsUdf(curval) && udftreat_==Skip )
	    continue;
	const EM::SectionID sid = hors_[validx]->sectionID(0);
	hors_[validx]->setPos( sid, geomid, curtrcnr, curval, false );

	const int prevtrcnr = prevtrcnrs_[validx];

	if ( udftreat_==Interpolate && prevtrcnr>=0
				    && abs(curtrcnr-prevtrcnr)>1 )
	{
	    interpolateAndSetVals( validx, geomid, curtrcnr, prevtrcnr,
				   curval, prevtrcvals_[validx] );
	}

	prevtrcnrs_[validx] = curtrcnr;
	prevtrcvals_[validx] = curval;
    }

    nrdone_++;
    return Executor::MoreToDo();
}


void interpolateAndSetVals( int hidx, Pos::GeomID geomid, int curtrcnr,
			    int prevtrcnr, float curval, float prevval )
{
    if ( !curlinegeom_ ) return;

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
	hors_[hidx]->setPos( hors_[hidx]->sectionID(0), geomid,trcnr,val,false);
    }
}

protected:

    ObjectSet<EM::Horizon2D>&	hors_;
    const BinIDValueSet*	bvalset_;
    TypeSet<Pos::GeomID>	geomids_;
    const Survey::Geometry2D*	curlinegeom_;
    int				nrdone_;
    TypeSet<int>		prevtrcnrs_;
    TypeSet<float>		prevtrcvals_;
    int				prevlineidx_;
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


static Table::FormatDesc* getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Bulk 2D Horizon" );

    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
			StringInpSpec(sKey::FloatUdf()), Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Horizon name", Table::Required );
    fd->bodyinfos_ += new Table::TargetInfo( "Line name", Table::Required );
    Table::TargetInfo* ti = Table::TargetInfo::mkHorPosition( false, false );
    fd->bodyinfos_ += ti;
    Table::TargetInfo* trcspti = new Table::TargetInfo( "", Table::Optional );
    trcspti->form(0).setName( "Trace Nr" );
    Table::TargetInfo::Form* spform =
		new Table::TargetInfo::Form( "SP Nr", IntInpSpec() );
    trcspti->add( spform );
    fd->bodyinfos_ += trcspti;
    fd->bodyinfos_ += Table::TargetInfo::mkZPosition( true );
    return fd;
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
    , fd_(BulkHorizon2DAscIO::getDesc())
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::Table );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				    mODHelpKey(mTableImpDataSelwellsHelpID) );
    dataselfld_->attach( alignedBelow, inpfld_ );
    BufferStringSet udftreatments;
    udftreatments.add( "Skip" ).add( "Adopt" ).add( "Interpolate" );
    udftreatfld_ = new uiGenInput( this, tr("Undefined values"),
				   StringListInpSpec(udftreatments) );
    udftreatfld_->attach( alignedBelow, dataselfld_ );
}


uiBulk2DHorizonImport::~uiBulk2DHorizonImport()
{
    delete fd_;
}


#define mErrRet(s) { if ( !s.isEmpty() ) uiMSG().error(s); return false; }


#define mDeburstRet( retval, unreffn ) \
{ \
    for ( int horidx=0; horidx<hor2ds.size(); horidx++ ) \
    { \
	if ( hor2ds[horidx]->hasBurstAlert() ) \
	    hor2ds[horidx]->setBurstAlert( false ); \
\
	hor2ds[horidx]->unreffn(); \
    } \
    return retval; \
 } \

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
    ObjectSet<BinIDValueSet> data;
    BulkHorizon2DAscIO aio( *fd_, strm );
    BufferString hornm; Coord3 crd;
    BufferString linenm;
    BufferStringSet hornmset; BufferStringSet linenmset;
    BinIDValueSet* bidvs = 0;
    TypeSet<int> trnrset;
    int trnr=0;
    BufferString prevhornm;
    while ( aio.getData(hornm,linenm,crd,trnr) )
    {
	if ( prevhornm.isEmpty() )
	    prevhornm = hornm;

	if ( !crd.isDefined() || hornm.isEmpty() )
	    continue;
	else
	    hornmset.addIfNew(hornm);

	if ( linenm )
	    linenmset.addIfNew(linenm);

	trnrset.add(trnr);
	if ( !bidvs || (prevhornm != hornm) )
	    bidvs = new BinIDValueSet( 1, true );

	Pos::GeomID geomid = Survey::GM().getGeomID(linenm);
	int nr = 0;
	const Survey::Geometry2D* curlinegeom(0);

	if ( aio.isTrNr() )
	    nr = trnr;
	else if ( crd.isDefined() )
	{
	    mDynamicCast( const Survey::Geometry2D*, curlinegeom,
					Survey::GM().getGeometry(geomid) );
	    if ( !curlinegeom )
		return Executor::ErrorOccurred();
	    PosInfo::Line2DPos pos;
	    if ( !curlinegeom->data().getPos(crd.coord(),pos,
							SI().inlDistance()) )
		continue;
	    nr = pos.nr_;
	}

	const BinID bid( geomid.asInt(), nr );
	bidvs->add( bid, crd.z );

	if ( (prevhornm != hornm) || data.isEmpty() )
	{
	    prevhornm = hornm;
	    data += bidvs;
	}
    }

    // TODO: Check if name exists, ask user to overwrite or give new name
    BufferStringSet errors;
    uiTaskRunner dlg( this );
    ObjectSet<EM::Horizon2D> hor2ds;
    EM::EMManager& em = EM::EMM();
    PtrMan<IOObj> existioobj(0);
    BufferStringSet existinghornms;
    for ( int idx=0; idx<hornmset.size(); idx++ )
    {
	IOM().to( IOObjContext::Surf );
	if ( IOM().getLocal(hornm,0) )
	    existinghornms.add(hornmset.get(idx));
    }

    if ( !existinghornms.isEmpty() )
    {
	bool ret = uiMSG().askGoOn(tr("Horizons %1 already exist. "
	    "Do You want to overwrite them.")
	    .arg(toUiString(existinghornms.getDispString(5))));
	if ( !ret )
	    return false;
    }

    for ( int idx=0; idx<hornmset.size(); idx++ )
    {
	hor2ds.setEmpty();
	BufferString nm = hornmset.get( idx );
	PtrMan<IOObj> ioobj = IOM().getLocal( nm,
				EMHorizon2DTranslatorGroup::sGroupName() );
	EM::ObjectID id = ioobj ? em.getObjectID( ioobj->key() )
				: EM::ObjectID::udf();
	EM::EMObject* emobj = em.getObject(id);
	if ( emobj )
	    emobj->setBurstAlert( true );

	id = em.createObject( EM::Horizon2D::typeStr(), nm );
	mDynamicCastGet(EM::Horizon2D*,hor,em.getObject(id));
	if ( ioobj )
	    hor->setMultiID( ioobj->key() );

	hor->setPreferredColor( OD::getRandomColor() );
	hor->ref();
	hor->setBurstAlert( true );
	hor2ds += hor;

	PtrMan<Horizon2DBulkImporter> importr = new Horizon2DBulkImporter(
						linenmset, hor2ds, data[idx],
	    (Horizon2DBulkImporter::UndefTreat) udftreatfld_->getIntValue() );
	uiTaskRunner impdlg( this );
	if ( !TaskRunner::execute(&impdlg,*importr) )
	    mDeburstRet( false, unRef );
	PtrMan<Executor> saver = hor2ds[0]->saver();
	if ( !saver->execute() )
	    mErrRet(uiStrings::phrCannotSave(toUiString(hornm)))

    }

    uiString msg = tr( "2D Horizons successfully imported.\n\n"
		    "Do you want to import more 2D Horizons?" );
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

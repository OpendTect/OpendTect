/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Prajjaval Singh
 Date:		July 2017
________________________________________________________________________

-*/


#include "uibulk2dhorizonimp.h"

#include "binidvalset.h"
#include "emhorizon2d.h"
#include "bulk2dhorizonscanner.h"
#include "emhorizonascio.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "hiddenparam.h"
#include "ioman.h"
#include "od_helpids.h"
#include "od_istream.h"
#include "od_ostream.h"
#include "posinfodetector.h"
#include "randcolor.h"
#include "survgeom2d.h"
#include "survinfo.h"
#include "tableascio.h"
#include "tabledef.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uifileinput.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"

static HiddenParam<uiBulk2DHorizonImport,uiPushButton*> scanbut_(nullptr);

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


uiString uiMessage() const
{ return tr("Horizon Import"); }

od_int64 totalNr() const
{ return bvalset_ ? bvalset_->totalSize() : 0; }

uiString uiNrDoneText() const
{ return tr("Positions written:"); }

od_int64 nrDone() const
{ return nrdone_; }

int nextStep()
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

    bvalset_->get( pos_, bid, vals );
    if ( bid.inl() < 0 )
	return Executor::ErrorOccurred();

    const Pos::GeomID geomid = geomids_[bid.inl()];
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





uiBulk2DHorizonImport::uiBulk2DHorizonImport( uiParent* p )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrImport(
				 tr("Multiple 2D Horizons")), mNoDlgTitle,
				 mODHelpKey(mBulkHorizonImportHelpID) )
			    .modal(false))
    , fd_(EM::BulkHorizon2DAscIO::getDesc())
{
    setOkText( uiStrings::sImport() );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setExamStyle( File::Table );
    mAttachCB(inpfld_->valuechanged,uiBulk2DHorizonImport::scanButState);
    mAttachCB(inpfld_->valuechanged,uiBulk2DHorizonImport::descChg);

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
				    mODHelpKey(mTableImpDataSelwellsHelpID) );
    mAttachCB(dataselfld_->descChanged,uiBulk2DHorizonImport::descChg);
    dataselfld_->attach( alignedBelow, inpfld_ );

    auto scanbut = new uiPushButton( this, tr("Scan Input Files"),
			    mCB(this,uiBulk2DHorizonImport,scanPush), false );
    scanbut->attach( alignedBelow, dataselfld_ );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, scanbut );

    BufferStringSet udftreatments;
    udftreatments.add( "Skip" ).add( "Adopt" ).add( "Interpolate" );
    udftreatfld_ = new uiGenInput( this, tr("Undefined values"),
				   StringListInpSpec(udftreatments) );
    udftreatfld_->attach( alignedBelow, scanbut );
    udftreatfld_->attach( ensureBelow, sep );

    scanbut_.setParam( this, scanbut );

    mAttachCB(postFinalise(),uiBulk2DHorizonImport::scanButState);
}


uiBulk2DHorizonImport::~uiBulk2DHorizonImport()
{
    detachAllNotifiers();
    scanbut_.removeAndDeleteParam( this );
    delete scanner_;
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


void uiBulk2DHorizonImport::descChg( CallBacker* )
{
    delete scanner_;
    scanner_ = nullptr;
}


void uiBulk2DHorizonImport::scanButState( CallBacker* )
{
    const FixedString filenm = inpfld_->fileName();
    scanbut_.getParam( this )->setSensitive( !filenm.isEmpty() );
}


void uiBulk2DHorizonImport::scanPush( CallBacker* cb )
{
    if ( !dataselfld_->commit() )
	return;

    BufferString msg;
    if ( !EM::Horizon2DAscIO::isFormatOK(*fd_, msg) )
    {
	uiMSG().message( mToUiStringTodo(msg) );
	return;
    }

    if ( scanner_ )
    {
	if ( cb )
	    scanner_->launchBrowser();

	return;
    }

    BufferStringSet filenms;
    if ( !getFileNames(filenms) )
	return;

    scanner_ = new EM::BulkHorizon2DScanner( filenms, *fd_ );
    uiTaskRunner taskrunner( this );
    TaskRunner::execute( &taskrunner, *scanner_ );
    if ( cb )
	scanner_->launchBrowser();
}


bool uiBulk2DHorizonImport::getFileNames( BufferStringSet& filenames ) const
{
    if ( !*inpfld_->fileName() )
	mErrRet( tr("Please select input file(s)") )

    inpfld_->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File::exists(fnm) )
	{
	    uiString errmsg = tr("Cannot find input file:\n%1")
			    .arg(fnm);
	    filenames.setEmpty();
	    mErrRet( errmsg );
	}
    }

    return true;
}


bool uiBulk2DHorizonImport::doImport()
{
    scanPush( nullptr );
    if ( !scanner_ )
	return false;

    const int udfchoice = udftreatfld_->getIntValue();
    if ( scanner_->hasGaps() && udfchoice==0 )
    {
	const int res = uiMSG().askGoOn(
		tr("Horizon has gaps, but interpolation is turned off.\n"
		   "Continue?") );
	if ( res==0 )
	    return false;
    }

    TypeSet<BufferStringSet> linenms;
    scanner_->getLineNames( linenms );
    ObjectSet<BinIDValueSet> data = scanner_->getVals();
    if ( data.size() == 0 )
    {
	uiString msg = tr("No valid positions found\nPlease re-examine "
			  "input files and format definition");
	uiMSG().message( msg );
	return false;
    }
    BufferStringSet hornms;
    scanner_->getHorizonName( hornms );
    ObjectSet<EM::Horizon2D> horizons;
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Surf)->id_) );
    EM::EMManager& em = EM::EMM();
    BufferStringSet existinghornms;
    for ( int idx=0; idx<hornms.size(); idx++ )
    {
	IOM().to( MultiID(IOObjContext::getStdDirData(
						IOObjContext::Surf)->id_) );
	if ( IOM().getLocal(hornms.get(idx),0) )
	    existinghornms.add(hornms.get(idx));
    }

    if ( !existinghornms.isEmpty() )
    {
	bool ret = uiMSG().askGoOn(tr("Horizons %1 already exist. "
	    "Do You want to overwrite them.")
	    .arg(toUiString(existinghornms.getDispString(5))));
	if ( !ret )
	    return false;
    }

    ObjectSet<EM::Horizon2D> hor2ds;
    for ( int idx=0; idx<hornms.size(); idx++ )
    {
	hor2ds.setEmpty();
	BufferString nm = hornms.get( idx );
	PtrMan<IOObj> ioobj = IOM().getLocal( nm,
				EMHorizon2DTranslatorGroup::sGroupName() );
	EM::ObjectID id = ioobj ? em.getObjectID( ioobj->key() ) : -1;
	EM::EMObject* emobj = em.getObject(id);
	if ( emobj )
	    emobj->setBurstAlert( true );

	PtrMan<Executor> exec = ioobj ? em.objectLoader( ioobj->key() ) : 0;

	id = em.createObject( EM::Horizon2D::typeStr(), nm );
	mDynamicCastGet(EM::Horizon2D*,hor,em.getObject(id));
	if ( ioobj )
	    hor->setMultiID( ioobj->key() );
	hor->setPreferredColor(getRandomColor());
	hor->ref();
	hor->setBurstAlert( true );
	hor2ds += hor;

	PtrMan<Horizon2DBulkImporter> importr = new Horizon2DBulkImporter(
						linenms[idx], hor2ds, data[idx],
	    (Horizon2DBulkImporter::UndefTreat) udftreatfld_->getIntValue() );
	uiTaskRunner impdlg( this );
	if ( !TaskRunner::execute(&impdlg,*importr) )
	    mDeburstRet( false, unRef );
	PtrMan<Executor> saver = hor2ds[0]->saver();
	if ( !saver->execute() )
	    mErrRet(uiStrings::phrCannotSave(toUiString(hornms.get(idx))))

    }
    return true;
}

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

    // TODO: Check if name exists, ask user to overwrite or give new name

    const bool res = doImport();
    if ( !res )
	return false;

    uiString msg = tr( "2D Horizons successfully imported.\n\n"
		    "Do you want to import more 2D Horizons?" );
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

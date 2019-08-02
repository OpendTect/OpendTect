/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman Singh
 Date:          May 2008
________________________________________________________________________

-*/

#include "uiimphorizon2d.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uiempartserv.h"
#include "uifilesel.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseispartserv.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"

#include "binnedvalueset.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "horizon2dscanner.h"
#include "dbman.h"
#include "dbdir.h"
#include "randcolor.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "tabledef.h"
#include "file.h"
#include "emhorizon2d.h"
#include "emhorizonascio.h"
#include "od_helpids.h"

#include <math.h>


class Horizon2DImporter : public Executor
{ mODTextTranslationClass(Horizon2DImporter);
public:

    enum UndefTreat		{ Skip, Adopt, Interpolate };

Horizon2DImporter( const BufferStringSet& lnms, ObjectSet<EM::Horizon2D>& hors,
		   const BinnedValueSet* valset, UndefTreat udftreat )
    : Executor("2D Horizon Importer")
    , linenames_(lnms)
    , curlinegeom_(0)
    , hors_(hors)
    , bvalset_(valset)
    , prevlineidx_(-1)
    , nrdone_(0)
    , udftreat_(udftreat)
{
    for ( int lineidx=0; lineidx<lnms.size(); lineidx++ )
	geomids_ += SurvGeom::getGeomID( lnms.get(lineidx).buf() );
}


uiString message() const
{ return tr("Horizon Import"); }

od_int64 totalNr() const
{ return bvalset_ ? bvalset_->totalSize() : 0; }

uiString nrDoneText() const
{ return tr("Positions written:"); }

od_int64 nrDone() const
{ return nrdone_; }

int nextStep()
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

    const Pos::GeomID geomid = geomids_[bid.inl()];

    if ( bid.inl() != prevlineidx_ )
    {
	prevlineidx_ = bid.inl();
	prevtrcnrs_ = TypeSet<int>( nrvals, -1);
	prevtrcvals_ = TypeSet<float>( nrvals, mUdf(float) );

	const auto& geom2d = SurvGeom::get2D( geomid );
	if ( geom2d.isEmpty() )
	    return Executor::ErrorOccurred();

	curlinegeom_ = &geom2d;

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

	hors_[validx]->setZPos( geomid, curtrcnr, curval, false );

	if ( mIsUdf(curval) )
	    continue;

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
    if ( curlinegeom_ ) return;

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
	hors_[hidx]->setZPos( geomid, trcnr, val, false );
    }
}

protected:

    const BufferStringSet&	linenames_;
    ObjectSet<EM::Horizon2D>&	hors_;
    const BinnedValueSet*	bvalset_;
    GeomIDSet			geomids_;
    const SurvGeom2D*		curlinegeom_;
    int				nrdone_;
    TypeSet<int>		prevtrcnrs_;
    TypeSet<float>		prevtrcvals_;
    int				prevlineidx_;
    BinnedValueSet::SPos	pos_;
    UndefTreat			udftreat_;
};


uiImportHorizon2D::uiImportHorizon2D( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import 2D Horizon"),mNoDlgTitle,
		mODHelpKey(mImportHorizon2DHelpID) ).modal(false))
    , scanner_(0)
    , linesetnms_(*new BufferStringSet)
    , fd_(*EM::Horizon2DAscIO::getDesc())
    , readyForDisplay(this)
{
    enableSaveButton( tr("Display after import") );
    setCtrlStyle( RunAndClose );
    setOkText( uiStrings::sImport() );

    uiFileSel::Setup fssu( OD::TextContent ); fssu.selectMultiFile();
    inpfld_ = new uiFileSel( this, uiStrings::sInputASCIIFile(), fssu );
    inpfld_->newSelection.notify( mCB(this,uiImportHorizon2D,formatSel) );

    BufferStringSet hornms;
    uiEMPartServer::getAllSurfaceInfo( horinfos_, true );
    for ( int idx=0; idx<horinfos_.size(); idx++ )
	hornms.add( horinfos_[idx]->name );

    uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Horizon(s) to import") );
    horselfld_ = new uiListBox( this, su );
    horselfld_->addItems( hornms );
    horselfld_->attach( alignedBelow, inpfld_ );
    horselfld_->setAllowDuplicates( false );
    horselfld_->selectionChanged.notify(mCB(this,uiImportHorizon2D,formatSel));

    uiPushButton* addbut = new uiPushButton( horselfld_, tr("Add new"),
				mCB(this,uiImportHorizon2D,addHor), false );
    addbut->attach( rightTo, horselfld_->box() );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
			mODHelpKey(mTableImpDataSel2DSurfacesHelpID) );
    dataselfld_->attach( alignedBelow, horselfld_ );
    dataselfld_->descChanged.notify( mCB(this,uiImportHorizon2D,descChg) );

    scanbut_ = new uiPushButton( this, tr("Scan Input Files"),
				 mCB(this,uiImportHorizon2D,scanPush), false );
    scanbut_->attach( alignedBelow, dataselfld_ );

    uiSeparator* sep = new uiSeparator( this );
    sep->attach( stretchedBelow, scanbut_ );

    BufferStringSet udftreatments;
    udftreatments.add( "Skip" ).add( "Adopt" ).add( "Interpolate" );
    udftreatfld_ = new uiGenInput( this, tr("Undefined values"),
				   StringListInpSpec(udftreatments) );
    udftreatfld_->attach( alignedBelow, scanbut_ );
    udftreatfld_->attach( ensureBelow, sep );

    postFinalise().notify( mCB(this,uiImportHorizon2D,formatSel) );
}


uiImportHorizon2D::~uiImportHorizon2D()
{
    delete &linesetnms_;
    deepErase( horinfos_ );
}


void uiImportHorizon2D::descChg( CallBacker* cb )
{
    delete scanner_;
    scanner_ = 0;
}


void uiImportHorizon2D::formatSel( CallBacker* cb )
{
    BufferStringSet hornms;
    horselfld_->getChosen( hornms );
    const int nrhors = hornms.size();
    EM::Horizon2DAscIO::updateDesc( fd_, hornms );
    dataselfld_->updateSummary();
    dataselfld_->setSensitive( nrhors );
    scanbut_->setSensitive( *inpfld_->fileName() && nrhors );
}


void uiImportHorizon2D::addHor( CallBacker* )
{
    uiGenInputDlg dlg( this, uiStrings::phrAdd(uiStrings::sHorizon()),
		       uiStrings::sName(), new StringInpSpec() );
    if ( !dlg.go() ) return;

    const char* hornm = dlg.text();
    PtrMan<IOObj> existioobj = DBM().getByName( IOObjContext::Surf, hornm );
    if ( existioobj )
    {
	uiMSG().error(tr("Failed to add: a surface already "
                         "exists with name %1").arg(toUiString(hornm)));
	return;
    }

    horselfld_->addItem( toUiString(hornm) );
    horselfld_->setChosen( horselfld_->size()-1, true );
    horselfld_->scrollToBottom();
}


void uiImportHorizon2D::scanPush( CallBacker* cb )
{
    if ( horselfld_->nrChosen() < 1 )
	return;

    if ( !dataselfld_->commit() ) return;

    uiString msg;
    if ( !EM::Horizon2DAscIO::isFormatOK(fd_,msg) )
	{ uiMSG().error( msg ); return; }

    if ( scanner_ )
    {
	if ( cb )
	    scanner_->launchBrowser();

	return;
    }

    BufferStringSet filenms;
    if ( !getFileNames(filenms) ) return;

    scanner_ = new Horizon2DScanner( filenms, fd_ );
    uiTaskRunner taskrunner( this );
    TaskRunner::execute( &taskrunner, *scanner_ );
    if ( cb )
	scanner_->launchBrowser();
}


#define mDeburstRet( retval, unreffn ) \
{ \
    for ( int horidx=0; horidx<horizons.size(); horidx++ ) \
    { \
	if ( horizons[horidx]->hasBurstAlert() ) \
	    horizons[horidx]->setBurstAlert( false ); \
\
	horizons[horidx]->unreffn(); \
    } \
    return retval; \
}

bool uiImportHorizon2D::doImport()
{
    scanPush(0);
    if ( !scanner_ ) return false;

    const BinnedValueSet* valset = scanner_->getVals();
    if ( !valset || valset->totalSize() == 0 )
    {
	uiString msg = tr("No valid positions found\nPlease re-examine "
			  "input files and format definition");
	uiMSG().error( msg );
	return false;
    }

    BufferStringSet linenms;
    scanner_->getLineNames( linenms );
    BufferStringSet hornms;
    horselfld_->getChosen( hornms );
    ObjectSet<EM::Horizon2D> horizons;
    EM::ObjectManager& mgr = EM::Hor2DMan();
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Surf );
    SilentTaskRunnerProvider trprov;
    if ( dbdir )
    {
	for ( int idx=0; idx<hornms.size(); idx++ )
	{
	    BufferString nm = hornms.get( idx );
	    PtrMan<IOObj> ioobj = dbdir->getEntryByName( nm,
				    EMHorizon2DTranslatorGroup::sGroupName() );
	    RefMan<EM::Object> emobj =
		    ioobj ? mgr.fetchForEdit(ioobj->key(),trprov) : 0;
	    if ( emobj )
		emobj->setBurstAlert( true );

	    mDynamicCastGet(EM::Horizon2D*,hor,emobj.ptr());
	    if ( !hor )
	    {
		uiMSG().error( uiStrings::phrCannotLoad(
					    uiStrings::sHorizon().toLower()) );
		mDeburstRet( false, unRef );
	    }

	    BufferStringSet existinglines;
	    for ( int ldx=0; ldx<linenms.size(); ldx++ )
	    {
		const BufferString linenm = linenms.get( ldx );
		if ( hor->geometry().lineIndex(linenm) >= 0 )
		    existinglines.add( linenm );
	    }

	    const int nrexist = existinglines.size();
	    if ( nrexist > 0 )
	    {
		uiString msg = tr("Horizon %1 already exists for %2").arg(nm)
			     .arg((nrexist == 1
			     ? tr("2D line %1")
			     : tr("some 2D lines (%1) and will be overwritten"))
			     .arg(existinglines.getDispString(3, false)));
		if ( !uiMSG().askOverwrite(msg) )
		    mDeburstRet( false, unRef );
	    }

	    hor->setPreferredColor(getRandomColor());
	    hor->ref();
	    horizons += hor;
	    if ( !hor->hasBurstAlert() )
		hor->setBurstAlert( true );
	}
    }

    PtrMan<Horizon2DImporter> exec =
	new Horizon2DImporter( linenms, horizons, valset,
	    (Horizon2DImporter::UndefTreat) udftreatfld_->getIntValue() );
    uiTaskRunner impdlg( this );
    if ( !TaskRunner::execute(&impdlg,*exec) )
	mDeburstRet( false, unRef );

    objids_.erase();
    for ( int idx=0; idx<horizons.size(); idx++ )
    {
	PtrMan<Executor> saver = horizons[idx]->saver();
	if ( saver->execute() )
	    objids_ += horizons[idx]->id();
    }

    mDeburstRet( true, unRefNoDelete );
}


bool uiImportHorizon2D::acceptOK()
{
    if ( !checkInpFlds() ) return false;

    const bool res = doImport();
    if ( !res ) return false;

    if ( saveButtonChecked() )
	readyForDisplay.trigger();

    uiString msg = tr("2D Horizon successfully imported."
		      "\n\nDo you want to import more 2D Horizons?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


#define mErrRet(s) { uiMSG().error(s); return 0; }

bool uiImportHorizon2D::getFileNames( BufferStringSet& filenames ) const
{
    if ( !*inpfld_->fileName() )
	mErrRet( uiStrings::phrSelect(tr("input files")) )

    inpfld_->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File::exists(fnm) )
	{
	    uiString errmsg = uiStrings::phrCannotFind(tr("input file:\n%1")
	                    .arg(fnm));
	    filenames.setEmpty();
	    mErrRet( errmsg );
	}
    }

    return true;
}


bool uiImportHorizon2D::checkInpFlds()
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    if ( horselfld_->nrChosen() < 1 )
	mErrRet(tr("No horizons available"))

    if ( !dataselfld_->commit() )
	mErrRet( tr("Please define data format") );

    return true;
}


void uiImportHorizon2D::getEMObjIDs( DBKeySet& ids ) const
{ ids = objids_; }

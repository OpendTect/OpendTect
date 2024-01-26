/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiimphorizon2d.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uiempartserv.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseispartserv.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"

#include "binidvalset.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "horizon2dscanner.h"
#include "ioman.h"
#include "randcolor.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "tabledef.h"
#include "file.h"
#include "emhorizon2d.h"
#include "emhorizonascio.h"
#include "od_helpids.h"
#include "randcolor.h"

#include "hiddenparam.h"

#include <math.h>

//userInputGroup
class userInputGroup : public uiGroup
{ mODTextTranslationClass(userInputGroup)
public:
    userInputGroup( uiParent* p, Table::FormatDesc& fd )
	: uiGroup(p,"Display Group")
	, descChanged(this)
	, scanButtonPushed(this)
	, fd_(fd)
    {
	inpfld_ = new uiASCIIFileInput( this, true );
	inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
	mAttachCB(inpfld_->valueChanged,userInputGroup::formatSel);

	zdomselfld_ = new uiGenInput( this, tr("Horizon is in"),
	    BoolInpSpec(true,uiStrings::sTime(),uiStrings::sDepth()) );
	zdomselfld_->attach( alignedBelow, inpfld_ );
	zdomselfld_->setValue( SI().zIsTime() );
	mAttachCB(zdomselfld_->valueChanged,userInputGroup::zDomainCB);

	dataselfld_ = new uiTableImpDataSel( this, fd_,
	    mODHelpKey(mTableImpDataSel2DSurfacesHelpID) );
	dataselfld_->attach( alignedBelow, zdomselfld_ );
	mAttachCB(dataselfld_->descChanged,userInputGroup::descChg);

	scanbut_ = new uiPushButton( this, tr("Scan Input Files"),
	    mCB(this,userInputGroup,scanPush), false );
	scanbut_->attach( alignedBelow, dataselfld_ );

	uiSeparator* sep = new uiSeparator( this );
	sep->attach( stretchedBelow, scanbut_ );

	BufferStringSet udftreatments;
	udftreatments.add( "Skip" ).add( "Pass" ).add( "Interpolate" );
	udftreatfld_ = new uiGenInput( this, tr("Undefined values"),
	    StringListInpSpec(udftreatments) );
	udftreatfld_->attach( alignedBelow, scanbut_ );
	udftreatfld_->attach( ensureBelow, sep );

	timeoutputfld_ = new uiHorizonSel( this, true, &ZDomain::TWT(), false );
	timeoutputfld_->setLabelText(  tr("Output time horizon") );
	timeoutputfld_->attach( alignedBelow, udftreatfld_ );

	const ZDomain::Info& depthinfo = SI().zInFeet() ? ZDomain::DepthFeet() :
	    ZDomain::DepthMeter();
	depthoutputfld_ = new uiHorizonSel( this, true, &depthinfo, false );
	depthoutputfld_->setLabelText( tr("Output depth horizon") );
	depthoutputfld_->attach( alignedBelow, udftreatfld_ );
	mAttachCB(postFinalize(),userInputGroup::zDomainCB);
    }


    ~userInputGroup()
    {
	detachAllNotifiers();
    }


    uiIOObjSel* getWorkingOutFld() const
    {
	return isASCIIFileInTime() ? timeoutputfld_ : depthoutputfld_;
    }


    bool getFileNames( BufferStringSet& filenames ) const
    {
	if ( StringView(inpfld_->fileName()).isEmpty() )
	{
	    uiMSG().error( tr("Please select input file") );
	    return false;
	}

	inpfld_->getFileNames( filenames );
	for ( int idx=0; idx<filenames.size(); idx++ )
	{
	    const char* fnm = filenames[idx]->buf();
	    if ( !File::exists(fnm) )
	    {
		uiString errmsg = tr("Cannot find input file:\n%1")
		    .arg(fnm);
		filenames.setEmpty();
		uiMSG().error( errmsg );
		return false;
	    }
	}

	return true;
    }


    const char* getFileName() const
    {
	return inpfld_->fileName();
    }

    bool commitChanges()
    {
	return dataselfld_->commit();
    }


    int getUdfChoice() const
    {
	return udftreatfld_->getIntValue();
    }


    void setHorizon( EM::Horizon2D* hor )
    {
	horizon_ = hor;
    }

protected:

    bool isASCIIFileInTime() const
    {
	return zdomselfld_->getBoolValue();
    }

    void zDomainCB( CallBacker* cb )
    {
	NotifyStopper ns1( inpfld_->valueChanged );
	const bool istime = isASCIIFileInTime();
	const ZDomain::Info& zinfo = istime ? ZDomain::TWT() :
	    SI().depthsInFeet() ? ZDomain::DepthFeet() : ZDomain::DepthMeter();

	timeoutputfld_->display( istime );
	depthoutputfld_->display( !istime );

	EM::Horizon2DAscIO::updateDesc_( fd_, zinfo.def_ );
    }


    void descChg( CallBacker* )
    {
	descChanged.trigger();
    }


    void formatSel( CallBacker* )
    {
	BufferStringSet hornms;
	dataselfld_->updateSummary();
	dataselfld_->setSensitive( true );
	scanbut_->setSensitive( *inpfld_->fileName() );
	EM::Horizon2DAscIO::updateDesc_( fd_,
		    isASCIIFileInTime() ? ZDomain::Time() : ZDomain::Depth() );
    }


    const ZDomain::Info& zDomain() const
    {
	uiRetVal ret;
	return Table::AscIO::zDomain( fd_, 3, ret );
    }


    void scanPush( CallBacker* cb )
    {
	if ( !dataselfld_->commit() )
	    return;

	scanButtonPushed.trigger();
    }


private:

    uiFileInput*		    inpfld_;
    uiPushButton*		    scanbut_;
    uiTableImpDataSel*		    dataselfld_;
    uiGenInput*			    udftreatfld_;
    uiGenInput*			    zdomselfld_;
    uiIOObjSel*			    timeoutputfld_;
    uiIOObjSel*			    depthoutputfld_;
    Table::FormatDesc&		    fd_;
    Horizon2DScanner*		    scanner_;
    RefMan<EM::Horizon2D>	    horizon_;
public:
    Notifier<userInputGroup>	    descChanged;
    Notifier<userInputGroup>	    scanButtonPushed;

};


//Horizon2DImporter
class Horizon2DImporter : public Executor
{ mODTextTranslationClass(Horizon2DImporter);
public:

    enum UndefTreat		{ Skip, Adopt, Interpolate };

Horizon2DImporter( const BufferStringSet& lnms, EM::Horizon2D& hor,
		   const BinIDValueSet* valset, UndefTreat udftreat )
    : Executor("2D Horizon Importer")
    , linenames_(lnms)
    , curlinegeom_(0)
    , hor_(&hor)
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
    if ( !bvalset_ )
	return ErrorOccurred();

    if ( !bvalset_->next(pos_) )
	return Finished();

    BinID bid;
    const int nrvals = bvalset_->nrVals();
    mAllocVarLenArr( float, vals, nrvals )
    for ( int idx=0; idx<nrvals; idx++ )
	vals[idx] = mUdf(float);

    bvalset_->get( pos_, bid, vals );
    if ( bid.inl() < 0 )
	return ErrorOccurred();

    const Pos::GeomID geomid = geomids_[bid.inl()];
    if ( bid.inl() != prevlineidx_ )
    {
	prevlineidx_ = bid.inl();
	prevtrcnrs_ = TypeSet<int>( nrvals, -1);
	prevtrcvals_ = TypeSet<float>( nrvals, mUdf(float) );

	mDynamicCast( const Survey::Geometry2D*, curlinegeom_,
		      Survey::GM().getGeometry(geomid) );
	if ( !curlinegeom_ )
	    return ErrorOccurred();

	hor_->geometry().addLine( geomid );
    }

    const int curtrcnr = bid.crl();
    for ( int validx=0; validx<nrvals; validx++ )
    {
	const float curval = vals[validx];
	if ( mIsUdf(curval) && udftreat_==Skip )
	    continue;

	hor_->setPos( geomid, curtrcnr, curval, false );

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
    return MoreToDo();
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
	const float sq = sCast(float,vec.sqAbs());
	const float prod = sCast(float,vec.dot(newvec));
	const float factor = mIsZero(sq,mDefEps) ? 0 : prod / sq;
	const float val = prevval + factor * ( curval - prevval );
	hor_->setPos( geomid,trcnr,val,false);
    }
}

protected:

    const BufferStringSet&	linenames_;
    RefMan<EM::Horizon2D>	hor_;
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

//uiImportHorizon2D
static HiddenParam<uiImportHorizon2D,userInputGroup*>
					    uiimporthorizon2dhpmgr_(nullptr);
uiImportHorizon2D::uiImportHorizon2D( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import 2D Horizon"),mNoDlgTitle,
		mODHelpKey(mImportHorizon2DHelpID) ).modal(false))
    , scanner_(nullptr)
    , linesetnms_(*new BufferStringSet)
    , fd_(*EM::Horizon2DAscIO::getDesc_(SI().zDomain()))
    , readyForDisplay(this)
{
    enableSaveButton( tr("Display after import") );
    setCtrlStyle( RunAndClose );
    setOkText( uiStrings::sImport() );

    auto* userinpgrp = new userInputGroup( this, fd_ );
    mAttachCB(userinpgrp->descChanged,uiImportHorizon2D::descChg);
    mAttachCB(userinpgrp->scanButtonPushed,uiImportHorizon2D::scanPush);
    uiimporthorizon2dhpmgr_.setParam( this, userinpgrp );
}


uiImportHorizon2D::~uiImportHorizon2D()
{
    detachAllNotifiers();
    delete &linesetnms_;
    deepErase( horinfos_ );
    uiimporthorizon2dhpmgr_.removeParam( this );
}


void uiImportHorizon2D::zDomainCB( CallBacker* cb )
{
}



void uiImportHorizon2D::descChg( CallBacker* )
{
    delete scanner_;
    scanner_ = nullptr;
}


void uiImportHorizon2D::formatSel( CallBacker* )
{
}


void uiImportHorizon2D::addHor( CallBacker* )
{
}


const ZDomain::Info& uiImportHorizon2D::zDomain() const
{
    uiRetVal ret;
    return Table::AscIO::zDomain( fd_, 3, ret );
}


void uiImportHorizon2D::scanPush( CallBacker* cb )
{
    BufferString msg;
    if ( !EM::Horizon2DAscIO::isFormatOK(fd_, msg) )
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

    scanner_ = new Horizon2DScanner( filenms, fd_, zDomain() );
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
    emobjids_.setEmpty();
    scanPush( nullptr );
    if ( !scanner_ )
	return false;

    const BinIDValueSet* valset = scanner_->getVals();
    if ( !valset || valset->totalSize() == 0 )
    {
	uiString msg = tr("No valid positions found\nPlease re-examine "
	    "input files and format definition");
	uiMSG().message( msg );
	return false;
    }

    auto* inpgrp = uiimporthorizon2dhpmgr_.getParam( this );
    const int udfchoice = inpgrp->getUdfChoice();
    if ( scanner_->hasGaps() && udfchoice==0 )
    {
	const int res = uiMSG().askGoOn(
	    tr("Horizon has gaps, but interpolation is turned off.\n"
	    "Continue?") );
	if ( res==0 )
	    return false;
    }

    const IOObj* ioobj = inpgrp->getWorkingOutFld()->ioobj();
    if ( !ioobj )
    {
	uiMSG().error( tr("Error in creating output object") );
	return false;
    }

    EM::EMManager& em = EM::EMM();
    const EM::ObjectID id = em.createObject( EM::Horizon2D::typeStr(),
							    ioobj->name() );
    mDynamicCastGet(EM::Horizon2D*,horizon,em.getObject(id));
    if ( !horizon )
    {
	uiMSG().error(
	   tr("Wrong output object detected, Horizon2D object was expected") );
	return false;
    }

    horizon->setMultiID( ioobj->key() );
    horizon->setName( ioobj->name() );
    BufferStringSet linenms;
    scanner_->getLineNames( linenms );
    const ZDomain::Info& zinfo = zDomain();
    horizon->setZDomain( zinfo );
    PtrMan<Horizon2DImporter> exec =
	new Horizon2DImporter( linenms, *horizon, valset,
				    (Horizon2DImporter::UndefTreat)udfchoice );
    uiTaskRunner impdlg( this );
    if ( !TaskRunner::execute(&impdlg,*exec) )
    {
	uiMSG().error( tr("Error while importing data") );
	if ( horizon->hasBurstAlert() )
	    horizon->setBurstAlert( false );

	return false;
    }

    if ( horizon->hasBurstAlert() )
	horizon->setBurstAlert( false );

    PtrMan<Executor> saver = horizon->saver();
    if ( !saver || !saver->execute() )
    {
	uiMSG().error( tr("2D Horizon not saved. "
	    "Please check access permission") );
	return false;
    }

    zinfo.fillPar( ioobj->pars() );
    ioobj->pars().update( sKey::CrFrom(), inpgrp->getFileName() );
    ioobj->updateCreationPars();
    IOM().commitChanges( *ioobj );
    inpgrp->setHorizon( horizon );
    emobjids_.add( horizon->id() );
    return true;
}


bool uiImportHorizon2D::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() )
	return false;

    const bool res = doImport();
    if ( !res )
	return false;

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
    auto* fld = uiimporthorizon2dhpmgr_.getParam( this );
    return fld->getFileNames( filenames );
}


bool uiImportHorizon2D::checkInpFlds()
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) )
	return false;

    if ( !uiimporthorizon2dhpmgr_.getParam(this)->commitChanges())
	mErrRet( tr("Please define data format") );

    return true;
}


void uiImportHorizon2D::getEMObjIDs( TypeSet<EM::ObjectID>& ids ) const
{ ids = emobjids_; }

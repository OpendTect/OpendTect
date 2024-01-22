/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiimpfault.h"

#include "bufstring.h"
#include "ctxtioobj.h"
#include "emfault.h"
#include "emfault3d.h"
#include "emfsstofault3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "file.h"
#include "filepath.h"
#include "hiddenparam.h"
#include "ioobj.h"
#include "lmkemfaulttransl.h"
#include "oddirs.h"
#include "streamconn.h"
#include "survinfo.h"
#include "tabledef.h"
#include "od_istream.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uistrings.h"
#include "uiiosurface.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "od_helpids.h"

class uiFaultImportHP
{
public:
    uiFaultImportHP()
    {}

    ~uiFaultImportHP()
    {}



    void setDepthOutField( uiIOObjSel* fld )
    {
	depthoutfld_ = fld;
    }

    void setZDomSelFld( uiGenInput* fld )
    {
	zdomselfld_ = fld;
    }

    uiGenInput*		zdomselfld_	= nullptr;
    uiIOObjSel*		depthoutfld_	= nullptr;
};

#define mGet( tp, fss, f3d ) \
    StringView(tp) == EMFaultStickSetTranslatorGroup::sGroupName() ? fss : f3d

#define mGetCtxt(tp) \
    mGet( tp, mIOObjContext(EMFaultStickSet), mIOObjContext(EMFault3D) )

#define mGetHelpKey(tp) \
    mGet( tp, (is2d ? mODHelpKey(mImportFaultStick2DHelpID) \
		    : mODHelpKey(mImportFaultStick3DHelpID) ), \
    mODHelpKey(mImportFaultHelpID) )

static HiddenParam<uiImportFault,uiFaultImportHP*> uiimportfaulthpmgr_(nullptr);

uiImportFault::uiImportFault( uiParent* p, const char* type, bool is2d )
    : uiDialog(p,uiDialog::Setup(
			mGet(type,(is2d ? tr("Import FaultStickSet 2D")
					: tr("Import FaultStickSet")),
			     tr("Import Fault") ),
		mNoDlgTitle,mGetHelpKey(type)).modal(false))
    , isfss_(mGet(type,true,false))
    , fd_(nullptr)
    , type_(type)
    , typefld_(nullptr)
    , sortsticksfld_(0)
    , stickselfld_(nullptr)
    , thresholdfld_(nullptr)
    , is2d_(is2d)
    , importReady(this)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );
    enableSaveButton( tr("Display after import") );
}


const char* uiImportFault::sKeyAutoStickSel()	{ return "Auto"; }
const char* uiImportFault::sKeyInlCrlSep()	{ return "Inl/Crl separation"; }
const char* uiImportFault::sKeySlopeThres()	{ return "Slope threshold"; }
const char* uiImportFault::sKeyGeometric()	{ return "Geometric"; }
const char* uiImportFault::sKeyIndexed()	{ return "Indexed"; }
const char* uiImportFault::sKeyFileOrder()	{ return "File order"; }


void uiImportFault::createUI()
{
    uiimportfaulthpmgr_.setParam( this, new uiFaultImportHP() );
    infld_ = new uiASCIIFileInput( this, true );
    mAttachCB( infld_->valuechanged, uiImportFault::inputChgd );
    uiString zdomlbl;
    if ( isfss_ )
	zdomlbl = tr("Fault Stickset is in");
    else
	zdomlbl = tr("Fault is in");

    auto* zdomselfld = new uiGenInput( this, zdomlbl,
	BoolInpSpec(true,uiStrings::sTime(),uiStrings::sDepth()) );
    zdomselfld->attach( alignedBelow, infld_ );
    zdomselfld->setValue( SI().zIsTime() );
    mAttachCB( zdomselfld->valueChanged, uiImportFault::zDomainCB );
    uiimportfaulthpmgr_.getParam(this)->setZDomSelFld( zdomselfld );
    if ( !isfss_ )
    {
	BufferStringSet types;
	types.add( "Plain ASCII" );
	typefld_ = new uiGenInput( this, uiStrings::sType(),
				   StringListInpSpec(types) );
	mAttachCB( typefld_->valuechanged, uiImportFault::typeSel );
	typefld_->attach( alignedBelow, zdomselfld );

	formatfld_ = new uiFileInput( this, tr("Input Landmark formatfile"),
				      uiFileInput::Setup(uiFileDialog::Gen)
				      .filter("*.fault_fmt") );
	formatfld_->attach( alignedBelow, typefld_ );

	BufferStringSet stickselopt; stickselopt.add( sKeyAutoStickSel() )
						.add( sKeyInlCrlSep() )
						.add( sKeySlopeThres() );
	stickselfld_ = new uiGenInput( this, tr("Stick selection"),
				    StringListInpSpec(stickselopt) );
	stickselfld_->attach( alignedBelow, typefld_ );
	mAttachCB( stickselfld_->valuechanged, uiImportFault::stickSel );

	thresholdfld_ = new uiGenInput(this, uiString::emptyString(),
				DoubleInpSpec(1.0).setName("Threshold") );
	thresholdfld_->attach( rightOf, stickselfld_ );

	BufferStringSet sticksortopt; sticksortopt.add( sKeyGeometric() )
						  .add( sKeyIndexed() )
						  .add( sKeyFileOrder() );
	sortsticksfld_ = new uiGenInput( this, tr("Stick sorting"),
					 StringListInpSpec(sticksortopt) );
	sortsticksfld_->attach( alignedBelow, stickselfld_ );
    }

    dataselfld_ = new uiTableImpDataSel( this, *fd_, isfss_ ? (is2d_ ?
		mODHelpKey(mTableImpDataSelFaultStickSet2DHelpID) :
		mODHelpKey(mTableImpDataSelFaultStickSet3DHelpID) )
			: mODHelpKey(mTableImpDataSelFaultsHelpID) );
    if ( isfss_  )
	dataselfld_->attach( alignedBelow, zdomselfld );
    else
	dataselfld_->attach( alignedBelow, sortsticksfld_ );

    const ZDomain::Info& depthzinfo = SI().zInFeet() ? ZDomain::DepthFeet() :
	ZDomain::DepthMeter();
    const EM::IOObjInfo::ObjectType type =
			     isfss_ ? EM::IOObjInfo::FaultStickSet
				    : EM::IOObjInfo::Fault;

    outfld_ = new uiFaultSel( this, type, &ZDomain::TWT(), false );
    auto* depthoutfld = new uiFaultSel( this, type, &depthzinfo, false );
    outfld_->attach( alignedBelow, dataselfld_ );
    depthoutfld->attach( alignedBelow, dataselfld_ );
    uiimportfaulthpmgr_.getParam(this)->setDepthOutField( depthoutfld );
    mAttachCB( postFinalize(), uiImportFault::initGrpCB );
}


uiImportFault::~uiImportFault()
{
    detachAllNotifiers();
    uiimportfaulthpmgr_.removeParam( this );
}


uiGenInput* uiImportFault::zdomtypefld_() const
{
    return uiimportfaulthpmgr_.getParam(this)->zdomselfld_;
}


uiIOObjSel* uiImportFault::depthoutfld_() const
{
    return uiimportfaulthpmgr_.getParam(this)->depthoutfld_;
}


void uiImportFault::initGrpCB( CallBacker* )
{
    typeSel( nullptr );
    stickSel( nullptr );
    zDomainCB( nullptr );
}


void uiImportFault::inputChgd( CallBacker* )
{
    FilePath fnmfp( infld_->fileName() );
    fnmfp.setExtension( "" );
    getValidOutFld()->setInputText(fnmfp.fileName());
}


void uiImportFault::zDomainCB( CallBacker* )
{
    const bool istime = zdomtypefld_()->getBoolValue();
    outfld_->display( istime );
    depthoutfld_()->display(!istime);
    EM::FaultAscIO::updateDesc( *fd_, is2d_, istime ? ZDomain::Time()
	: ZDomain::Depth() );
}


uiIOObjSel* uiImportFault::getValidOutFld() const
{
    return zdomtypefld_()->getBoolValue() ? outfld_ : depthoutfld_();
}


void uiImportFault::typeSel( CallBacker* )
{
    if ( !typefld_ )
	return;

    const int tp = typefld_->getIntValue();
    typefld_->display( typefld_->nrElements()>1 );
    formatfld_->display( tp == 1 );
    dataselfld_->display( tp == 0 );
    sortsticksfld_->display( tp == 0 );
    stickselfld_->display( tp == 0 );
    stickSel( nullptr );
}


const ZDomain::Info& uiImportFault::zDomain() const
{
    const UnitOfMeasure* selunit = fd_->bodyinfos_.validIdx(1) ?
	fd_->bodyinfos_[1]->selection_.unit_ : nullptr;
    bool isimperial = false;
    if ( selunit )
	isimperial = selunit->isImperial();

    const bool istime = zdomtypefld_()->getBoolValue();
    if ( istime )
	return ZDomain::TWT();
    else if ( isimperial )
	return ZDomain::DepthFeet();

    return ZDomain::DepthMeter();
}


void uiImportFault::stickSel( CallBacker* )
{
    if ( !stickselfld_ )
	return;

    const bool showthresfld
	= StringView(stickselfld_->text()) == sKeySlopeThres();
    const bool stickseldisplayed = stickselfld_->attachObj()->isDisplayed();
    thresholdfld_->display( stickseldisplayed && showthresfld );
}


#define mErrRet(s) { \
    uiMSG().error(s); \
    if ( fault ) fault->unRef(); \
    return false; }

EM::Fault* uiImportFault::createFault() const
{
    const char* fltnm = getValidOutFld()->getInput();
    EM::EMManager& em = EM::EMM();
    const char* typestr = isfss_ ? EM::FaultStickSet::typeStr()
				 : EM::Fault3D::typeStr();
    const EM::ObjectID emid = em.createObject( typestr, fltnm );
    mDynamicCastGet(EM::Fault*,fault,em.getObject(emid))
    fault->setZDomain( zDomain() );
    return fault;
}


bool uiImportFault::handleLMKAscii()
{
    EM::Fault* fault = createFault();
    mDynamicCastGet(EM::Fault3D*,fault3d,fault)
    if ( !fault3d )
	mErrRet( uiStrings::phrCannotCreate(uiStrings::sFault()) );

    fault3d->ref();

    PtrMan<lmkEMFault3DTranslator> transl =
	lmkEMFault3DTranslator::getInstance();
    Conn* conn = new StreamConn( infld_->fileName(), true );

    PtrMan<Executor> exec =
	transl->reader( *fault3d, conn, formatfld_->fileName() );

    if ( !exec )
	mErrRet( uiStrings::phrCannotImport(uiStrings::sFault()));

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *exec ) )
	mErrRet( taskrunner.lastMsg() );

    if ( saveButtonChecked() )
    {
	importReady.trigger();
	fault->unRefNoDelete();
    }
    else
	fault->unRef();

    const char* tp = fault3d ? "FaultSets" : "FaultStickSets";
    uiString msg = tr("%1 successfully imported."
		      "\n\nDo you want to import more %1?")
		      .arg(tp);
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


bool uiImportFault::handleAscii()
{
    EM::Fault* fault = createFault();
    if ( !fault )
	mErrRet( uiStrings::phrCannotCreate(isfss_ ?
	    uiStrings::sFaultStickSet() : uiStrings::sFault()) )

    fault->ref();

    od_istream strm( infld_->fileName() );
    if ( !strm.isOK() )
	mErrRet( uiStrings::sCantOpenInpFile() )

    mDynamicCastGet(EM::Fault3D*,fault3d,fault)

    uiString tp = fault3d ? uiStrings::sFault() : uiStrings::sFaultStickSet();

    const bool res = getFromAscIO( strm, *fault );
    if ( !res )
	mErrRet( uiStrings::phrImport(tp));
    PtrMan<Executor> exec = fault->saver();
    bool isexec = exec->execute();
    if ( !isexec )
	mErrRet( uiStrings::phrCannotSave(tp) );
    if ( saveButtonChecked() )
    {
	importReady.trigger();
	fault->unRefNoDelete();
    }
    else
	fault->unRef();

    uiString msg = tr("%1 successfully imported."
		      "\n\nDo you want to import more objects?")
		      .arg(tp).arg(fault3d ? uiStrings::sFault(mPlural) :
					   uiStrings::sFaultStickSet(mPlural));
    bool ret= uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


bool uiImportFault::getFromAscIO( od_istream& strm, EM::Fault& flt )
{
    EM::FaultAscIO ascio( *fd_ );
    mDynamicCastGet( EM::Fault3D*, fault3d, &flt );
    if ( !fault3d )
	return ascio.get( strm, flt, true, false );

    EM::FSStoFault3DConverter::Setup convsu;
    convsu.sortsticks_ = sortsticksfld_ &&
			StringView(sortsticksfld_->text()) == sKeyGeometric();
    if ( stickselfld_ && StringView(stickselfld_->text()) == sKeyInlCrlSep() )
	convsu.useinlcrlslopesep_ = true;
    if ( stickselfld_ && StringView(stickselfld_->text()) == sKeySlopeThres() )
	convsu.stickslopethres_ = thresholdfld_->getDValue();

    EM::EMObject* emobj = EM::FaultStickSet::create( EM::EMM() );
    mDynamicCastGet( EM::FaultStickSet*, interfss, emobj );
    const bool sortsticks = sortsticksfld_ &&
			StringView( sortsticksfld_->text() ) == sKeyIndexed();
    bool res = ascio.get( strm, *interfss, sortsticks, false );
    if ( res )
    {
	EM::FSStoFault3DConverter fsstof3d( convsu, *interfss, *fault3d );
	res = fsstof3d.convert( true );
    }

    EM::EMM().removeObject( emobj );
    return res;
}


#undef mErrRet
#define mErrRet(msg) { if ( !msg.isEmpty() ) uiMSG().error( msg );  \
		       return false; }

bool uiImportFault::checkInpFlds()
{
    StringView fnm = infld_->fileName();
    if ( fnm.isEmpty() )
	mErrRet( tr("Please select the input file") )
    else if ( !File::exists(fnm) )
	mErrRet( tr("Input file does not exist") )

    if ( !isfss_ )
    {
	if ( typefld_->getIntValue() == 1 )
	{
	    if ( !*formatfld_->fileName() )
		mErrRet( tr("Please select the format file") )
	    else if ( !File::exists(formatfld_->fileName()) )
		mErrRet( tr("Format file does not exist") )
	}
    }

    if ( !getValidOutFld()->ioobj())
	return false;

    if ( !dataselfld_->commit() )
	return false;

    return true;
}


MultiID uiImportFault::getSelID() const
{
    return getValidOutFld()->key(false);
}


// uiImportFault3D
uiImportFault3D::uiImportFault3D( uiParent* p, const char* type )
    : uiImportFault(p,type)
{
    fd_ = EM::FaultAscIO::getDesc_( false, SI().zDomain() );
    createUI();
}


uiImportFault3D::~uiImportFault3D()
{}


bool uiImportFault3D::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    if ( typefld_ && typefld_->getIntValue()!=0 )
	return handleLMKAscii();

    return handleAscii();
}


// uiImportFaultStickSet2D
uiImportFaultStickSet2D::uiImportFaultStickSet2D( uiParent* p,
						  const char* type )
    : uiImportFault(p,type,true)
{
    fd_ = EM::FaultAscIO::getDesc_( true, SI().zDomain() );
    createUI();
}


uiImportFaultStickSet2D::~uiImportFaultStickSet2D()
{}


bool uiImportFaultStickSet2D::getFromAscIO( od_istream& strm, EM::Fault& flt )
{
    EM::FaultAscIO ascio( *fd_ );
    return ascio.get( strm, flt, false, true );
}


bool uiImportFaultStickSet2D::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() )
	return false;

    return handleAscii();
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiimpfault.h"

#include "bufstring.h"
#include "ctxtioobj.h"
#include "emfault.h"
#include "emfault3d.h"
#include "emfsstofault3d.h"
#include "emmanager.h"
#include "file.h"
#include "ioobj.h"
#include "lmkemfaulttransl.h"
#include "streamconn.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tabledef.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "uidatapointset.h"

#define mGet( tp, fss, f3d ) \
    !strcmp(tp,EMFaultStickSetTranslatorGroup::keyword()) ? fss : f3d

#define mGetCtio(tp) \
    mGet( tp, *mMkCtxtIOObj(EMFaultStickSet), *mMkCtxtIOObj(EMFault3D) )

#define mGetTitle(tp) \
    mGet( tp, "Import FaultStickSet", "Import Fault" )

#define mGetHelpID(tp) \
    mGet( tp, (is2d ? "104.1.4" : "104.1.3"), "104.1.0" )

uiImportFault::uiImportFault( uiParent* p, const char* type, bool is2d ) 
    : uiDialog(p,uiDialog::Setup(mGetTitle(type),"Specify parameters",
				 mGetHelpID(type)))
    , ctio_(mGetCtio(type))
    , isfss_(mGet(type,true,false))
    , fd_(0)
    , type_(type)
    , typefld_(0)
    , sortsticksfld_(0)
    , stickselfld_(0)
    , thresholdfld_(0)
    , is2d_(is2d)
    , importReady(this)
{
    enableSaveButton( "Display on import" );
    setModal( false );

    setCtrlStyle( DoAndStay );
    setDeleteOnClose( true );
}


const char* uiImportFault::sKeyAutoStickSel()	{ return "Auto"; }
const char* uiImportFault::sKeyInlCrlSep()	{ return "Inl/Crl separation"; }
const char* uiImportFault::sKeySlopeThres()	{ return "Slope threshold"; }
const char* uiImportFault::sKeyGeometric()	{ return "Geometric"; }
const char* uiImportFault::sKeyIndexed()	{ return "Indexed"; }
const char* uiImportFault::sKeyFileOrder()	{ return "File order"; }


void uiImportFault::createUI()
{
    infld_ = new uiFileInput( this, "Input ascii file",
		uiFileInput::Setup().withexamine(true)
		.defseldir(IOObjContext::getDataDirName(IOObjContext::Surf)) );

    if ( !isfss_ ) 
    {
	BufferStringSet types; types.add( "Plain ascii" );
//	    			    .add( "Landmark format" );
    	typefld_ = new uiGenInput( this, "Type", StringListInpSpec(types) );
	typefld_->valuechanged.notify( mCB(this,uiImportFault,typeSel) );
	typefld_->attach( alignedBelow, infld_ );

	formatfld_ = new uiFileInput( this, "Input Landmark formatfile",
				      uiFileInput::Setup(uiFileDialog::Gen)
				      .filter("*.fault_fmt") );
	formatfld_->attach( alignedBelow, typefld_ );

	BufferStringSet stickselopt; stickselopt.add( sKeyAutoStickSel() )
	    					.add( sKeyInlCrlSep() ) 
						.add( sKeySlopeThres() );
	stickselfld_ = new uiGenInput( this, "Stick selection",
				    StringListInpSpec(stickselopt) );
	stickselfld_->attach( alignedBelow, typefld_ );
	stickselfld_->valuechanged.notify(mCB(this,uiImportFault,stickSel));

	thresholdfld_ = new uiGenInput(this, "",
				DoubleInpSpec(1.0).setName("Threshold") );
	thresholdfld_->attach( rightOf, stickselfld_ );

	BufferStringSet sticksortopt; sticksortopt.add( sKeyGeometric() )
						  .add( sKeyIndexed() ) 
						  .add( sKeyFileOrder() );
	sortsticksfld_ = new uiGenInput( this, "Stick sorting",
					 StringListInpSpec(sticksortopt) );
	sortsticksfld_->attach( alignedBelow, stickselfld_ );
    }

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
	    				isfss_ ? (is2d_ ? "104.1.7" :"104.1.6")
					      : "104.1.2" );
    if ( !isfss_  )
	dataselfld_->attach( alignedBelow, sortsticksfld_ );
    else
	dataselfld_->attach( alignedBelow, infld_ );

    ctio_.ctxt.forread = false;
    BufferString labl( "Output " );
    labl += type_;
    outfld_ = new uiIOObjSel( this, ctio_, labl );
    outfld_->attach( alignedBelow, dataselfld_ );
    typeSel( 0 );
    stickSel( 0 );
}


uiImportFault::~uiImportFault()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiImportFault::typeSel( CallBacker* )
{
    if ( !typefld_ ) return;

    const int tp = typefld_->getIntValue();
    typefld_->display( typefld_->nrElements()>1 );
    formatfld_->display( tp == 1 );
    dataselfld_->display( tp == 0 );
    sortsticksfld_->display( tp == 0 );
    stickselfld_->display( tp == 0 );
    stickSel( 0 );
}


void uiImportFault::stickSel( CallBacker* )
{
    if ( !stickselfld_ ) return;

    const bool showthresfld = !strcmp( stickselfld_->text(), sKeySlopeThres() );
    const bool stickseldisplayed = stickselfld_->attachObj()->isDisplayed();
    thresholdfld_->display( stickseldisplayed && showthresfld );
}


#define mErrRet(s) { \
    uiMSG().error(s); \
    if ( fault ) fault->unRef(); \
    return false; }

EM::Fault* uiImportFault::createFault() const
{
    const char* fltnm = outfld_->getInput();
    EM::EMManager& em = EM::EMM();
    const char* typestr = isfss_ ? EM::FaultStickSet::typeStr()
				 : EM::Fault3D::typeStr();
    const EM::ObjectID emid = em.createObject( typestr, fltnm );
    mDynamicCastGet(EM::Fault*,fault,em.getObject(emid))
    return fault;
}


bool uiImportFault::handleLMKAscii()
{
    EM::Fault* fault = createFault();
    mDynamicCastGet(EM::Fault3D*,fault3d,fault)
    if ( !fault3d )
	mErrRet( "Cannot create fault" );

    fault3d->ref();

    PtrMan<lmkEMFault3DTranslator> transl =
	lmkEMFault3DTranslator::getInstance();
    StreamData sd = StreamProvider( infld_->fileName() ).makeIStream();
    Conn* conn = new StreamConn( sd.istrm );

    PtrMan<Executor> exec =
	transl->reader( *fault3d, conn, formatfld_->fileName() ); 

    if ( !exec )
	mErrRet( "Cannot import fault" );

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*exec) )
	mErrRet( taskrunner.lastMsg() );

    if ( saveButtonChecked() )
    {
	importReady.trigger();
	fault->unRefNoDelete();
    }
    else
	fault->unRef();

    return true;
}


bool uiImportFault::handleAscii()
{
    EM::Fault* fault = createFault();
    if ( !fault )
	mErrRet( "Cannot create fault" )

    fault->ref();

    StreamData sd( StreamProvider(infld_->fileName()).makeIStream() );
    if ( !sd.usable() )
	mErrRet( "Cannot open input file" )

    mDynamicCastGet(EM::Fault3D*,fault3d,fault)
    mDynamicCastGet(EM::FaultStickSet*,fss,fault)
    const char* tp = fault3d ? "fault" : "faultstickset";

    const bool res = getFromAscIO( *sd.istrm, *fault );
    if ( !res )
	mErrRet( BufferString("Cannot import ",tp) );
    PtrMan<Executor> exec = fault->saver();
    bool isexec = exec->execute();
    if ( !isexec )
	mErrRet( BufferString("Cannot save ",tp) );
    if ( saveButtonChecked() )
    {
	importReady.trigger();
	fault->unRefNoDelete();
    }
    else
	fault->unRef();

    uiMSG().message( "Import successful" );
    return false;
}


bool uiImportFault::getFromAscIO( std::istream& strm, EM::Fault& flt )
{
    EM::FaultAscIO ascio( *fd_ );
    mDynamicCastGet( EM::Fault3D*, fault3d, &flt );
    if ( !fault3d )
	return ascio.get( strm, flt, true, 0, false );

    EM::FSStoFault3DConverter::Setup convsu;
    convsu.sortsticks_ = sortsticksfld_ &&
			!strcmp( sortsticksfld_->text(), sKeyGeometric() ); 
    if ( stickselfld_ && !strcmp(stickselfld_->text(), sKeyInlCrlSep()) )
	convsu.useinlcrlslopesep_ = true;
    if ( stickselfld_ && !strcmp(stickselfld_->text(), sKeySlopeThres()) )
	convsu.stickslopethres_ = thresholdfld_->getdValue();

    EM::EMObject* emobj = EM::FaultStickSet::create( EM::EMM() );
    mDynamicCastGet( EM::FaultStickSet*, interfss, emobj );
    const bool sortsticks = sortsticksfld_ &&
			    !strcmp( sortsticksfld_->text(), sKeyIndexed() ); 
    bool res = ascio.get( strm, *interfss, sortsticks, 0, false );
    if ( res )
    {
	EM::FSStoFault3DConverter fsstof3d( convsu, *interfss, *fault3d );
	res = fsstof3d.convert();
    }

    EM::EMM().removeObject( emobj );
    return res;
}


#undef mErrRet
#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }

bool uiImportFault::checkInpFlds()
{
    if ( !*infld_->fileName() )
	mErrRet( "Please select the input file" )
    else if ( !File::exists(infld_->fileName()) )
	mErrRet( "Input file does not exist" )

    if( !isfss_ )
    {
	if ( typefld_->getIntValue() == 1 )
	{
	    if ( !*formatfld_->fileName() )
		mErrRet( "Please select the format file" )
	    else if ( !File::exists(formatfld_->fileName()) )
		mErrRet( "Format file does not exist" )
	}
    }

    if ( !outfld_->commitInput() )
	mErrRet( outfld_->isEmpty() ? "Please select the output" : 0)

    if ( !dataselfld_->commit() )
	return false;

    return true;
}


MultiID uiImportFault::getSelID() const
{
    MultiID mid = ctio_.ioobj ? ctio_.ioobj->key() : -1;
    return mid;
}


uiImportFault3D::uiImportFault3D( uiParent* p, const char* type )
    : uiImportFault(p,type)
{
    fd_ = EM::FaultAscIO::getDesc(false);
    createUI();
}


bool uiImportFault3D::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    if ( typefld_ && typefld_->getIntValue()!=0 )
	return handleLMKAscii();

    return handleAscii();
}

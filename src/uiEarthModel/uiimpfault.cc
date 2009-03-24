/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiimpfault.cc,v 1.31 2009-03-24 12:33:51 cvsbert Exp $";

#include "uiimpfault.h"

#include "bufstring.h"
#include "ctxtioobj.h"
#include "emfault.h"
#include "emfault3d.h"
#include "emmanager.h"
#include "filegen.h"
#include "ioobj.h"
#include "lmkemfaulttransl.h"
#include "streamconn.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"

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


uiImportFault::uiImportFault( uiParent* p, const char* type ) 
    : uiDialog(p,uiDialog::Setup(mGetTitle(type),"Specify parameters",
				 "104.1.0"))
    , ctio_(mGetCtio(type))
    , isfss_(mGet(type,true,false))
    , fd_(0)
    , type_(type)
    , typefld_(0)
    , sortsticksfld_(0)
{
    setCtrlStyle( DoAndStay );
}


void uiImportFault::createUI()
{
    infld_ = new uiFileInput( this, "Input ascii file",
		uiFileInput::Setup().withexamine(true)
		.defseldir(IOObjContext::getDataDirName(IOObjContext::Surf)) );

    if ( !isfss_ ) 
    {
	BufferStringSet types; types.add( "Plain ascii" )
	    			    .add( "Landmark format" );
    	typefld_ = new uiGenInput( this, "Type", StringListInpSpec(types) );
	typefld_->valuechanged.notify( mCB(this,uiImportFault,typeSel) );
	typefld_->attach( alignedBelow, infld_ );

	formatfld_ = new uiFileInput( this, "Input Landmark formatfile",
				      uiFileInput::Setup().
				      		   filter("*.fault_fmt") );
	formatfld_->setDefaultSelectionDir(
		    IOObjContext::getDataDirName(IOObjContext::Surf) );
	formatfld_->attach( alignedBelow, typefld_ );

	sortsticksfld_ = new uiGenInput( this, "Stick order",
				    BoolInpSpec(true,"sorted","as in file") );
	sortsticksfld_->attach( alignedBelow, typefld_ );
    }

    dataselfld_ = new uiTableImpDataSel( this, *fd_, "104.1.2" );
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
}


uiImportFault::~uiImportFault()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiImportFault::typeSel( CallBacker* )
{
    if ( !typefld_ ) return;

    const int tp = typefld_->getIntValue();
    dataselfld_->display( tp == 0 );
    formatfld_->display( tp == 1 );
    sortsticksfld_->display( tp == 0 );
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
    fault->unRef();
    uiMSG().message( "Import successful" );
    return false;
}


bool uiImportFault::getFromAscIO( std::istream& strm, EM::Fault& flt )
{
    EM::FaultAscIO ascio( *fd_ );
    const bool sortsticks = sortsticksfld_ ? sortsticksfld_->getBoolValue() :
					     false;
    return ascio.get( strm, flt, sortsticks, 0, false );
}


#undef mErrRet
#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiImportFault::checkInpFlds()
{
    if ( !*infld_->fileName() )
	mErrRet( "Please select the input file" )
    else if ( !File_exists(infld_->fileName()) )
	mErrRet( "Input file does not exist" )

    if( !isfss_ )
    {
	if ( typefld_->getIntValue() == 1 )
	{
	    if ( !*formatfld_->fileName() )
		mErrRet( "Please select the format file" )
	    else if ( !File_exists(formatfld_->fileName()) )
		mErrRet( "Format file does not exist" )
	}
    }

    if ( !outfld_->commitInput() )
	mErrRet( "Please select the output" )

    if ( !dataselfld_->commit() )
	return false;

    return true;
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

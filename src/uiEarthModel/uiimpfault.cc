/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.cc,v 1.23 2008-05-21 10:30:06 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiimpfault.h"

#include "ctxtioobj.h"
#include "emfault.h"
#include "emfaulttransl.h"
#include "emmanager.h"
#include "filegen.h"
#include "ioobj.h"
#include "streamconn.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"

#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitblimpexpdatasel.h"
#include "uitaskrunner.h"

#include <iostream>


uiImportFault::uiImportFault( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Faults","Specify fault parameters",
				 "104.1.2"))
    , ctio_(*mMkCtxtIOObj(EMFault))
    , fd_(*EM::FaultAscIO::getDesc())
{
    infld_ = new uiFileInput( this, "Input ascii file",
		uiFileInput::Setup().withexamine(true)
		.defseldir(IOObjContext::getDataDirName(IOObjContext::Surf)) );

    BufferStringSet types; types.add( "Plain ascii" ).add( "Landmark format" );
    typefld_ = new uiGenInput( this, "Type", StringListInpSpec(types) );
    typefld_->valuechanged.notify( mCB(this,uiImportFault,typeSel) );
    typefld_->attach( alignedBelow, infld_ );

    formatfld_ = new uiFileInput( this, "Input Landmark formatfile",
	    			  uiFileInput::Setup().filter("*.fault_fmt") );
    formatfld_->setDefaultSelectionDir(
		IOObjContext::getDataDirName(IOObjContext::Surf) );
    formatfld_->attach( alignedBelow, typefld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "100.0.0" );
    dataselfld_->attach( alignedBelow, typefld_ );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Fault" );
    outfld_->attach( alignedBelow, dataselfld_ );

    typeSel( 0 );
}


uiImportFault::~uiImportFault()
{
    delete ctio_.ioobj; delete &ctio_;
}


void uiImportFault::typeSel( CallBacker* )
{
    const int tp = typefld_->getIntValue();
    dataselfld_->display( tp == 0 );
    formatfld_->display( tp == 1 );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiImportFault::handleLMKAscii()
{
    const char* faultnm = outfld_->getInput();

    EM::EMManager& em = EM::EMM();
    const EM::ObjectID key = em.createObject( EM::Fault::typeStr(), faultnm );
    mDynamicCastGet( EM::Fault*, fault, em.getObject( key ) );
    if ( !fault )
	mErrRet( "Cannot create fault" );

    fault->ref();

    PtrMan<lmkEMFaultTranslator> transl = lmkEMFaultTranslator::getInstance();
    StreamData sd = StreamProvider( infld_->fileName() ).makeIStream();
    Conn* conn = new StreamConn( sd.istrm );

    PtrMan<Executor> exec =
	transl->reader( *fault, conn, formatfld_->fileName() ); 

    if ( !exec )
    {
	fault->unRef();
	mErrRet( "Cannot import fault" );
    }

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*exec) )
    {
	fault->unRef();
	mErrRet( taskrunner.lastMsg() );
    }

    fault->unRef();

    return true;
}


bool uiImportFault::handleAscii()
{
    return true;
}


bool uiImportFault::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    if ( typefld_->getIntValue() == 0 )
	return handleAscii();

    return handleLMKAscii();
}


bool uiImportFault::checkInpFlds()
{
    if ( !*infld_->fileName() )
	mErrRet( "Please select the input file" )
    else if ( !File_exists(infld_->fileName()) )
	mErrRet( "Input file does not exist" )

    if ( typefld_->getIntValue() == 1 )
    {
	if ( !*formatfld_->fileName() )
	    mErrRet( "Please select the format file" )
	else if ( !File_exists(formatfld_->fileName()) )
	    mErrRet( "Format file does not exist" )
    }

    if ( !outfld_->commitInput(true) )
	mErrRet( "Please select the output" )

    if ( !dataselfld_->commit() )
	mErrRet( "Please define data format" );

    return true;
}


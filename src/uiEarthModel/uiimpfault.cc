/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.cc,v 1.22 2008-05-21 06:30:38 cvsnanne Exp $
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


uiImportLMKFault::uiImportLMKFault( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Import Landmark Fault",
				     "Specify fault parameters","104.1.2"))
	, ctio(*mMkCtxtIOObj(EMFault))
	, fd_(*EM::FaultAscIO::getDesc())
{
    infld = new uiFileInput( this, "Input file" );
    infld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );

    formatfilefld = new uiFileInput( this, "Input Landmark formatfile",
	    			     uiFileInput::Setup()
				     .filter("*.fault_fmt") );
    formatfilefld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );
    formatfilefld->attach( alignedBelow, infld );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "100.0.0" );
    dataselfld_->attach( alignedBelow, formatfilefld );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Fault" );
    outfld->attach( alignedBelow, dataselfld_ );
}


uiImportLMKFault::~uiImportLMKFault()
{
    delete ctio.ioobj; delete &ctio;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiImportLMKFault::handleAscii()
{
    const char* faultnm = outfld->getInput();

    EM::EMManager& em = EM::EMM();
    const EM::ObjectID key = em.createObject( EM::Fault::typeStr(), faultnm );
    mDynamicCastGet( EM::Fault*, fault, em.getObject( key ) );
    if ( !fault )
	mErrRet( "Cannot create fault" );

    fault->ref();

    PtrMan<lmkEMFaultTranslator> transl = lmkEMFaultTranslator::getInstance();
    StreamData sd = StreamProvider( infld->fileName() ).makeIStream();
    Conn* conn = new StreamConn( sd.istrm );

    PtrMan<Executor> exec =
	transl->reader( *fault, conn, formatfilefld->fileName() ); 

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
/*
    PtrMan<Executor> saveexec = fault->geometry.saver();
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*saveexec) )
    {
	fault->unRef();
	mErrRet( savedlg.lastMsg() );
    }
    */

    fault->unRef();

    return true;
}


bool uiImportLMKFault::acceptOK( CallBacker* )
{
    bool ret = checkInpFlds() && handleAscii();
    return ret;
}


bool uiImportLMKFault::checkInpFlds()
{
    if ( !*infld->fileName() )
	mErrRet( "Please select the input file" )
    else if ( !File_exists(infld->fileName()) )
	mErrRet( "Input file does not exist" )

    if ( !*formatfilefld->fileName() )
	mErrRet( "Please select the format file" )
    else if ( !File_exists(formatfilefld->fileName()) )
	mErrRet( "Format file does not exist" )

    if ( !outfld->commitInput(true) )
	mErrRet( "Please select the output" )

    return true;
}


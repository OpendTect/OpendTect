/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimpfault.cc,v 1.19 2008-02-06 04:36:34 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiimpfault.h"

#include "emmanager.h"
#include "emfault.h"
#include "emfaulttransl.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "uiioobjsel.h"
#include "strmdata.h"
#include "strmprov.h"
#include "uitaskrunner.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "filegen.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uibinidsubsel.h"
#include "scaler.h"
#include "survinfo.h"

#include "streamconn.h"
#include "strmprov.h"
#include <iostream>


uiImportLMKFault::uiImportLMKFault( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Import Landmark Fault",
				     "Specify fault parameters","104.1.2"))
	, ctio(*mMkCtxtIOObj(EMFault))
{
    infld = new uiFileInput( this, "Input Landmark file" );
    infld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );

    formatfilefld = new uiFileInput( this, "Input Landmark formatfile",
	    			     uiFileInput::Setup()
				     .filter("*.fault_fmt") );
    formatfilefld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::Surf) );
    formatfilefld->attach( alignedBelow, infld );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Fault" );
    outfld->attach( alignedBelow, formatfilefld );
}


uiImportLMKFault::~uiImportLMKFault()
{
    delete ctio.ioobj; delete &ctio;
}


#define mWarnRet(s) { uiMSG().warning(s); return false; }
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
    if ( ! *infld->fileName() )
	mWarnRet( "Please select the input file" )
    else if ( !File_exists(infld->fileName()) )
	mWarnRet( "Input file does not exist" )

    if ( ! *formatfilefld->fileName() )
	mWarnRet( "Please select the format file" )
    else if ( !File_exists(formatfilefld->fileName()) )
	mWarnRet( "Format file does not exist" )

    if ( !outfld->commitInput( true ) )
	mWarnRet( "Please select the output" )

    return true;
}


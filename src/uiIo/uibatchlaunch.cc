/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:           $Id: uibatchlaunch.cc,v 1.57 2007-08-10 09:52:07 cvsbert Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
#include "uicombobox.h"
#include "uispinbox.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "uibutton.h"
#include "uimsg.h"
#include "iopar.h"
#include "strmdata.h"
#include "strmprov.h"
#include "hostdata.h"
#include "filepath.h"
#include "oddirs.h"
#include "ptrman.h"
#include "keystrs.h"
#include "ioman.h"

static const char* sSingBaseNm = "batch_processing";
static const char* sMultiBaseNm = "cube_processing";


static void getProcFilename( const char* basnm, const char* altbasnm,
			     BufferString& tfname )
{
    if ( !basnm || !*basnm ) basnm = altbasnm;
    tfname = basnm;
    cleanupString( tfname.buf(), NO, NO, YES );
    tfname += ".par";
    tfname = GetProcFileName( tfname );
}


static bool writeProcFile( IOPar& iop, const char* tfname )
{
    const_cast<IOPar&>(iop).set( sKey::Survey, IOM().surveyName() );
    if ( !iop.write(tfname,sKey::Pars) )
    {
	BufferString msg = "Cannot write to:\n"; msg += tfname;
	uiMSG().error( msg );
	return false;
    }

    return true;
}

#ifdef HAVE_OUTPUT_OPTIONS

uiBatchLaunch::uiBatchLaunch( uiParent* p, const IOPar& ip,
			      const char* hn, const char* pn, bool wp )
        : uiDialog(p,uiDialog::Setup("Batch launch","Specify output mode",
		   "0.1.4"))
	, iop(*new IOPar(ip))
	, hostname(hn)
	, progname(pn)
{
    finaliseDone.notify( mCB(this,uiBatchLaunch,remSel) );
    HostDataList hdl;
    rshcomm = hdl.rshComm();
    if ( rshcomm.isEmpty() ) rshcomm = "rsh";
    nicelvl = hdl.defNiceLevel();

    BufferString dispstr( "Remote (using " );
    dispstr += rshcomm; dispstr += ")";
    remfld = new uiGenInput( this, "Execute",
			     BoolInpSpec(true,"Local",dispstr) );
    remfld->valuechanged.notify( mCB(this,uiBatchLaunch,remSel) );

    opts.add( "Output window" );
    opts.add( "Log file" );
    opts.add( "Standard output" );
    if ( wp )
	opts.add( "Parameter report (no run)" );
    optfld = new uiLabeledComboBox( this, opts, "Output to" );
    optfld->attach( alignedBelow, remfld );
    optfld->box()->setCurrentItem( 0 );
    optfld->box()->selectionChanged.notify( mCB(this,uiBatchLaunch,optSel) );

    StringListInpSpec spec;
    for ( int idx=0; idx<hdl.size(); idx++ )
	spec.addString( hdl[idx]->name() );
    remhostfld = new uiGenInput( this, "Hostname", spec );
    remhostfld->attach( alignedBelow, remfld );

    static BufferString fname = "";
    if ( fname.isEmpty() )
    {
	fname = GetProcFileName( "log" );
	if ( GetSoftwareUser() )
	    { fname += "_"; fname += GetSoftwareUser(); }
	fname += ".txt";
    }
    filefld = new uiFileInput( this, "Log file",
	   		       uiFileInput::Setup(fname)
				.forread(false)
	   			.filter("*.log;;*.txt") );
    filefld->attach( alignedBelow, optfld );

    nicefld = new uiLabeledSpinBox( this, "Nice level" );
    nicefld->attach( alignedBelow, filefld );
    nicefld->box()->setInterval( 0, 19 );
    nicefld->box()->setValue( nicelvl );
}


uiBatchLaunch::~uiBatchLaunch()
{
    delete &iop;
}


bool uiBatchLaunch::execRemote() const
{
    return !remfld->getBoolValue();
}


void uiBatchLaunch::optSel( CallBacker* )
{
    const int sel = selected();
    filefld->display( sel == 1 || sel == 3 );
}


void uiBatchLaunch::remSel( CallBacker* )
{
    bool isrem = execRemote();
    remhostfld->display( isrem );
    optfld->display( !isrem );
    optSel(0);
}


void uiBatchLaunch::setParFileName( const char* fnm )
{
    parfname = fnm;
    FilePath fp( fnm );
    fp.setExtension( "log", true );
    filefld->setFileName( fp.fullPath() );
}


int uiBatchLaunch::selected()
{
    return execRemote() ? 1 : optfld->box()->currentItem();
}


bool uiBatchLaunch::acceptOK( CallBacker* )
{
    const bool dormt = execRemote();
    if ( dormt )
    {
	hostname = remhostfld->text();
	if ( hostname.isEmpty() )
	{
	    uiMSG().error( "Please specify the name of the remote host" );
	    return false;
	}
    }

    const int sel = selected();
    BufferString fname = sel == 0 ? "window"
		       : (sel == 2 ? "stdout" : filefld->fileName());
    if ( fname.isEmpty() ) fname = "/dev/null";
    iop.set( sKey::LogFile, fname );
    iop.set( sKey::Survey, IOM().surveyName() );

    if ( selected() == 3 )
    {
	iop.set( sKey::LogFile, "stdout" );
	if ( !iop.write(fname,sKey::Pars) )
	{
	    uiMSG().error( "Cannot write parameter file" );
            return false;
	}
	return true;
    }

    if ( parfname.isEmpty() )
	getProcFilename( sSingBaseNm, sSingBaseNm, parfname );
    if ( !writeProcFile(iop,parfname) )
	return false;

    BufferString comm( "@" );
    comm += GetExecScript( dormt );
    if ( dormt )
    {
	comm += hostname;
	comm += " --rexec ";
	comm += rshcomm;
    }

    const bool inbg=dormt;
#ifdef __win__ 

    comm += " --inbg "; comm += progname;
    FilePath parfp( parfname );

    BufferString _parfnm( parfp.fullPath(FilePath::Unix) );
    replaceCharacter(_parfnm.buf(),' ','%');
    comm += " \""; comm += _parfnm; comm += "\"";

#else

    nicelvl = nicefld->box()->getValue();
    if ( nicelvl != 0 )
	{ comm += " --nice "; comm += nicelvl; }
    comm += " "; comm += progname;
    comm += " -bg "; comm += parfname;

#endif

    if ( !StreamProvider( comm ).executeCommand(inbg) )
    {
	uiMSG().error( "Cannot start batch program" );
	return false;
    }
    return true;
}

#endif // HAVE_OUTPUT_OPTIONS


uiFullBatchDialog::uiFullBatchDialog( uiParent* p, const Setup& s )
	: uiDialog(p,uiDialog::Setup(s.wintxt_,"X",0).oktext("Proceed")
						     .modal(s.modal_))
    	, uppgrp(new uiGroup(this,"Upper group"))
	, procprognm(s.procprognm_.isEmpty() ? "process_attrib" : s.procprognm_)
	, multiprognm(s.multiprocprognm_.isEmpty() ? "SeisMMBatch"
						   : s.multiprocprognm_)
    	, redo_(false)
	, parfnamefld(0)
{
    setParFileNmDef( 0 );
}


void uiFullBatchDialog::addStdFields( bool forread )
{
    uiGroup* dogrp = new uiGroup( this, "Proc details" );
    if ( !redo_ )
    {
	uiSeparator* sep = new uiSeparator( this, "Hor sep" );
	sep->attach( stretchedBelow, uppgrp );
	dogrp->attach( alignedBelow, uppgrp );
	dogrp->attach( ensureBelow, sep );
    }

    singmachfld = new uiGenInput( dogrp, "Submit to",
		BoolInpSpec(true,"Single machine","Multiple machines") );
    singmachfld->valuechanged.notify( mCB(this,uiFullBatchDialog,singTogg) );
    const char* txt = redo_ ? "Processing specification file"
			    : "Store processing specification as";
    parfnamefld = new uiFileInput( dogrp, txt, uiFileInput::Setup(singparfname)
					       .forread(forread)
					       .filter("*.par;;*") );
    parfnamefld->attach( alignedBelow, singmachfld );

    dogrp->setHAlignObj( singmachfld );
}


void uiFullBatchDialog::setParFileNmDef( const char* nm )
{
    getProcFilename( nm, sSingBaseNm, singparfname );
    getProcFilename( nm, sMultiBaseNm, multiparfname );
    if ( parfnamefld )
	parfnamefld->setFileName( singmachfld->getBoolValue() ? singparfname
							      : multiparfname );
}

void uiFullBatchDialog::singTogg( CallBacker* cb )
{
    const BufferString inpfnm = parfnamefld->fileName();
    const bool issing = singmachfld->getBoolValue();
    if ( issing && inpfnm == multiparfname )
	parfnamefld->setFileName( singparfname );
    else if ( !issing && inpfnm == singparfname )
	parfnamefld->setFileName( multiparfname );
}


bool uiFullBatchDialog::acceptOK( CallBacker* cb )
{
    if ( !prepareProcessing() ) return false;
    BufferString inpfnm = parfnamefld->fileName();
    if ( inpfnm.isEmpty() )
	getProcFilename( 0, "tmp_proc", inpfnm );
    else if ( !FilePath(inpfnm).isAbsolute() )
	getProcFilename( inpfnm, sSingBaseNm, inpfnm );

    const bool issing = singmachfld->getBoolValue();

    PtrMan<IOPar> iop = 0;
    if ( redo_ )
    {
	if ( issing )
	{
	    StreamData sd = StreamProvider( inpfnm ).makeIStream();
	    if ( !sd.usable() )
		{ uiMSG().error( "Cannot open parameter file" ); return false; }
	    iop = new IOPar; iop->read( *sd.istrm, sKey::Pars );
	}
    }
    else
    {
	iop = new IOPar( "Processing" );
	if ( !fillPar(*iop) )
	    return false;
    }

    if ( !issing && !redo_ && !writeProcFile(*iop,inpfnm) )
	return false;

    return issing ? singLaunch( *iop, inpfnm ) : multiLaunch( inpfnm );
}


bool uiFullBatchDialog::singLaunch( const IOPar& iop, const char* fnm )
{
#ifdef HAVE_OUTPUT_OPTIONS
    uiBatchLaunch dlg( this, iop, 0, procprognm, false );
    dlg.setParFileName( fnm );
    return dlg.go();
#else

    BufferString fname = "stdout";

    IOPar& workiop( const_cast<IOPar&>( iop ) );
    workiop.set( "Log file", fname );

    FilePath parfp( fnm );
    if ( !parfp.nrLevels() )
        parfp = singparfname;
    if ( !writeProcFile(workiop,parfp.fullPath()) )
	return false;

    const bool dormt = false;
    BufferString comm( "@" );
    comm += GetExecScript( dormt );

#ifdef __win__ 
    comm += " --inxterm+askclose "; comm += procprognm;

    BufferString _parfnm( parfp.fullPath(FilePath::Unix) );
    replaceCharacter(_parfnm.buf(),' ','%');

    comm += " \""; comm += _parfnm; comm += "\"";

#else
    comm += " "; comm += procprognm;
    comm += " -bg "; comm += parfp.fullPath();
#endif

    const bool inbg=dormt;
    if ( !StreamProvider( comm ).executeCommand(inbg) )
    {
	uiMSG().error( "Cannot start batch program" );
	return false;
    }
    return true;

#endif

}


bool uiFullBatchDialog::multiLaunch( const char* fnm )
{
    BufferString comm( multiprognm );	comm += " ";
    comm += procprognm;			comm += " \"";
    comm += fnm; 
    comm += "\"";

    if ( !ExecOSCmd( comm, true ) )
	{ uiMSG().error( "Cannot start multi-machine program" ); return false;}

    return true;
}


uiRestartBatchDialog::uiRestartBatchDialog( uiParent* p, const char* ppn,
       					    const char* mpn )
    	: uiFullBatchDialog(p,Setup("(Re-)Start processing")
				.procprognm(ppn).multiprocprognm(mpn))
{
    redo_ = true;
    setHelpID( "101.2.1" );
    setTitleText( "Run a saved processing job" );
    setCtrlStyle( DoAndLeave );
    addStdFields( true );
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uibatchlaunch.h"

#include "uibutton.h"
#include "uiclusterjobprov.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uispinbox.h"

#include "envvars.h"
#include "file.h"
#include "filepath.h"
#include "hostdata.h"
#include "ioman.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "ptrman.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "ascstream.h"

static const char* sSingBaseNm = "batch_processing";
static const char* sMultiBaseNm = "cube_processing";


static void getProcFilename( const char* basnm, const char* altbasnm,
			     BufferString& tfname )
{
    if ( !basnm || !*basnm ) basnm = altbasnm;
    tfname = basnm;
    cleanupString( tfname.buf(), false, false, true );
    tfname += ".par";
    tfname = GetProcFileName( tfname );
}


static bool writeProcFile( IOPar& iop, const char* tfname )
{
    const_cast<IOPar&>(iop).set( sKey::DataRoot(), GetBaseDataDir() );
    const_cast<IOPar&>(iop).set( sKey::Survey(), IOM().surveyName() );
    if ( !iop.write(tfname,sKey::Pars()) )
    {
	BufferString msg = "Cannot write to:\n"; msg += tfname;
	uiMSG().error( msg );
	return false;
    }

    return true;
}


// uiBatchLaunch
uiBatchLaunch::uiBatchLaunch( uiParent* p, const IOPar& ip,
			      const char* hn, const char* pn, bool wp )
        : uiDialog(p,uiDialog::Setup("Batch launch","Specify output mode",
		   "0.1.4"))
	, iop_(*new IOPar(ip))
	, hostname_(hn)
	, progname_(pn)
{
    postFinalise().notify( mCB(this,uiBatchLaunch,remSel) );
    HostDataList hdl;
    rshcomm_ = hdl.rshComm();
    if ( rshcomm_.isEmpty() ) rshcomm_ = "rsh";
    nicelvl_ = hdl.defNiceLevel();

    BufferString dispstr( "Remote (using " );
    dispstr += rshcomm_; dispstr += ")";
    remfld_ = new uiGenInput( this, "Execute",
			     BoolInpSpec(true,"Local",dispstr) );
    remfld_->valuechanged.notify( mCB(this,uiBatchLaunch,remSel) );

    opts_.add( "Output window" ).add( "Log file" ).add( "Standard output" );
    if ( wp )
	opts_.add( "Parameter report (no run)" );
    optfld_ = new uiLabeledComboBox( this, opts_, "Output to" );
    optfld_->attach( alignedBelow, remfld_ );
    optfld_->box()->setCurrentItem( 0 );
    optfld_->box()->selectionChanged.notify( mCB(this,uiBatchLaunch,optSel) );

    BufferStringSet hostnames;
    for ( int idx=0; idx<hdl.size(); idx++ )
	hostnames.add( hdl[idx]->name() );
    const char* localhost = hostnames.size() ? hostnames[0]->buf() : 0;
    hostnames.sort();
    const int localhostidx = localhost ? hostnames.indexOf( localhost ) : 0;
    StringListInpSpec spec( hostnames );
    remhostfld_ = new uiGenInput( this, "Hostname", spec );
    remhostfld_->attach( alignedBelow, remfld_ );
    remhostfld_->setValue( localhostidx );

    static BufferString fname = "";
    if ( fname.isEmpty() )
    {
	fname = GetProcFileName( "log" );
	if ( GetSoftwareUser() )
	    { fname += "_"; fname += GetSoftwareUser(); }
	fname += ".txt";
    }
    filefld_ = new uiFileInput( this, "Log file",
	   		       uiFileInput::Setup(uiFileDialog::Gen,fname)
				.forread(false)
	   			.filter("*.log;;*.txt") );
    filefld_->attach( alignedBelow, optfld_ );

#ifndef __msvc__
    nicefld_ = new uiLabeledSpinBox( this, "Nice level" );
    nicefld_->attach( alignedBelow, filefld_ );
    nicefld_->box()->setInterval( 0, 19 );
    nicefld_->box()->setValue( nicelvl_ );
#endif
}


uiBatchLaunch::~uiBatchLaunch()
{
    delete &iop_;
}


bool uiBatchLaunch::execRemote() const
{
    return !remfld_->getBoolValue();
}


void uiBatchLaunch::optSel( CallBacker* )
{
    const int sel = selected();
    filefld_->display( sel == 1 || sel == 3 );
}


void uiBatchLaunch::remSel( CallBacker* )
{
    bool isrem = execRemote();
    remhostfld_->display( isrem );
    optfld_->display( !isrem );
    optSel(0);
}


void uiBatchLaunch::setParFileName( const char* fnm )
{
    parfname_ = fnm;
    FilePath fp( fnm );
    fp.setExtension( "log", true );
    filefld_->setFileName( fp.fullPath() );
}


int uiBatchLaunch::selected()
{
    return execRemote() ? 1 : optfld_->box()->currentItem();
}


bool uiBatchLaunch::acceptOK( CallBacker* )
{
    const bool dormt = execRemote();
    if ( dormt )
    {
	hostname_ = remhostfld_->text();
	if ( hostname_.isEmpty() )
	{
	    uiMSG().error( "Please specify the name of the remote host" );
	    return false;
	}
    }

    const int sel = selected();
    BufferString fname = sel == 0 ? "window"
		       : (sel == 2 ? "stdout" : filefld_->fileName());
    if ( fname.isEmpty() ) fname = "/dev/null";
    iop_.set( sKey::LogFile(), fname );
    iop_.set( sKey::Survey(), IOM().surveyName() );

    if ( selected() == 3 )
    {
	iop_.set( sKey::LogFile(), "stdout" );
	if ( !iop_.write(fname,sKey::Pars()) )
	{
	    uiMSG().error( "Cannot write parameter file" );
            return false;
	}
	return true;
    }

    if ( parfname_.isEmpty() )
	getProcFilename( sSingBaseNm, sSingBaseNm, parfname_ );
    if ( !writeProcFile(iop_,parfname_) )
	return false;

    BufferString comm( "@" );
#ifdef __msvc__
    bool inbg = true;
    if ( dormt )
    {
	HostDataList hdl;
	const HostData* hd = hdl.find( hostname_.buf() );
	FilePath remfp = hd->prefixFilePath( HostData::Data );
	FilePath temppath( parfname_ );
	iop_.set( sKey::DataRoot(), remfp.fullPath() );
	remfp.add(  GetSurveyName() ).add( "Proc" )
	     .add( temppath.fileName() );
	FilePath logfp( remfp );
	logfp.setExtension( ".log", true );
	iop_.set( sKey::LogFile(), logfp.fullPath() );
	if ( !iop_.write(parfname_,sKey::Pars()) )
	{
	    uiMSG().error( "Cannot write parameter file" );
            return false;
	}
	parfname_ = remfp.fullPath();
	comm.add( "od_remexec " ).add( hostname_ )
	    .add( " " ).add( progname_ ).add( " " );
    }
    else
	comm += FilePath(GetBinPlfDir()).add(progname_).fullPath();
#else
    bool inbg = dormt;
    comm += GetExecScript( dormt );
    if ( dormt )
    {
	comm += hostname_;
	comm += " --rexec ";
	comm += rshcomm_;
    }

    nicelvl_ = nicefld_->box()->getValue();
    if ( nicelvl_ != 0 )
	comm.add( " --nice " ).add( nicelvl_ );

    comm.add( " --inbg " ).add( progname_ );
#endif
    comm.add( " \"" ).add( parfname_ ).add( "\"" );

    if ( !StreamProvider( comm ).executeCommand(inbg,sel==2) )
    {
	uiMSG().error( "Cannot start batch program" );
	return false;
    }

    return true;
}


// uiFullBatchDialog
uiFullBatchDialog::uiFullBatchDialog( uiParent* p, const Setup& s )
    : uiDialog(p,uiDialog::Setup(s.wintxt_,"",mNoHelpID)
		 .modal(s.modal_).menubar(s.menubar_))
    , uppgrp_(new uiGroup(this,"Upper group"))
    , procprognm_(s.procprognm_.isEmpty() ? "od_process_attrib" : s.procprognm_)
    , multiprognm_(s.multiprocprognm_.isEmpty() ? "od_SeisMMBatch"
						: s.multiprocprognm_)
    , redo_(false)
    , parfnamefld_(0)
    , singmachfld_( 0 )
    , hascluster_(false)
{
    setCtrlStyle( DoAndProceed );
    setParFileNmDef( 0 );
    if ( s.showoutputopts_ )
    {
	enableSaveButton( "Show options" );
	setSaveButtonChecked( false );
    }
}


void uiFullBatchDialog::addStdFields( bool forread, bool onlysinglemachine,
       				      bool clusterproc )
{
    uiGroup* dogrp = new uiGroup( this, "Proc details" );
    if ( !redo_ )
    {
	uiSeparator* sep = new uiSeparator( this, "Hor sep" );
	sep->attach( stretchedBelow, uppgrp_ );
	dogrp->attach( alignedBelow, uppgrp_ );
	dogrp->attach( ensureBelow, sep );
    }

#ifndef __win__ 
    hascluster_ = !onlysinglemachine && clusterproc
				    && GetEnvVarYN("DTECT_CLUSTER_PROC");
#endif

    if ( !onlysinglemachine )
    {
	BufferStringSet opts;
	opts.add("Single machine").add("Multiple machines").add("Cluster");
	if ( hascluster_ )
	    singmachfld_ = new uiGenInput( dogrp, "Submit to",
		    			   StringListInpSpec(opts) );
	else
	    singmachfld_ = new uiGenInput( dogrp, "Submit to",
		    		BoolInpSpec(true,opts.get(0),opts.get(1)) );

	singmachfld_->valuechanged.notify( 
		mCB(this,uiFullBatchDialog,singTogg) );
    }
    const char* txt = redo_ ? "Processing specification file"
			    : "Store processing specification as";
    parfnamefld_ = new uiFileInput( dogrp,txt, uiFileInput::Setup(singparfname_)
					       .forread(forread)
					       .filedlgtype(uiFileDialog::Gen)
					       .filter("*.par;;*")
					       .confirmoverwrite(false) );
    if ( !onlysinglemachine )
	parfnamefld_->attach( alignedBelow, singmachfld_ );

    dogrp->setHAlignObj( parfnamefld_ );
}


void uiFullBatchDialog::setMode( Mode md )
{
    if ( !singmachfld_ )
	return;

    if ( md == Single )
	singmachfld_->setValue( hascluster_ ? 0 : 1 );
    else if ( md == Multi )
	singmachfld_->setValue( hascluster_ ? 1 : 0 );
    else
	singmachfld_->setValue( hascluster_ ? 2 : 0 );

    singTogg( 0 );
}


bool uiFullBatchDialog::isSingleMachine() const
{
    if ( !singmachfld_ )
	return true;

    return hascluster_ ?
	singmachfld_->getIntValue()==0 : singmachfld_->getBoolValue();
}


void uiFullBatchDialog::setParFileNmDef( const char* nm )
{
    getProcFilename( nm, sSingBaseNm, singparfname_ );
    getProcFilename( nm, sMultiBaseNm, multiparfname_ );
    if ( parfnamefld_ )
	parfnamefld_->setFileName( isSingleMachine()
		? singparfname_	: multiparfname_ );
}


void uiFullBatchDialog::singTogg( CallBacker* cb )
{
    const BufferString inpfnm = parfnamefld_->fileName();
    const bool issing = isSingleMachine();
    if ( issing && inpfnm == multiparfname_ )
	parfnamefld_->setFileName( singparfname_ );
    else if ( !issing && inpfnm == singparfname_ )
	parfnamefld_->setFileName( multiparfname_ );

    parfnamefld_->display( !hascluster_ || singmachfld_->getIntValue()<2 );
    uiButton* optionsbut = button( uiDialog::SAVE );
    if ( optionsbut ) optionsbut->display( issing );
}


bool uiFullBatchDialog::acceptOK( CallBacker* cb )
{
    if ( !prepareProcessing() ) return false;
    PtrMan<IOPar> iop = 0;
    if ( hascluster_ && singmachfld_->getIntValue()==2 )
    {
	iop = new IOPar( "Processing" );
	if ( !fillPar(*iop) )
	    return false;

	uiClusterJobProv dlg( this, *iop, procprognm_.buf(),
			      multiparfname_.buf() );
	return dlg.go();
    }

    const bool issing = isSingleMachine();
    BufferString fnm = parfnamefld_ ? parfnamefld_->fileName()
				    : ( issing ? singparfname_.buf()
					       : multiparfname_.buf() );
    if ( fnm.isEmpty() )
	getProcFilename( 0, "tmp_proc", fnm );
    else if ( !FilePath(fnm).isAbsolute() )
	getProcFilename( fnm, sSingBaseNm, fnm );

    if ( redo_ )
    {
	if ( File::isEmpty(fnm) )
	{
	    uiMSG().error("Invalid (empty or not readable) specification file");
	    return false;
	}
    }
    else if ( File::exists(fnm) )
    {
	if ( !File::isWritable(fnm) )
	{
	    BufferString msg( "Cannot write specifications to\n", fnm.buf(),
		"\nPlease select another file, or make sure file is writable.");
	    uiMSG().error( msg );
	    return false;
	}
    }

    if ( redo_ )
    {
	if ( issing )
	{
	    StreamData sd = StreamProvider( fnm ).makeIStream();
	    if ( !sd.usable() )
		{ uiMSG().error( "Cannot open parameter file" ); return false; }
	    ascistream aistrm( *sd.istrm, true );
	    if ( aistrm.fileType()!=sKey::Pars() )
	    {
		sd.close();
		uiMSG().error(BufferString(fnm," is not a parameter file"));
		return false;
	    }
	    const float ver = toFloat( aistrm.version() );
	    if ( ver < 3.1999 )
	    {
		sd.close();
		uiMSG().error( "Invalid parameter file: pre-3.2 version");
		return false;
	    }
	    iop = new IOPar; iop->getFrom( aistrm );
	    sd.close();
	}
    }
    else
    {
	iop = new IOPar( "Processing" );
	if ( !fillPar(*iop) )
	    return false;
    }

    if ( !issing && !redo_ && !writeProcFile(*iop,fnm) )
	return false;

    bool res = issing ? singLaunch( *iop, fnm ) : multiLaunch( fnm );
    return ctrlstyle_ == DoAndStay ? false : res; 
}


bool uiFullBatchDialog::singLaunch( const IOPar& iop, const char* fnm )
{
    if ( saveButtonChecked() )
    {
	uiBatchLaunch dlg( this, iop, 0, procprognm_, false );
	dlg.setParFileName( fnm );
	return dlg.go();
    }

    BufferString fname = "window";

    IOPar& workiop( const_cast<IOPar&>( iop ) );
    workiop.set( "Log file", fname );

    FilePath parfp( fnm );
    if ( !parfp.nrLevels() )
        parfp = singparfname_;
    if ( !writeProcFile(workiop,parfp.fullPath()) )
	return false;

    bool dormt = false;

#ifndef __msvc__
    BufferString comm( "@" );
    comm += GetExecScript( dormt );

# ifdef __win__ 
    comm += " --inxterm+askclose "; comm += procprognm_;

    BufferString _parfnm( parfp.fullPath(FilePath::Unix) );
    replaceCharacter(_parfnm.buf(),' ','%');

    comm += " \""; comm += _parfnm; comm += "\"";

# else
    comm += " "; comm += procprognm_;
    comm += " -bg "; comm += parfp.fullPath();
# endif

#else
    BufferString comm = FilePath(GetBinPlfDir()).add(procprognm_).fullPath();
    BufferString _parfnm( parfp.fullPath(FilePath::Windows) );
    comm += " \""; comm += _parfnm; comm += "\"";
    dormt = true;
#endif

    const bool inbg=dormt;
    if ( !StreamProvider( comm ).executeCommand(inbg) )
    {
	uiMSG().error( "Cannot start batch program" );
	return false;
    }
    return true;
}


bool uiFullBatchDialog::multiLaunch( const char* fnm )
{
    BufferString comm;
#ifndef __msvc__
    comm.add( GetExecScript(false) ).add( " " );
#endif

    comm.add( multiprognm_ ).add( " " ).add( procprognm_ )
	.add( " \"" ).add( fnm ).add( "\"" );

#ifdef __msvc__ 
    if ( !ExecOSCmd( comm, false ) )
#else
    if ( !ExecOSCmd( comm, true ) )
#endif
	{ uiMSG().error( "Cannot start multi-machine program" ); return false; }

    return true;
}


// uiRestartBatchDialog
uiRestartBatchDialog::uiRestartBatchDialog( uiParent* p, const char* ppn,
       					    const char* mpn )
    	: uiFullBatchDialog(p,Setup("(Re-)Start processing")
				.procprognm(ppn).multiprocprognm(mpn))
{
    redo_ = true;
    setHelpID( "101.2.1" );
    setTitleText( "Run a saved processing job" );
    setCtrlStyle( DoAndProceed );
    addStdFields( true );
}

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:           $Id: uibatchlaunch.cc,v 1.22 2003-04-22 09:51:30 arend Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"
#include "uidistriblaunch.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uiseparator.h"
#include "uibutton.h"
#include "uimsg.h"
#include "ioparlist.h"
#include "strmdata.h"
#include "strmprov.h"
#include "lic.h"
#include "hostdata.h"
#include "filegen.h"

static const char* sSingBaseNm = "batch_processing";
static const char* sMultiBaseNm = "cube_processing";


uiGenBatchLaunch::uiGenBatchLaunch( uiParent* p, const UserIDSet& nms )
        : uiDialog(p,uiDialog::Setup("Run batch program",
		   "Specify batch parameters","0.1.5"))
        , prognms(nms)
{
    progfld = new uiLabeledComboBox( this, prognms );

    BufferString dir = File_getFullPath( GetDataDir(), "Proc" );
    parfld = new uiFileInput( this, "Parameter file", dir, true );
    parfld->attach( alignedBelow, progfld );
}


bool uiGenBatchLaunch::acceptOK( CallBacker* )
{
    const char* prognm = progfld->box()->text();
    if ( ! *prognm )
        { uiMSG().error("Please specify the batch program"); return NO; }

    const char* fname = parfld->fileName();
    StreamData sd = StreamProvider( fname ).makeIStream();
    if ( !sd.usable() )
    { 
	sd.close();
	uiMSG().error( "Could not open input file" );
        return false; 
    }
	
    parlist = new IOParList( *sd.istrm );
    sd.close();
    if ( parlist->size() < 1 )
    {
        uiMSG().error( "The parameter file is invalid" );
        delete parlist; parlist = 0;
        return false;
    }

    parlist->setFileName( fname );
    return true;
}

const char* uiGenBatchLaunch::getProg()
{
    return progfld->box()->text();
}


static void getProcFilename( const char* basnm, BufferString& tfname )
{
    const BufferString inpfnm( basnm );
    tfname = File_getFullPath( GetDataDir(), "Proc" );
    tfname = File_getFullPath( tfname, inpfnm );
    if ( GetSoftwareUser() )
	{ tfname += "_"; tfname += GetSoftwareUser(); }
    tfname += ".par";
}


static bool writeProcFile( const IOParList& iopl, const char* tfname )
{
    StreamData sd = StreamProvider(tfname).makeOStream();
    bool allok = sd.usable() && iopl.write(*sd.ostrm);
    sd.close();
    if ( !allok )
    {
	BufferString msg = "Cannot write to:\n"; msg += tfname;
	uiMSG().error( msg );
	return false;
    }

    return true;
}


uiBatchLaunch::uiBatchLaunch( uiParent* p, const IOParList& pl,
			      const char* hn, const char* pn, bool wp )
        : uiDialog(p,uiDialog::Setup("Batch launch","Specify output mode",
		   "0.1.4"))
	, iopl(pl)
	, hostname(hn)
	, progname(pn)
	, opts("Output to")
	, licfeat((int)Licenser::UdfFeat)
{
    finaliseDone.notify( mCB(this,uiBatchLaunch,remSel) );
    HostDataList hdl;
    rshcomm = hdl.rshComm();
    if ( rshcomm == "" ) rshcomm = "rsh";
    nicelvl = hdl.defNiceLevel();

    BufferString dispstr( "Remote (using " );
    dispstr += rshcomm; dispstr += ")";
    remfld = new uiGenInput( this, "Execute",
			     BoolInpSpec( "Local", dispstr ) );
    remfld->valuechanged.notify( mCB(this,uiBatchLaunch,remSel) );

    opts.add( "Output window" );
    opts.add( "Log file" );
    opts.add( "Standard output" );
    if ( wp )
	opts.add( "Parameter report (no run)" );
    opts.setCurrent( 0 );
    optfld = new uiLabeledComboBox( this, Ptr(opts) );
    optfld->attach( alignedBelow, remfld );
    optfld->box()->selectionChanged.notify( mCB(this,uiBatchLaunch,optSel) );

    StringListInpSpec spec;
    for ( int idx=0; idx<hdl.size(); idx++ )
	spec.addString( hdl[idx]->name() );
    remhostfld = new uiGenInput( this, "Hostname", spec );
    remhostfld->attach( alignedBelow, remfld );

    static BufferString fname = "";
    if ( fname == "" )
    {
	fname = GetDataDir();
	fname = File_getFullPath( fname, "Proc" );
	fname = File_getFullPath( fname, "log" );
	if ( GetSoftwareUser() )
	    { fname += "_"; fname += GetSoftwareUser(); }
	fname += ".txt";
    }
    filefld = new uiFileInput( this, "Log file", fname, false );
    filefld->attach( alignedBelow, optfld );
}


uiBatchLaunch::~uiBatchLaunch()
{
    opts.deepErase();
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


int uiBatchLaunch::selected()
{
    return execRemote() ? 1 : optfld->box()->currentItem();
}


bool uiBatchLaunch::acceptOK( CallBacker* )
{
    const bool dormt = execRemote();
    if ( execRemote() )
    {
	hostname = remhostfld->text();
	if ( hostname == "" )
	{
	    uiMSG().error( "Please specify the name of the remote host" );
	    return false;
	}
    }

    const int sel = selected();
    BufferString fname = sel == 0 ? "window"
		       : (sel == 2 ? "stdout" : filefld->fileName());
    if ( fname == "" ) fname = "/dev/null";
    IOPar* iop = const_cast<IOPar*>(iopl.size() ? iopl[0] : 0);
    if ( iop )
    {
	iop->set( "Log file", fname );
	if ( licfeat != (int)Licenser::UdfFeat
	  && LM().check((Licenser::Feat)licfeat) )
	    LM().setCert( *iop );
    }

    if ( selected() == 3 )
    {
	if ( iop ) iop->set( "Log file", "stdout" );
        StreamData sd = StreamProvider( fname ).makeOStream();
	if ( !sd.usable() )
        {
	    uiMSG().error( "Cannot open output stream" );
	    sd.close();
	    return false;
	}
	else if ( !iopl.write(*sd.ostrm) )
	{
	    uiMSG().error( "Error during write" );
	    sd.close();
            return false;
	}
	sd.close();
	return true;
    }

    if ( parfname == "" )
	getProcFilename( sSingBaseNm, parfname );
    if ( !writeProcFile(iopl,parfname) )
	return false;

    BufferString comm( "@" );
    comm += GetSoftwareDir();
    comm = File_getFullPath( comm, "bin" );
    comm = File_getFullPath( comm, "dgb_exec" );
    if ( dormt )
    {
	comm += "_rmt ";
	comm += hostname;
	comm += " --rexec ";
	comm += rshcomm;
    }
    if ( nicelvl != 0 )
	{ comm += " --nice "; comm += nicelvl; }
    comm += " "; comm += progname;
    comm += " -bg "; comm += parfname;


    if ( !StreamProvider( comm ).executeCommand(dormt) )
    {
	uiMSG().error( "Cannot start batch program" );
	return false;
    }
    return true;
}


uiFullBatchDialog::uiFullBatchDialog( uiParent* p, const char* t,
					const char* ppn, const char* mpn )
	: uiDialog(p,Setup(t,"X",0).oktext("Proceed"))
    	, uppgrp(new uiGroup(this,"Upper group"))
	, procprognm(ppn?ppn:"process_attrib")
	, multiprognm(mpn?mpn:"SeisMMBatch")
    	, redo_(false)
	, licfeat((int)Licenser::UdfFeat)
{
    getProcFilename( sSingBaseNm, singparfname );
    getProcFilename( sMultiBaseNm, multiparfname );
}


void uiFullBatchDialog::addStdFields()
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
	    		BoolInpSpec("Single machine","Multiple machines") );
    singmachfld->valuechanged.notify( mCB(this,uiFullBatchDialog,singTogg) );
    const char* txt = redo_ ? "Processing specification file"
			    : "Store processing specification as";
    parfnamefld = new uiFileInput( dogrp, txt, singparfname, false, "*.par;;*");
    parfnamefld->attach( alignedBelow, singmachfld );

    dogrp->setHAlignObj( singmachfld );
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
    if ( inpfnm == "" )
	getProcFilename( "tmp_proc", inpfnm );
    else if ( !File_isAbsPath(inpfnm) )
	getProcFilename( inpfnm, inpfnm );

    const bool issing = singmachfld->getBoolValue();
    IOParList* iopl;
    if ( redo_ )
    {
	if ( issing )
	{
	    StreamData sd = StreamProvider( inpfnm ).makeIStream();
	    if ( !sd.usable() )
		{ uiMSG().error( "Cannot open parameter file" ); return false; }
	    iopl = new IOParList( *sd.istrm );
	    iopl->setFileName( inpfnm );
	}
    }
    else
    {
	iopl = new IOParList; iopl->deepErase();
	IOPar* iopar = new IOPar( "Processing" );
	*iopl += iopar;
	if ( !fillPar(*iopar) )
	    return false;
    }

    if ( !issing && !redo_ && !writeProcFile(*iopl,inpfnm) )
	return false;

    return issing ? singLaunch( *iopl, inpfnm ) : multiLaunch( inpfnm );
}


bool uiFullBatchDialog::singLaunch( const IOParList& iopl, const char* fnm )
{
    uiBatchLaunch dlg( this, iopl, 0, procprognm, false );
    dlg.setParFileName( fnm );
    dlg.setLicFeat( licfeat );
    return dlg.go();
}


bool uiFullBatchDialog::multiLaunch( const char* fnm )
{
    BufferString comm( "@" );
    comm += multiprognm;	comm += " ";
    comm += procprognm;		comm += " ";
    comm += fnm;		comm += "&";
    StreamData sd = StreamProvider( comm ).makeOStream();
    if ( !sd.usable() )
	{ uiMSG().error( "Cannot start multi-machine program" ); return false; }

    sd.close();
    return true;
}


uiRestartBatchDialog::uiRestartBatchDialog( uiParent* p, const char* ppn,
       					    const char* mpn )
    	: uiFullBatchDialog(p,"(Re-)Start processing",ppn,mpn)
{
    redo_ = true;
    setHelpID( "101.2.2" );
    setTitleText( "Run a saved processing job" );
    setOkText( "Ok" );
    addStdFields();
}

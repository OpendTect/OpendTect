/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          January 2002
 RCS:           $Id: uibatchlaunch.cc,v 1.2 2002-01-07 14:17:13 nanne Exp $
________________________________________________________________________

-*/

#include "uibatchlaunch.h"

#include "filegen.h"
#include "ioparlist.h"
#include "strmdata.h"
#include "strmprov.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uimsg.h"

#include <fstream.h>


uiGenBatchLaunch::uiGenBatchLaunch( uiParent* p, UserIDSet nms )
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
    const char* prognm = progfld->box()->getText();
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

    return true;
}

const char* uiGenBatchLaunch::getProg()
{
    return progfld->box()->getText();
}



uiBatchLaunch::uiBatchLaunch( uiParent* p, const IOParList& pl,
			      BufferString& hn, const char* pn, bool wp )
        : uiDialog(p,uiDialog::Setup("Batch launch","Specify output mode",
		   "0.1.4"))
	, iopl(pl)
	, hostname(hn)
	, progname(pn)
	, opts("Output to")
{
    opts.add( "Output window" );
    opts.add( "Standard output" );
    opts.add( "Log file" );
    if ( wp )
	opts.add( "Parameter report (no run)" );
    opts.setCurrent( 0 );
    optfld = new uiLabeledComboBox( this, Ptr(opts) );
    optfld->box()->selectionChanged.notify( mCB(this,uiBatchLaunch,optSel) );

    BufferString fname = "/tmp/out";
    if ( GetSoftwareUser() )
	fname += GetSoftwareUser();
    fname += ".log";
    filefld = new uiFileInput( this, "Output file", fname, false );
    filefld->attach( alignedBelow, optfld );

    remfld = new uiGenInput( this, "Execute", 
			     BoolInpSpec("Local", "Remote (using rsh)") );
    remfld->attach( alignedBelow, filefld );
    remfld->changed.notify( mCB(this,uiBatchLaunch,remSel) );

    remhostfld = new uiGenInput( this, "Hostname", StringInpSpec(hostname) );
    remhostfld->attach( alignedBelow, remfld );

    optSel(0);
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
    int sel = selected();
    filefld->display( sel > 1 );
    if ( sel != 2 ) remfld->setValue( true );
    remSel(0);
}


void uiBatchLaunch::remSel( CallBacker* )
{
    bool doshw = selected() == 2;
    bool isrem = execRemote();
    remfld->display( doshw );
    remhostfld->display( doshw && isrem );
    optfld->box()->setSensitive( !(doshw && isrem) );
}


int uiBatchLaunch::selected()
{
    return optfld->box()->currentItem();
}


bool uiBatchLaunch::acceptOK( CallBacker* )
{
    BufferString fname;
    int sel = selected();
    if ( sel > 1 )		fname = filefld->fileName();
    else if ( sel == 0 )	fname = "window";
    else			fname = "stdout";

    if ( fname == "" )   fname = "/dev/null";
    if ( execRemote() )
    {
	hostname = remhostfld->text();
	if ( hostname == "" )
	{
	    uiMSG().error( "Please specify the name of the remote host" );
	    return false;
	}
    }

    IOPar* iop = const_cast<IOPar*>(iopl.size() ? iopl[0] : 0);
    if ( iop ) iop->set( "Log file", fname );

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

    BufferString comm( "@" );
    if ( !execRemote() )
	{ comm += progname; comm += " -bg "; comm += StreamProvider::sStdIO; }
    else
    {
	BufferString tfname = File_getFullPath( GetDataDir(), ".transfer." );
	if ( GetSoftwareUser() )
	    tfname += GetSoftwareUser();
	ofstream strm( tfname );
	if ( !iopl.write(strm) )
	{
	    comm = "Cannot write to:\n";
	    comm += tfname;
	    uiMSG().error( comm );
	    return false;
	}
	strm.close();

	comm += GetSoftwareDir();
	comm = File_getFullPath( comm, "bin" );
	comm = File_getFullPath( comm, "GDIexec.rmt" );
	comm += " ";
	comm += hostname; comm += " ";
	comm += progname; comm += " -bg ";
	comm += tfname;
    }

    bool rv = false;
    StreamData sd = StreamProvider( comm ).makeOStream();
    if ( !sd.usable() )
    {
	uiMSG().error( "Cannot create pipe to processing application" );
	sd.close();
	return false;
    }
    else if ( !iopl.write( *sd.ostrm ) )
    {
	uiMSG().error( "Error during write to processing application" );
	sd.close();
	return false;
    }
    else
	rv = true;

    sd.close();
    return rv;
}

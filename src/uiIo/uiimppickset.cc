/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.cc,v 1.19 2006-05-16 16:28:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiimppickset.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "ioobj.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "filegen.h"
#include "pickset.h"
#include "picksettr.h"

#include <math.h>


uiImpExpPickSet::uiImpExpPickSet( uiParent* p, bool imp )
    : uiDialog(p,uiDialog::Setup(imp ? "Import Pickset" : "Export PickSet",
				 "Specify pickset parameters",
				 imp ? "105.0.1" : "105.0.2"))
    , ctio(*mMkCtxtIOObj(PickSet))
    , import(imp)
    , xyfld(0)
{
    BufferString label( import ? "Input " : "Output " );
    label += "Ascii file";
    filefld = new uiFileInput( this, label, uiFileInput::Setup()
					    .withexamine(import)
					    .forread(import) );
    filefld->setDefaultSelectionDir( 
			    IOObjContext::getDataDirName(IOObjContext::Loc) );

    ctio.ctxt.forread = !import;
    ctio.ctxt.maychdir = false;
    label = import ? "Output " : "Input "; label += "PickSet";
    objfld = new uiIOObjSel( this, ctio, label );

    if ( import )
    {
	xyfld = new uiGenInput( this, "Positions in:",
				BoolInpSpec("X/Y","Inl/Crl") );
	xyfld->attach( alignedBelow, filefld );
	objfld->attach( alignedBelow, xyfld );
    }
    else
	filefld->attach( alignedBelow, objfld );
}


uiImpExpPickSet::~uiImpExpPickSet()
{
    delete ctio.ioobj; delete &ctio;
}

#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiImpExpPickSet::doImport()
{
    const char* fname = filefld->fileName();
    StreamData sdi = StreamProvider( fname ).makeIStream();
    if ( !sdi.usable() ) 
    { 
	sdi.close();
	mErrRet( "Could not open input file" )
    }

    const char* psnm = objfld->getInput();
    Pick::Set ps( psnm );
    ps.color_ = Color::DgbColor;

    bool doxy = xyfld ? xyfld->getBoolValue() : true;
    bool firstpos = true;
    bool doscale = false;
    Pick::Location ploc;
    char buf[1024];
    const float zfactor = SI().zFactor();
    while ( true )
    {
	sdi.istrm->getline( buf, 1024 );
	if ( !ploc.fromString(buf,doxy) )
	    break;

        if ( firstpos )
        {
            firstpos = false;
            doscale = ploc.pos.z > 2 * SI().zRange(false).stop &&
                      SI().zRange(false).includes( ploc.pos.z / zfactor );
        }

	if ( doscale )
	    ploc.pos.z /= zfactor;
	if ( ploc.hasDir() )
	{
	    ploc.dir.theta *= M_PI/180;
	    ploc.dir.phi *= M_PI/180;
	}

	ps += ploc;
    }

    sdi.close();

    BufferString errmsg;
    if ( !PickSetTranslator::store(ps,ctio.ioobj,errmsg) )
	mErrRet(errmsg);

    return true;
}


bool uiImpExpPickSet::doExport()
{
    Pick::Set ps;
    BufferString errmsg;
    if ( !PickSetTranslator::retrieve(ps,ctio.ioobj,errmsg) )
	mErrRet(errmsg);

    const char* fname = filefld->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() ) 
    { 
	sdo.close();
	mErrRet( "Could not open output file" )
    }

    char buf[80];
    for ( int locidx=0; locidx<ps.size(); locidx++ )
    {
	ps[locidx].toString( buf );
	*sdo.ostrm << buf << '\n';
    }

    *sdo.ostrm << '\n';
    sdo.close();
    return true;
}


bool uiImpExpPickSet::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;
    bool ret = import ? doImport() : doExport();
    return ret;
}


bool uiImpExpPickSet::checkInpFlds()
{
    BufferString filenm = filefld->fileName();
    if ( import && !File_exists(filenm) )
	mErrRet( "Please select input file" );

    if ( !import && filenm == "" )
	mErrRet( "Please select output file" );

    if ( !objfld->commitInput( true ) )
	mErrRet( "Please select PickSet" );

    return true;
}

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.cc,v 1.13 2004-07-21 13:20:29 nanne Exp $
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
				 "Specify pickset parameters","105.0.1"))
    , ctio(*mMkCtxtIOObj(PickSetGroup))
    , import(imp)
{
    BufferString label( import ? "Input " : "Output " );
    label += "Ascii file";
    filefld = new uiFileInput( this, label, uiFileInput::Setup()
					    .withexamine(import)
					    .forread(!import) );
    filefld->setDefaultSelectionDir( 
			    IOObjContext::getDataDirName(IOObjContext::Loc) );

    xyfld = new uiGenInput( this, "Positions in:",
			    BoolInpSpec("X/Y","Inl/Crl") );

    ctio.ctxt.forread = !import;
    ctio.ctxt.maychdir = false;
    label = import ? "Output " : "Input "; label += "PickSet";
    objfld = new uiIOObjSel( this, ctio, label );


    if ( import )
    {
	xyfld->attach( alignedBelow, filefld );
	objfld->attach( alignedBelow, xyfld );
    }
    else
    {
	xyfld->attach( alignedBelow, objfld );
	filefld->attach( alignedBelow, xyfld );
    }
}


uiImpExpPickSet::~uiImpExpPickSet()
{
    delete ctio.ioobj; delete &ctio;
}

#define mWarnRet(s) { uiMSG().warning(s); return false; }
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
    PickSet* ps = new PickSet( psnm );
    ps->color = Color::DgbColor;

    bool doxy = xyfld->getBoolValue();
    bool firstpos = true;
    bool doscale = false;
    PickLocation ploc;
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
            doscale = ploc.z > 2 * SI().zRange(false).stop &&
                      SI().zRange(false).includes( ploc.z / zfactor );
        }

	if ( doscale )
	    ploc.z /= zfactor;
	if ( ploc.hasDir() )
	{
	    ploc.dir.theta *= M_PI/180;
	    ploc.dir.phi *= M_PI/180;
	}

	*ps += ploc;
    }

    sdi.close();

    PickSetGroup psg;
    psg.clear();
    psg.add( ps );
    BufferString bs;
    if ( !PickSetGroupTranslator::store( psg, ctio.ioobj, bs ) )
	{ uiMSG().error(bs); return false; }

    return true;
}


bool uiImpExpPickSet::doExport()
{
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
    if ( !File_exists(filefld->fileName()) )
	mWarnRet( "Please select ascii file,\nor file does not exist" );

    if ( !objfld->commitInput( true ) )
	mWarnRet( "Please select PickSet" );

    return true;
}


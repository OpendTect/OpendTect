/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.cc,v 1.14 2004-07-30 11:40:13 nanne Exp $
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
    , xyfld(0)
{
    BufferString label( import ? "Input " : "Output " );
    label += "Ascii file";
    filefld = new uiFileInput( this, label, uiFileInput::Setup()
					    .withexamine(import)
					    .forread(!import) );
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

    bool doxy = xyfld ? xyfld->getBoolValue() : true;
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
    psg.add( ps );
    BufferString errmsg;
    if ( !PickSetGroupTranslator::store(psg,ctio.ioobj,errmsg) )
	mErrRet(errmsg);

    return true;
}


bool uiImpExpPickSet::doExport()
{
    PickSetGroup psg;
    BufferString errmsg;
    if ( !PickSetGroupTranslator::retrieve(psg,ctio.ioobj,errmsg) )
	mErrRet(errmsg);

    const char* fname = filefld->fileName();
    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() ) 
    { 
	sdo.close();
	mErrRet( "Could not open output file" )
    }

    for ( int setidx=0; setidx<psg.nrSets(); setidx++ )
    {
	PickSet& ps = *psg.get( setidx );
	char buf[80];
	for ( int locidx=0; locidx<ps.size(); locidx++ )
	{
	    ps[locidx].toString( buf );
	    *sdo.ostrm << buf << '\n';
	}

	*sdo.ostrm << '\n';
    }
   
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
    if ( !File_exists(filefld->fileName()) )
	mWarnRet( "Please select ascii file,\nor file does not exist" );

    if ( !objfld->commitInput( true ) )
	mWarnRet( "Please select PickSet" );

    return true;
}


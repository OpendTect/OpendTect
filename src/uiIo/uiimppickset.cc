/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.cc,v 1.2 2002-08-08 08:55:35 nanne Exp $
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


uiImportPickSet::uiImportPickSet( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Import Pickset",
				     "Specify pickset parameters",0))
	, ctio(*new CtxtIOObj(PickSetGroupTranslator::ioContext()))
{
    infld = new uiFileInput( this, "Input Ascii file");
    BufferString dirnm( GetDataDir() );
    dirnm = File_getFullPath( dirnm, "Locations" );
    infld->setDefaultSelectionDir( dirnm );

    ctio.ctxt.forread = false;
    ctio.ctxt.maychdir = false;
    outfld = new uiIOObjSel( this, ctio, "Output PickSet" );
    outfld->attach( alignedBelow, infld );
}


uiImportPickSet::~uiImportPickSet()
{
    delete &ctio;
}

#define mWarnRet(s) { uiMSG().warning(s); return false; }
#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiImportPickSet::handleAscii()
{
    const char* fname = infld->fileName();
    StreamData sdi = StreamProvider( fname ).makeIStream();
    if ( !sdi.usable() ) 
    { 
	sdi.close();
	mErrRet( "Could not open input file" )
    }

    const char* psnm = outfld->getInput();
    PickSet* ps = new PickSet( psnm );
    ps->color = Color::DgbColor;
 
    float x, y, z;
    while ( true )
    {
	*sdi.istrm >> x >> y >> z;
	if ( !(*sdi.istrm) ) break;

	BinID bid( SI().transform(Coord(x,y)) );
	if ( !SI().isReasonable(bid) )
	{
	    // It may be a BinID
	    bid.inl = mNINT(x); bid.crl = mNINT(y);
	    if ( SI().isReasonable(bid) )
	    {
		Coord c = SI().transform( bid );
		x = c.x; y = c.y;
	    }
	}

	if ( z > 2 * SI().zRange(false).stop )
	    z *= 0.001;
	*ps += PickLocation( x, y, z );
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


bool uiImportPickSet::acceptOK( CallBacker* )
{
    bool ret = checkInpFlds() && handleAscii();
    return ret;
}


bool uiImportPickSet::checkInpFlds()
{
    if ( !File_exists(infld->fileName()) )
	mWarnRet( "Please select input,\nor file does not exist" );

    if ( !outfld->commitInput( true ) )
	mWarnRet( "Please select output" );

    return true;
}


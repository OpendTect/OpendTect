/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.cc,v 1.10 2004-02-25 12:01:29 nanne Exp $
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
				     "Specify pickset parameters","105.0.1"))
	, ctio(*mMkCtxtIOObj(PickSetGroup))
{
    infld = new uiFileInput( this, "Input Ascii file");
    infld->setDefaultSelectionDir( 
	    IOObjContext::getDataDirName(IOObjContext::Loc) );

    xyfld = new uiGenInput( this, "Positions in:",
			    BoolInpSpec("X/Y","Inl/Crl") );
    xyfld->attach( alignedBelow, infld );

    ctio.ctxt.forread = false;
    ctio.ctxt.maychdir = false;
    outfld = new uiIOObjSel( this, ctio, "Output PickSet" );
    outfld->attach( alignedBelow, xyfld );
}


uiImportPickSet::~uiImportPickSet()
{
    delete ctio.ioobj; delete &ctio;
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

    bool doxy = xyfld->getBoolValue();
    bool firstpos = true;
    bool doscale = false;
    PickLocation ploc;
    char buf[1024];
    const float zfactor = SI().zFactor();
    while ( true )
    {
	sdi.istrm->getline( buf, 1024 );
	if ( !ploc.fromString(buf) )
	    break;

        if ( firstpos )
        {
            firstpos = false;

            doscale = ploc.z > 2 * SI().zRange(false).stop &&
                      SI().zRange(false).includes( ploc.z / zfactor );

            BinID bid = doxy ? SI().transform(Coord(ploc.pos.x,ploc.pos.y))
                             : BinID( mNINT(ploc.pos.x), mNINT(ploc.pos.y) );
            bool reasonable = SI().isReasonable(bid);

            if ( !reasonable
	      && !uiMSG().askGoOn( "First position in file is not valid.\n"
				   "Do you wish to continue?" ) )
                return false;
        }

        if ( !doxy )
        {
            BinID bid( mNINT(ploc.pos.x), mNINT(ploc.pos.y) );
            ploc.pos = SI().transform( bid );
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


/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          June 2002
 RCS:           $Id: uiimppickset.cc,v 1.7 2003-11-07 12:22:01 bert Exp $
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
    Coord3 pos;
    while ( true )
    {
	*sdi.istrm >> pos.x >> pos.y >> pos.z;
	if ( !(*sdi.istrm) ) break;

        if ( firstpos )
        {
            doscale = pos.z > 2 * SI().zRange(false).stop &&
                      SI().zRange(false).includes( pos.z * .001 );

            BinID bid = doxy ? SI().transform(Coord(pos.x,pos.y))
                             : BinID( mNINT(pos.x), mNINT(pos.y) );
            bool reasonable = SI().isReasonable(bid);

            BufferString msg( "First position in file is not valid.\n"
                              "Do you wish to continue?" );
            if ( !reasonable && !uiMSG().askGoOn( msg ) )
                return false;

            firstpos = false;
        }

        if ( !doxy )
        {
            BinID bid( mNINT(pos.x), mNINT(pos.y) );
            Coord crd  = SI().transform( bid );
            pos.x = crd.x; pos.y = crd.y;
        }

	if ( doscale )
	    pos.z *= 0.001;
	*ps += PickLocation( pos );
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


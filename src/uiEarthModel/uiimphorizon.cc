/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          May 2002
 RCS:           $Id: uiimphorizon.cc,v 1.1 2002-05-22 09:24:58 nanne Exp $
________________________________________________________________________

-*/

#include "uiimphorizon.h"

#include "emmanager.h"
#include "emhorizon.h"
#include "emhorizontransl.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "uiioobjsel.h"
#include "strmdata.h"
#include "strmprov.h"
#include "uiexecutor.h"
#include "uifileinput.h"
#include "filegen.h"
#include "uimsg.h"


uiImportHorizon::uiImportHorizon( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Import Horizon",
				     "Specify horizon paramaters",0))
	, ctio(*new CtxtIOObj(EarthModelHorizonTranslator::ioContext()))
{
    infld = new uiFileInput( this, "Input Ascii file");
    BufferString dirnm( GetDataDir() );
    dirnm = File_getFullPath( dirnm, "Surfaces" );
    infld->setDefaultSelectionDir( dirnm );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Horizon" );
    outfld->attach( alignedBelow, infld );
}


uiImportHorizon::~uiImportHorizon()
{
    delete &ctio;
}


#define mWarnRet(s) { uiMSG().warning(s); return false; }
#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiImportHorizon::handleAscii()
{
    const char* fname = infld->fileName();
    StreamData sdi = StreamProvider( fname ).makeIStream();
    if ( !sdi.usable() ) 
    { 
	sdi.close();
	mErrRet( "Could not open input file" )
    }

    const char* horizonnm = outfld->getInput();
    EarthModel::EMManager& em = EarthModel::EMM();
    MultiID key = em.add( EarthModel::EMManager::Hor, horizonnm );
    mDynamicCastGet( EarthModel::Horizon*, horizon, em.getObject( key ) );
    if ( !horizon )
	mErrRet( "Cannot create horizon" )
    
    Geometry::Pos pos;
    while ( *sdi.istrm )
    {
	*sdi.istrm >> pos.x >> pos.y >> pos.z;
// TODO: add points to horizon
    }

    sdi.close();

    BufferString msg;
    Executor* exec = EarthModelHorizonTranslator::writer( *horizon, ctio.ioobj,
							  msg );
    uiExecutor dlg( this, *exec );
    dlg.go();
    if ( msg && *msg )
	mErrRet( msg )

    return true;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    return false; // TODO: remove this line
    bool ret = checkInpFlds() && handleAscii();
    return ret;
}


bool uiImportHorizon::checkInpFlds()
{
    if ( !File_exists(infld->fileName()) )
	mWarnRet( "Please select input,\nor file does not exist" );

    if ( !outfld->commitInput( true ) )
	mWarnRet( "Please select output" );

    return true;
}


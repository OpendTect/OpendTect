/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellimpasc.cc,v 1.2 2003-08-28 08:20:48 nanne Exp $
________________________________________________________________________

-*/

#include "uiwellimpasc.h"
#include "wellimpasc.h"
#include "welldata.h"
#include "welltransl.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "filegen.h"
#include "ptrman.h"


uiWellImportAsc::uiWellImportAsc( uiParent* p )
	: uiDialog(p,uiDialog::Setup("Import Well Track",
				     "Specify well parameters","104.2.0"))
	, ctio(*new CtxtIOObj(WellTranslator::ioContext()))
{
    infld = new uiFileInput( this, "Input Ascii file", 
	    		     uiFileInput::Setup().withexamine() );
    infld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::WllInf) );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Well" );
    outfld->attach( alignedBelow, infld );
}


uiWellImportAsc::~uiWellImportAsc()
{
    delete &ctio;
}


bool uiWellImportAsc::acceptOK( CallBacker* )
{
    bool ret = checkInpFlds() && doWork();
    return ret;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiWellImportAsc::doWork()
{
    PtrMan<Well::Data> well = new Well::Data( outfld->getInput() );
    Well::AscImporter ascimp( *well );

    const char* fname = infld->fileName();
    const char* errmsg = ascimp.getTrack( fname, true ); // TODO: check args
    if ( errmsg ) mErrRet( errmsg );

    PtrMan<Translator> t = ctio.ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) mErrRet( "Object is not a well" );

    if ( !wtr->write(*well,*ctio.ioobj) ) mErrRet( "Cannot write well" );

    return true;
}


bool uiWellImportAsc::checkInpFlds()
{
    if ( ! *infld->fileName() )
	mErrRet( "Please select the input file" )
    else if ( !File_exists(infld->fileName()) )
	mErrRet( "Input file does not exist" )

    if ( !outfld->commitInput( true ) )
	mErrRet( "Please select output" )

    return true;
}

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellimpasc.cc,v 1.8 2003-10-31 14:26:32 nanne Exp $
________________________________________________________________________

-*/

#include "uiwellimpasc.h"
#include "wellimpasc.h"
#include "welldata.h"
#include "welltransl.h"
#include "welltrack.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uibutton.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "filegen.h"
#include "survinfo.h"
#include "ptrman.h"


uiWellImportAsc::uiWellImportAsc( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Well Track",
				 "Specify well parameters","107.0.0"))
    , ctio(*mMkCtxtIOObj(Well))
{
    infld = new uiFileInput( this, "Input Ascii file", 
	    		     uiFileInput::Setup().withexamine() );
    infld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::WllInf) );

    bool zistime = SI().zIsTime();
    if ( zistime )
    {
	d2tfld = new uiFileInput( this, "Depth to Time model",
				  uiFileInput::Setup().withexamine() );
	d2tfld->setDefaultSelectionDir(
		IOObjContext::getDataDirName(IOObjContext::WllInf) );
	d2tfld->attach( alignedBelow, infld );

	tvdfld = new uiCheckBox( this, "Model is TVD" );
	tvdfld->attach( alignedBelow, d2tfld );
    }

    coordfld = new uiGenInput( this, "Surface coordinate",
	    		       BinIDCoordInpSpec(true) );
    coordfld->attach( alignedBelow, zistime ? (uiObject*)tvdfld
	   				    : (uiObject*)infld );

    elevfld = new uiGenInput( this, "Surface elevation", FloatInpSpec() );
    elevfld->attach( alignedBelow, coordfld );

    unitfld = new uiGenInput( this, "Depth unit", BoolInpSpec("Meter","Feet") );
    unitfld->attach( alignedBelow, elevfld );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Well" );
    outfld->attach( alignedBelow, unitfld );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, outfld );

    uiLabel* infolbl = new uiLabel( this, "Optional information:" );
    infolbl->attach( alignedBelow, horsep );

    idfld = new uiGenInput( this, "Well ID" );
    idfld->attach( alignedBelow, outfld );
    idfld->attach( ensureBelow, infolbl );
    
    operfld = new uiGenInput( this, "Operator" );
    operfld->attach( alignedBelow, idfld );
    
    statefld = new uiGenInput( this, "State" );
    statefld->attach( alignedBelow, operfld );

    countyfld = new uiGenInput( this, "County" );
    countyfld->attach( rightTo, statefld );
}


uiWellImportAsc::~uiWellImportAsc()
{
    delete ctio.ioobj; delete &ctio;
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

    Well::Info& info = well->info();
    info.uwid = idfld->text();
    info.oper = operfld->text();
    info.state = statefld->text();
    info.county = countyfld->text();
    info.surfacecoord = coordfld->getCoord();

    Well::AscImporter ascimp( *well );

    const char* fname = infld->fileName();
    const bool zinfeet = !unitfld->getBoolValue();
    const char* errmsg = ascimp.getTrack( fname, true, zinfeet );
    if ( errmsg ) mErrRet( errmsg );

    if ( SI().zIsTime() )
    {
	fname = d2tfld->fileName();
	errmsg = ascimp.getD2T( fname, tvdfld->isChecked(), zinfeet );
	if ( errmsg ) mErrRet( errmsg );
    }

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

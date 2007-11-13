/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellimpasc.cc,v 1.36 2007-11-13 16:21:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiwellimpasc.h"

#include "ctxtioobj.h"
#include "filegen.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "welltrack.h"
#include "welltransl.h"

#include "uid2tmodelgrp.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"


uiWellImportAsc::uiWellImportAsc( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Well Track",
				 "Specify well parameters","107.0.0"))
    , ctio(*mMkCtxtIOObj(Well))
{
    setCtrlStyle( DoAndStay );

    infld = new uiFileInput( this, "Well Track file", 
	    		     uiFileInput::Setup().withexamine(true) );
    infld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::WllInf) );

    const bool zistime = SI().zIsTime();
    if ( zistime )
    {
	d2tgrp = new uiD2TModelGroup( this, false );
	d2tgrp->attach( alignedBelow, infld );
    }

    coordfld = new uiGenInput( this, "Surface coordinate",
			PositionInpSpec( PositionInpSpec::Setup(true) ) );
    coordfld->attach( alignedBelow, zistime ? (uiObject*)d2tgrp
	   				    : (uiObject*)infld );

    elevfld = new uiGenInput( this, "Elevation above surface", FloatInpSpec() );
    elevfld->attach( alignedBelow, coordfld );

    BoolInpSpec mft( !SI().depthsInFeetByDefault(), "Meter", "Feet" );
    unitfld = new uiGenInput( this, "Depth unit", mft );
    unitfld->attach( alignedBelow, elevfld );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Well" );
    outfld->attach( alignedBelow, unitfld );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, outfld );

    uiLabel* infolbl = new uiLabel( this, "Optional information:" );
    infolbl->attach( alignedBelow, horsep );
    uiLabel* RTFMlbl = new uiLabel( this,
	    			"[Logs and markers: use 'File-Manage-Wells']" );
    RTFMlbl->attach( ensureBelow, horsep );
    RTFMlbl->attach( rightBorder );

    idfld = new uiGenInput( this, "Well ID (UWI)" );
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

    SI().getPars().setYN( SurveyInfo::sKeyDpthInFt, !unitfld->getBoolValue() );
    SI().savePars();
    return ret;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiWellImportAsc::doWork()
{
    PtrMan<Well::Data> well = new Well::Data( outfld->getInput() );
    const bool zinfeet = !unitfld->getBoolValue();

    Well::Info& info = well->info();
    info.uwid = idfld->text();
    info.oper = operfld->text();
    info.state = statefld->text();
    info.county = countyfld->text();
    info.surfacecoord = coordfld->getCoord();
    info.surfaceelev = -elevfld->getfValue();
    if ( zinfeet && !mIsUdf(info.surfaceelev) ) 
	info.surfaceelev *= 0.3048;

    Well::AscImporter ascimp( *well );
    const char* errmsg = ascimp.getTrack( infld->fileName(), true, zinfeet );
    if ( errmsg ) mErrRet( errmsg );

    if ( SI().zIsTime() )
    {
	errmsg = ascimp.getD2T( d2tgrp->fileName(), d2tgrp->isTVD(),
				d2tgrp->isTWT(), zinfeet );
	if ( errmsg ) mErrRet( errmsg );
    }

    PtrMan<Translator> t = ctio.ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
	    		 "Another type object with this name already exists." );

    if ( !wtr->write(*well,*ctio.ioobj) ) mErrRet( "Cannot write well" );

    uiMSG().message( "Welltrack successfully imported" );
    return false;
}


bool uiWellImportAsc::checkInpFlds()
{
    if ( ! *infld->fileName() )
	mErrRet( "Please select 'Well Track' file" )

    if ( SI().zIsTime() && ! *d2tgrp->fileName() )
	mErrRet( "Please select 'Depth to Time model' file" )

    if ( !outfld->commitInput(true) )
	mErrRet( "Please select output" )

    return true;
}

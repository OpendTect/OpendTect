/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellimpasc.cc,v 1.44 2009-03-24 12:33:52 cvsbert Exp $";

#include "uiwellimpasc.h"

#include "ctxtioobj.h"
#include "filegen.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"
#include "strmprov.h"
#include "tabledef.h"
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
#include "uitblimpexpdatasel.h"


uiWellImportAsc::uiWellImportAsc( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Well Track",
				 "Specify well parameters","107.0.0"))
    , ctio( *mMkCtxtIOObj(Well) )
    , fd( *Well::WellAscIO::getDesc() )			       
{
    setCtrlStyle( DoAndStay );

    wtinfld = new uiFileInput( this, "Well Track File",
	    			   uiFileInput::Setup().withexamine(true) );
    wtinfld->setDefaultSelectionDir(
	    	  IOObjContext::getDataDirName(IOObjContext::WllInf) );

    dataselfld = new uiTableImpDataSel( this, fd, 0 );
    dataselfld->attach( alignedBelow, wtinfld );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, dataselfld );

    const bool zistime = SI().zIsTime();
    if ( zistime )
    {
	d2tgrp = new uiD2TModelGroup( this, false );
	d2tgrp->attach( alignedBelow, dataselfld );
	d2tgrp->attach( ensureBelow, sep );
    }

    coordfld = new uiGenInput( this, "Surface coordinate",
			PositionInpSpec( PositionInpSpec::Setup(true))
			   		 .setName("X",0).setName("Y",1) );
    coordfld->attach( alignedBelow, zistime ? (uiObject*)d2tgrp
	   				    : (uiObject*)dataselfld );
    if ( !zistime )
	coordfld->attach( ensureBelow, sep );

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
    delete &fd;
}


bool uiWellImportAsc::acceptOK( CallBacker* )
{
    bool ret = checkInpFlds() && doWork();

    SI().getPars().setYN( SurveyInfo::sKeyDpthInFt(), !unitfld->getBoolValue() );
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

    BufferString fnm( wtinfld->fileName() );
    if ( !fnm.isEmpty() )
    {
	StreamData sd = StreamProvider( wtinfld->fileName() ).makeIStream();
	if ( !sd.usable() )
	    mErrRet( "Cannot open input file" )

	Well::WellAscIO wellascio(fd, *sd.istrm );

	if ( !wellascio.getData( *well, true ) )
	    mErrRet( "Failed to convert into compatible data" );

	sd.close();
    }

    Well::AscImporter ascimp( *well );

    if ( SI().zIsTime() )
    {
	fnm = d2tgrp->fileName();
	if ( !fnm.isEmpty() )
	{
	    const char* errmsg = ascimp.getD2T( fnm, d2tgrp->isTVD(),
				    d2tgrp->isTWT(), zinfeet );
	    if ( errmsg ) mErrRet( errmsg );
	}
    }

    PtrMan<Translator> t = ctio.ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
	    		 "Another type object with this name already exists." );

    if ( !wtr->write(*well,*ctio.ioobj) ) mErrRet( "Cannot write well" );

    uiMSG().message( "Well successfully created" );
    return false;
}


bool uiWellImportAsc::checkInpFlds()
{
    const bool havetrack = *wtinfld->fileName();
    const bool haved2t = !SI().zIsTime() || *d2tgrp->fileName();
    if ( !havetrack || !haved2t )
    {
	BufferString msg( "" );
	msg += havetrack? "Remember ":"";
        msg +=	"you need to ";
	msg += havetrack ? "generate a " : "have a ";
	msg += havetrack ? "Depth/Time model" : "Track";
	msg += "\nto be able to use and display the new well";
	msg += havetrack? ".":"\nDo you want to continue ?";
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

    if ( !outfld->commitInput() )
	mErrRet( "Please select output" )

    return true;
}

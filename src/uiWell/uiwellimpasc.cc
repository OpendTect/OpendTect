/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellimpasc.cc,v 1.47 2009-04-21 12:07:45 cvsbert Exp $";

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
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitblimpexpdatasel.h"

static const char* sHelpID = "107.0.0";


uiWellImportAsc::uiWellImportAsc( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Well Track",
				 "Import Well Track",sHelpID))
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
	uiD2TModelGroup::Setup su; su.asksetcsmdl( true );
	d2tgrp = new uiD2TModelGroup( this, su );
	d2tgrp->attach( alignedBelow, dataselfld );
	d2tgrp->attach( ensureBelow, sep );
	sep = new uiSeparator( this, "H sep 2" );
	sep->attach( stretchedBelow, d2tgrp );
    }


    uiButton* but = new uiPushButton( this, "Advanced/Optional",
	    				mCB(this,uiWellImportAsc,doAdvOpt),
					false );
    but->attach( alignedBelow, zistime ? (uiObject*)d2tgrp
	    			       : (uiObject*)dataselfld );
    but->attach( ensureBelow, sep );

    ctio.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio, "Output Well" );
    outfld->attach( alignedBelow, but );
}


uiWellImportAsc::~uiWellImportAsc()
{
    delete ctio.ioobj; delete &ctio;
    delete &fd;
}


class uiWellImportAscOptDlg : public uiDialog
{
public:

uiWellImportAscOptDlg( uiWellImportAsc* p )
    : uiDialog(p,uiDialog::Setup("Import well: Advanced/Optional",
				 "Advanced and Optional",sHelpID))
    , uwia_(p)
{
    coordfld = new uiGenInput( this,
	"Surface coordinate (if different from first coordinate in track file)",
	PositionInpSpec( PositionInpSpec::Setup(true))
	.setName("X",0).setName("Y",1) );

    elevfld = new uiGenInput( this,
	    "Surface Reference Datum (SRD)", FloatInpSpec(0) );
    elevfld->attach( alignedBelow, coordfld );
    zinftbox = new uiCheckBox( this, "Feet" );
    zinftbox->attach( rightOf, elevfld );
    zinftbox->setChecked( SI().depthsInFeetByDefault() );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, elevfld );

    idfld = new uiGenInput( this, "Well ID (UWI)" );
    idfld->attach( alignedBelow, elevfld );
    
    operfld = new uiGenInput( this, "Operator" );
    operfld->attach( alignedBelow, idfld );
    
    statefld = new uiGenInput( this, "State" );
    statefld->attach( alignedBelow, operfld );

    countyfld = new uiGenInput( this, "County" );
    countyfld->attach( rightTo, statefld );
}


bool acceptOK( CallBacker* )
{
    Well::Info& info = uwia_->wd_.info();

    if ( *coordfld->text() )
	info.surfacecoord = coordfld->getCoord();
    if ( *elevfld->text() )
    {
	info.surfaceelev = -elevfld->getfValue();
	if ( zinftbox->isChecked() && !mIsUdf(info.surfaceelev) ) 
	    info.surfaceelev *= 0.3048;
    }

    info.uwid = idfld->text();
    info.oper = operfld->text();
    info.state = statefld->text();
    info.county = countyfld->text();

    return true;
}

    uiWellImportAsc*	uwia_;
    uiGenInput*		coordfld;
    uiGenInput*		elevfld;
    uiGenInput*		idfld;
    uiGenInput*		operfld;
    uiGenInput*		statefld;
    uiGenInput*		countyfld;
    uiCheckBox*		zinftbox;
};


void uiWellImportAsc::doAdvOpt( CallBacker* )
{
    uiWellImportAscOptDlg dlg( this );
    dlg.go();
}


bool uiWellImportAsc::acceptOK( CallBacker* )
{
    return checkInpFlds() && doWork();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiWellImportAsc::doWork()
{
    wd_.info().setName( outfld->getInput() );

    BufferString fnm( wtinfld->fileName() );
    if ( !fnm.isEmpty() )
    {
	StreamData sd = StreamProvider( wtinfld->fileName() ).makeIStream();
	if ( !sd.usable() )
	    mErrRet( "Cannot open track file" )

	Well::WellAscIO wellascio( fd, *sd.istrm );

	if ( !wellascio.getData(wd_,true) )
	    mErrRet( "The well track file cannot be loaded with given format" );

	sd.close();
    }

    if ( SI().zIsTime() )
    {
	const char* errmsg = d2tgrp->checkInput();
	if ( errmsg ) mErrRet( errmsg );
	Well::AscImporter ascimp( wd_ );
	errmsg = ascimp.getD2T( d2tgrp->getMI(), false );
	if ( errmsg ) mErrRet( errmsg );
	if ( d2tgrp->wantAsCSModel() )
	    ascimp.getD2T( d2tgrp->getMI(), true );
    }

    PtrMan<Translator> t = ctio.ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
	    		 "Another type object with this name already exists." );

    if ( !wtr->write(wd_,*ctio.ioobj) ) mErrRet( "Cannot write well" );

    uiMSG().message( "Well successfully created" );
    return false;
}


bool uiWellImportAsc::checkInpFlds()
{
    const bool havetrack = *wtinfld->fileName();
    if ( !*wtinfld->fileName() )
	mErrRet("Please specify a well track file")

    const char* errmsg = SI().zIsTime() ? 0 : d2tgrp->checkInput();
    if ( errmsg && *errmsg )
	mErrRet( errmsg )

    if ( !outfld->commitInput() )
	mErrRet( "Please select output" )

    return true;
}

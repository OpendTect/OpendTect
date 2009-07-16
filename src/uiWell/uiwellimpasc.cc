/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwellimpasc.cc,v 1.53 2009-07-16 13:00:38 cvsbert Exp $";

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
#include "uimsg.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uiselsurvranges.h"
#include "uitblimpexpdatasel.h"

static const char* sHelpID = "107.0.0";


uiWellImportAsc::uiWellImportAsc( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Well Track",
				 "Import Well Track",sHelpID))
    , ctio_( *mMkCtxtIOObj(Well) )
    , fd_( *Well::TrackAscIO::getDesc() )			       
    , trckinpfld_(0)
    , zrgfld_(0)
    , tmzrgfld_(0)
{
    setCtrlStyle( DoAndStay );

    havetrckbox_ = new uiCheckBox( this, "" );
    havetrckbox_->activated.notify( mCB(this,uiWellImportAsc,haveTrckSel) );
    havetrckbox_->setChecked( true );
    trckinpfld_ = new uiFileInput( this, "Well Track File",
	    			   uiFileInput::Setup().withexamine(true) );
    trckinpfld_->setDefaultSelectionDir(
	    	  IOObjContext::getDataDirName(IOObjContext::WllInf) );
    trckinpfld_->attach( rightOf, havetrckbox_ );
    vertwelllbl_ = new uiLabel( this, "-> Vertical well" );
    vertwelllbl_->attach( rightTo, havetrckbox_ );
    vertwelllbl_->attach( alignedWith, trckinpfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, 0 );
    dataselfld_->attach( alignedBelow, trckinpfld_ );

    coordfld_ = new uiGenInput( this, "Coordinate",
	    			PositionInpSpec(PositionInpSpec::Setup(true)) );
    coordfld_->attach( alignedBelow, trckinpfld_ );
    if ( !SI().zIsTime() )
    {
	zrgfld_ = new uiSelZRange( this, false );
	zrgfld_->attach( alignedBelow, coordfld_ );
    }
    else
    {
	tmzrgfld_ = new uiGenInput( this, "Depth range", FloatInpSpec(0),
	       						 FloatInpSpec()	);
	tmzrgfld_->attach( alignedBelow, coordfld_ );
	tmzrginftbox_ = new uiCheckBox( this, "Feet" );
	tmzrginftbox_->attach( rightOf, tmzrgfld_ );
	tmzrginftbox_->setChecked( SI().xyInFeet() );
    }

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, dataselfld_ );

    const bool zistime = SI().zIsTime();
    if ( zistime )
    {
	uiD2TModelGroup::Setup su; su.asksetcsmdl( true );
	d2tgrp_ = new uiD2TModelGroup( this, su );
	d2tgrp_->attach( alignedBelow, dataselfld_ );
	d2tgrp_->attach( ensureBelow, sep );
	sep = new uiSeparator( this, "H sep 2" );
	sep->attach( stretchedBelow, d2tgrp_ );
    }

    uiButton* but = new uiPushButton( this, "Advanced/Optional",
	    				mCB(this,uiWellImportAsc,doAdvOpt),
					false );
    but->attach( alignedBelow, zistime ? (uiObject*)d2tgrp_
	    			       : (uiObject*)dataselfld_ );
    but->attach( ensureBelow, sep );

    ctio_.ctxt.forread = false;
    outfld_ = new uiIOObjSel( this, ctio_, "Output Well" );
    outfld_->attach( alignedBelow, but );

    finaliseDone.notify( mCB(this,uiWellImportAsc,haveTrckSel) );
}


uiWellImportAsc::~uiWellImportAsc()
{
    delete ctio_.ioobj; delete &ctio_;
    delete &fd_;
}


void uiWellImportAsc::haveTrckSel( CallBacker* )
{
    if ( !trckinpfld_ ) return;
    const bool havetrck = havetrckbox_->isChecked();
    trckinpfld_->display( havetrck );
    dataselfld_->display( havetrck );
    vertwelllbl_->display( !havetrck );
    coordfld_->display( !havetrck );
    if ( zrgfld_ )
	zrgfld_->display( !havetrck );
    else
    {
	tmzrgfld_->display( !havetrck );
	tmzrginftbox_->display( !havetrck );
    }
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
	    info.surfaceelev *= mFromFeetFactor;
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
    wd_.empty();
    wd_.info().setName( outfld_->getInput() );

    if ( havetrckbox_->isChecked() )
    {
	BufferString fnm( trckinpfld_->fileName() );
	if ( !fnm.isEmpty() )
	{
	    StreamData sd = StreamProvider( trckinpfld_->fileName() )
					.makeIStream();
	    if ( !sd.usable() )
		mErrRet( "Cannot open track file" )
	    Well::TrackAscIO wellascio( fd_, *sd.istrm );
	    if ( !wellascio.getData(wd_,true) )
		mErrRet( "The track file cannot be loaded with given format" );
	    sd.close();
	}
    }
    else
    {
	const Coord c( coordfld_->getCoord() );
	Interval<float> zrg;
        if ( zrgfld_ )
	    zrg = zrgfld_->getRange();
	else
	{
	    zrg = tmzrgfld_->getFInterval();
	    if ( tmzrginftbox_->isChecked() )
		zrg.scale( mFromFeetFactor );
	}
	wd_.track().addPoint( c, zrg.start, 0 );
	wd_.track().addPoint( c, zrg.stop );
    }

    if ( SI().zIsTime() )
    {
	const char* errmsg = d2tgrp_->getD2T( wd_, false );
	if ( errmsg ) mErrRet( errmsg );
	if ( d2tgrp_->wantAsCSModel() )
	    d2tgrp_->getD2T( wd_, true );
    }

    PtrMan<Translator> t = ctio_.ioobj->getTranslator();
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
	    		 "Another type object with this name already exists." );

    if ( !wtr->write(wd_,*ctio_.ioobj) ) mErrRet( "Cannot write well" );

    uiMSG().message( "Well successfully created" );
    return false;
}


bool uiWellImportAsc::checkInpFlds()
{
    if ( havetrckbox_->isChecked() )
    {
	if ( !*trckinpfld_->fileName() )
	    mErrRet("Please specify a well track file")
    }
    else
    {
	if ( !SI().isReasonable(coordfld_->getCoord()) )
	{
	    if ( !uiMSG().askGoOn(
			"Well coordinate seems to be far outside the survey."
		    	"\nIs this correct?") );
	    return false;
	}
    }

    if ( !outfld_->commitInput() )
	mErrRet( "Please select output" )

    return true;
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          August 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwellimpasc.h"

#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "welltrack.h"
#include "welltransl.h"

#include "uibutton.h"
#include "uid2tmodelgrp.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uiselsurvranges.h"
#include "uitblimpexpdatasel.h"
#include "uiwellsel.h"


static const char* sHelpID = "107.0.0";
static const char* nHelpID = "107.0.4";


uiWellImportAsc::uiWellImportAsc( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Well Track",
				 "Import Well Track",sHelpID))
    , fd_(*Well::TrackAscIO::getDesc())
    , wd_(*new Well::Data)
    , zun_(UnitOfMeasure::surveyDefDepthUnit())
{
    setCtrlStyle( DoAndStay );

    havetrckbox_ = new uiCheckBox( this, "" );
    havetrckbox_->setChecked( true );
    havetrckbox_->activated.notify( mCB(this,uiWellImportAsc,haveTrckSel) );

    trckinpfld_ = new uiFileInput( this, "Well Track File",
	    			   uiFileInput::Setup().withexamine(true) );
    trckinpfld_->valuechanged.notify( mCB(this,uiWellImportAsc,inputChgd) );
    trckinpfld_->attach( rightOf, havetrckbox_ );

    vertwelllbl_ = new uiLabel( this, "-> Vertical well" );
    vertwelllbl_->attach( rightTo, havetrckbox_ );
    vertwelllbl_->attach( alignedWith, trckinpfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "107.0.2" );
    dataselfld_->attach( alignedBelow, trckinpfld_ );
    dataselfld_->descChanged.notify( mCB(this,uiWellImportAsc,trckFmtChg) );

    BufferString coordunitslbl( "Coordinate " );
    coordunitslbl += SI().getXYUnitString();
    coordfld_ = new uiGenInput( this, coordunitslbl,
			PositionInpSpec(PositionInpSpec::Setup(true)) );
    coordfld_->attach( alignedBelow, trckinpfld_ );

    BufferString zlbl = SI().depthsInFeetByDefault() ? " (ft) " : " (m) ";
    BufferString kblbl( "KB Elevation" ); kblbl += zlbl;
    kbelevfld_ = new uiGenInput( this, kblbl, FloatInpSpec(0) );
    kbelevfld_->attach( alignedBelow, coordfld_ );

    BufferString tdlbl( "TD" ); tdlbl += zlbl;
    tdfld_ = new uiGenInput( this, tdlbl, FloatInpSpec() );
    tdfld_->attach( alignedBelow, kbelevfld_ );

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

    outfld_ = new uiWellSel( this, false );
    outfld_->attach( alignedBelow, but );

    postFinalise().notify( mCB(this,uiWellImportAsc,haveTrckSel) );
}


uiWellImportAsc::~uiWellImportAsc()
{
    delete &fd_;
    delete &wd_;
}


void uiWellImportAsc::haveTrckSel( CallBacker* )
{
    const bool havetrck = havetrckbox_->isChecked();
    trckinpfld_->display( havetrck );
    dataselfld_->display( havetrck );
    vertwelllbl_->display( !havetrck );
    coordfld_->display( !havetrck );
    kbelevfld_->display( !havetrck );
    tdfld_->display( !havetrck );
}


void uiWellImportAsc::inputChgd( CallBacker* )
{
    FilePath fnmfp( trckinpfld_->fileName() );
    outfld_->setInputText( fnmfp.baseName() );
}


void uiWellImportAsc::trckFmtChg( CallBacker* )
{
    const Table::FormatDesc& fd = dataselfld_->desc();
    if ( !fd.isGood() )
	return;

    for ( int idx=0; idx<fd.bodyinfos_.size(); idx++ )
    {
	const Table::TargetInfo& ti = *fd.bodyinfos_[idx];
	if ( ti.name() == "Z" || ti.name() == "MD" )
	{
	    if ( ti.selection_.isInFile(0) )
		return;
	}
    }

    uiMSG().error( "The format you defined has neither Z nor MD."
		   "\nYou should define at least one."
		   "\nAs it is now, the track will not load." );
}



class uiWellImportAscOptDlg : public uiDialog
{
public:

uiWellImportAscOptDlg( uiWellImportAsc* p )
    : uiDialog(p,uiDialog::Setup("Import well: Advanced/Optional",
				 "Advanced and Optional",nHelpID))
    , uwia_(p)
    , zun_(UnitOfMeasure::surveyDefDepthUnit())
{
    const Well::Info& info = uwia_->wd_.info();

    PositionInpSpec::Setup possu( true );
    if ( !mIsZero(info.surfacecoord.x,0.1) )
	possu.coord_ = info.surfacecoord;
    coordfld = new uiGenInput( this,
	"Surface coordinate (if different from first coordinate in track file)",
	PositionInpSpec(possu).setName( "X", 0 ).setName( "Y", 1 ) );

    float dispval = -info.surfaceelev;
    if ( SI().depthsInFeetByDefault() && !mIsUdf(info.surfaceelev) && zun_ ) 
	dispval = zun_->userValue( -info.surfaceelev );
    if ( mIsZero(dispval,0.01) ) dispval = 0;
    elevfld = new uiGenInput( this, "Seismic Reference Datum",
	    			     FloatInpSpec(dispval) );
    elevfld->attach( alignedBelow, coordfld );
    zinftbox = new uiCheckBox( this, "Feet" );
    zinftbox->attach( rightOf, elevfld );
    zinftbox->setChecked( SI().depthsInFeetByDefault() );

    BufferString str = "Replacement velocity "; str += "(";
    str += UnitOfMeasure::zUnitAnnot( false, true, false );
    str += "/s)";
    replvelfld = new uiGenInput( this, str, FloatInpSpec() );
    replvelfld->attach( alignedBelow, elevfld );

    dispval = info.groundelev;
    if ( SI().depthsInFeetByDefault() && !mIsUdf(info.groundelev) && zun_ ) 
	dispval = zun_->userValue( info.groundelev );
    if ( mIsZero(dispval,0.01) ) dispval = 0;
    gdelevfld = new uiGenInput( this, "Ground level elevation", FloatInpSpec(0) );
    gdelevfld->attach( alignedBelow, replvelfld );
    zinftbox = new uiCheckBox( this, "Feet" );
    zinftbox->attach( rightOf, gdelevfld );
    zinftbox->setChecked( SI().depthsInFeetByDefault() );

    uiSeparator* horsep = new uiSeparator( this );
    horsep->attach( stretchedBelow, gdelevfld );

    idfld = new uiGenInput( this, "Well ID (UWI)", StringInpSpec(info.uwid) );
    idfld->attach( alignedBelow, gdelevfld );
    
    operfld = new uiGenInput( this, "Operator", StringInpSpec(info.oper) );
    operfld->attach( alignedBelow, idfld );
    
    statefld = new uiGenInput( this, "State", StringInpSpec(info.state) );
    statefld->attach( alignedBelow, operfld );

    countyfld = new uiGenInput( this, "County", StringInpSpec(info.county) );
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
	if ( zinftbox->isChecked() && !mIsUdf(info.surfaceelev) && zun_ ) 
	    info.surfaceelev = zun_->internalValue( info.surfaceelev );
    }

    info.replvel = replvelfld->getfValue();

    if ( *gdelevfld->text() )
    {
	info.groundelev = gdelevfld->getfValue();
	if ( zinftbox->isChecked() && !mIsUdf(info.groundelev) && zun_ )
	    info.groundelev = zun_->internalValue( info.groundelev );
    }
    else info.groundelev = 0;


    info.uwid = idfld->text();
    info.oper = operfld->text();
    info.state = statefld->text();
    info.county = countyfld->text();

    return true;
}

    const UnitOfMeasure* zun_; 
    uiWellImportAsc*	uwia_;
    uiGenInput*		coordfld;
    uiGenInput*		elevfld;
    uiGenInput*		replvelfld;
    uiGenInput*		gdelevfld;
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
    if ( checkInpFlds() )
    {
	doWork();
	wd_.info().surfacecoord.x = wd_.info().surfacecoord.y = 0;
	wd_.info().surfaceelev = 0;
    }
    return false;
}


#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

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
	    {
		BufferString msg( "The track file cannot be loaded:\n" );
		msg += wellascio.errMsg();
		mErrRet( msg.buf() );
	    }
	    sd.close();
	}
    }
    else
    {
	float kbelev = kbelevfld_->getfValue();
	if ( mIsUdf(kbelev) ) kbelev = 0;
	else if ( SI().depthsInFeetByDefault() && zun_ ) 
	    kbelev = zun_->internalValue( kbelev );

	float td = tdfld_->getfValue();
	if ( !mIsUdf(td) && SI().depthsInFeetByDefault() && zun_ )
	    td = zun_->internalValue( td ) ;
	if ( mIsUdf(td) || td < 1e-6 )
	{
	    float survzstop = SI().zRange(false).stop;
	    if ( SI().zIsTime() )
		survzstop *= 2000;
	    td = survzstop - kbelev;
	}

	Interval<float> zrg( -kbelev, td-kbelev );
	const Coord c( coordfld_->getCoord() );
	wd_.track().addPoint( c, zrg.start, 0 );
	wd_.track().addPoint( c, zrg.stop, td );
    }

    if ( SI().zIsTime() )
    {
	const char* errmsg = d2tgrp_->getD2T( wd_, false );
	if ( errmsg ) mErrRet( errmsg );
	if ( d2tgrp_->wantAsCSModel() )
	    d2tgrp_->getD2T( wd_, true );
    }

    const IOObj* ioobj = outfld_->ioobj();
    PtrMan<Translator> t = ioobj ? ioobj->getTranslator() : 0;
    mDynamicCastGet(WellTranslator*,wtr,t.ptr())
    if ( !wtr ) mErrRet( "Please choose a different name for the well.\n"
	    		 "Another type object with this name already exists." );

    if ( !wtr->write(wd_,*ioobj) ) mErrRet( "Cannot write well" );

    uiMSG().message( "Well successfully created" );
    return false;
}


bool uiWellImportAsc::checkInpFlds()
{
    if ( havetrckbox_->isChecked() )
    {
	if ( !*trckinpfld_->fileName() )
	    mErrRet("Please specify a well track file")

	if ( !dataselfld_->commit() )
	    return false;
    }
    else
    {
	if ( !SI().isReasonable(coordfld_->getCoord()) )
	{
	    if ( !uiMSG().askGoOn(
			"Well coordinate seems to be far outside the survey."
		    	"\nIs this correct?") )
		return false;
	}
    }

    if ( !outfld_->commitInput() )
	mErrRet( outfld_->isEmpty() ? "Please enter a name for the well" : 0 )

    return true;
}

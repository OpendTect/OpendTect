/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: uiwelldlgs.cc,v 1.112 2012-07-24 07:09:59 cvsbruno Exp $";

#include "uiwelldlgs.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uid2tmodelgrp.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "unitofmeasure.h"
#include "uitable.h"
#include "uitblimpexpdatasel.h"
#include "uiwellpartserv.h"

#include "ctxtioobj.h"
#include "file.h"
#include "ioobj.h"
#include "ioman.h"
#include "iodirentry.h"
#include "iopar.h"
#include "oddirs.h"
#include "randcolor.h"
#include "strmdata.h"
#include "strmprov.h"
#include "survinfo.h"
#include "tabledef.h"
#include "welld2tmodel.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "welllog.h"
#include "welltransl.h"
#include "welllogset.h"
#include "welltrack.h"


static const char* trackcollbls[] = { "X", "Y", "Z", "MD", 0 };
static const int nremptyrows = 5;

uiWellTrackDlg::uiWellTrackDlg( uiParent* p, Well::Data& d )
	: uiDialog(p,uiDialog::Setup("Well Track",
				     "Edit Well Track",
				     "107.1.7"))
	, wd_(d)
    	, track_(d.track())
    	, orgtrack_(new Well::Track(d.track()))
	, fd_( *Well::TrackAscIO::getDesc() )

{
    tbl_ = new uiTable( this, uiTable::Setup().rowdesc("Point")
	    				       .rowgrow(true) 
					       .defrowlbl(""), "Table" );
    tbl_->setColumnLabels( trackcollbls );
    tbl_->setNrRows( nremptyrows );
    tbl_->setPrefWidth( 500 );
    tbl_->setPrefHeight( 400 );

    zinftfld_ = new uiCheckBox( this, "Z in Feet" );
    zinftfld_->setChecked( SI().depthsInFeetByDefault() );
    zinftfld_->activated.notify( mCB(this,uiWellTrackDlg,fillTable) );
    zinftfld_->attach( ensureBelow, tbl_ );
    zinftfld_->attach( rightBorder );

    uiGroup* actbutgrp = new uiGroup( this, "Action buttons grp" );
    uiButton* updnowbut = new uiPushButton( actbutgrp, "&Update display",
	    				    mCB(this,uiWellTrackDlg,updNow),
					    true );
    uiButton* readbut = new uiPushButton( actbutgrp, "&Read new",
	    				    mCB(this,uiWellTrackDlg,readNew),
					    false );
    readbut->attach( rightOf, updnowbut );

    uiButton* expbut = new uiPushButton( actbutgrp, "&Export",
	    				 mCB(this,uiWellTrackDlg,exportCB),
					 false );
    expbut->attach( rightOf, readbut );
    actbutgrp->attach( centeredBelow, tbl_ );
    zinftfld_->attach( ensureRightOf, actbutgrp );

    fillTable();
}


bool uiWellTrackDlg::fillTable( CallBacker* )
{
    RowCol curcell( tbl_->currentCell() );

    const int sz = track_.nrPoints();
    int newsz = sz + nremptyrows;
    if ( newsz < 8 ) newsz = 8;
    tbl_->setNrRows( newsz );
    tbl_->clearTable();
    if ( !sz ) return false;

    const bool zinft = zinftfld_->isChecked();

    float fac = 1;
    if ( SI().zIsTime() )
	fac = zinft ? mToFeetFactor : 1;
    else
    {
	if ( SI().zInFeet() && !zinft )
	    fac = mFromFeetFactor;
	else if ( SI().zInMeter() && zinft )
	    fac = mToFeetFactor;
    }

    for ( int idx=0; idx<sz; idx++ )
    {
	const Coord3& c( track_.pos(idx) );
	tbl_->setValue( RowCol(idx,0), c.x );
	tbl_->setValue( RowCol(idx,1), c.y );
	tbl_->setValue( RowCol(idx,2), c.z*fac );
	tbl_->setValue( RowCol(idx,3), track_.dah(idx)*fac );
    }

    if ( curcell.row >= newsz ) curcell.row = newsz-1;
    tbl_->setCurrentCell( curcell );
    return true;
}


uiWellTrackDlg::~uiWellTrackDlg()
{
    delete orgtrack_;
    delete &fd_;
}


class uiWellTrackReadDlg : public uiDialog
{
public:

uiWellTrackReadDlg( uiParent* p, Table::FormatDesc& fd, Well::Track& track )
    	: uiDialog(p,uiDialog::Setup("Read new Well Track",
		    		     "Specify new Well Track","107.1.8"))
	, track_(track)							  
{
    wtinfld_ = new uiFileInput( this, "Well Track File",
    uiFileInput::Setup().withexamine(true) );
    
    uiTableImpDataSel* dataselfld = new uiTableImpDataSel( this, fd, 0 );
    dataselfld->attach( alignedBelow, wtinfld_ );
}


bool acceptOK( CallBacker* )
{
    track_.erase();
    fnm_ = wtinfld_->fileName();
    if ( File::isEmpty(fnm_.buf()) )
	{ uiMSG().error( "Invalid input file" ); return false; }
    return true;
}

    uiFileInput*	wtinfld_;
    BufferString	fnm_;
    Well::Track&        track_;
};


void uiWellTrackDlg::readNew( CallBacker* )
{
    uiWellTrackReadDlg dlg( this, fd_, track_ );
    if ( !dlg.go() ) return;

    if ( !dlg.fnm_.isEmpty() )
    {
	StreamData sd = StreamProvider( dlg.fnm_ ).makeIStream();
	if ( !sd.usable() )
	uiMSG().error( "Cannot open input file" );

	Well::TrackAscIO wellascio(fd_, *sd.istrm );
	if ( !wellascio.getData( wd_, true ) )
	    uiMSG().error( "Failed to convert into compatible data" );

	sd.close();
	
	tbl_->clearTable();
	if ( !fillTable() )
	    return;

	wd_.trackchanged.trigger();
    }
    else
	uiMSG().error( "Please select a file" );
}


bool uiWellTrackDlg::updNow( CallBacker* )
{
    track_.erase();
    const int nrrows = tbl_->nrRows();
    const bool zinft = zinftfld_->isChecked();

    float fac = 1;
    if ( SI().zIsTime() && zinft )
	fac = mFromFeetFactor;
    else if ( SI().zInFeet() && !zinft )
	fac = mToFeetFactor;
    else if ( SI().zInMeter() && zinft )
	fac = mFromFeetFactor;

    bool needfill = false;
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* sval = tbl_->text( RowCol(idx,0) );
	if ( !*sval ) continue;
	const double xval = toDouble(sval);
	sval = tbl_->text( RowCol(idx,1) );
	if ( !*sval ) continue;
	const double yval = toDouble(sval);
	sval = tbl_->text( RowCol(idx,2) );
	if ( !*sval ) continue;
	const double zval = toDouble(sval) * fac; 

	const Coord3 newc( xval, yval, zval );
	if ( !SI().isReasonable( newc ) )
	{
	    BufferString msg( "Found undefined values in row ", idx+1, "." );
	    msg.add( "Please enter valid values" );
	    uiMSG().message( msg );
	    return false;
	}

	sval = tbl_->text( RowCol(idx,3) );
	float dahval = 0;
	if ( *sval )
	    dahval = toFloat(sval) * fac; 
	else if ( idx > 0 )
	{
	    dahval = track_.dah(idx-1) + track_.pos(idx-1).distTo( newc );
	    needfill = true;
	}

	track_.addPoint( newc, newc.z, dahval );
    }

    if ( track_.nrPoints() > 1 )
    {
	wd_.info().surfacecoord = track_.pos(0);
	wd_.info().surfaceelev = track_.dah(0);
	wd_.trackchanged.trigger();
    }
    else
    {
	uiMSG().error( "Please define at least two points." );
	return false;
    }

    if ( needfill && !fillTable() )
	return false;

    return true;
}


bool uiWellTrackDlg::rejectOK( CallBacker* )
{
    track_ = *orgtrack_;
    wd_.trackchanged.trigger();
    return true;
}


bool uiWellTrackDlg::acceptOK( CallBacker* )
{
    if ( !updNow( 0 ) )
	return false;

    const int nrpts = track_.nrPoints();
    if ( nrpts < 2 ) return false;
    const int orgnrpts = orgtrack_->nrPoints();
    bool dahchg = nrpts != orgnrpts;
    if ( !dahchg )
    {
	for ( int idx=0; idx<nrpts; idx++ )
	{
	    const float dah = track_.dah(idx);
	    const float orgdah = orgtrack_->dah(idx);
	    if ( !mIsEqual(dah,orgdah,0.001) )
		{ dahchg = true; break; }
	}
    }

    if ( dahchg )
    {
	BufferString msg( "You have changed at least one MD value.\nMarkers" );
	if ( SI().zIsTime() )
	    msg += ", logs, T/D and checkshot models";
	else
	    msg += " and logs";
	msg += " are based on the old MD values.\n"
	       "They may therefore become invalid.\n\nContinue?";
	if ( !uiMSG().askGoOn(msg) )
	    return false;
    }

    return true;
}


void uiWellTrackDlg::exportCB( CallBacker* )
{
    if ( !track_.size() )
    {
	uiMSG().message( "No data available to export" );
	return;
    }

    uiFileDialog fdlg( this, false, 0, 0, "File name for export" );
    fdlg.setDirectory( GetDataDir() );
    if ( !fdlg.go() )
	return;

    StreamData sd( StreamProvider(fdlg.fileName()).makeOStream() );
    if ( !sd.usable() )
    {
	uiMSG().error( BufferString( "Cannot open '", fdlg.fileName(),
		    		     "' for write" ) );
	return;
    }

    char buf[70];
    for ( int idx=0; idx<track_.size(); idx++ )
    {
	const Coord3 coord( track_.pos(idx) );
	sprintf( buf, "%16.4lf%16.4lf%10.3lf%10.3f\n", coord.x, coord.y,
		 coord.z, track_.dah( idx ) );
	*sd.ostrm << buf;
    }

    sd.close();
}


// ==================================================================


static const char* t2dcollbls[] = { "Depth (MD)", "Time (ms)", 0 };
#define mD2TModel (cksh_ ? wd_.checkShotModel() : wd_.d2TModel())

uiD2TModelDlg::uiD2TModelDlg( uiParent* p, Well::Data& d, bool cksh )
	: uiDialog(p,uiDialog::Setup("Depth/Time Model",
		 BufferString("Edit ",cksh?"Checkshot":"Time/Depth"," model"),
				     "107.1.5"))
	, wd_(d)
    	, cksh_(cksh)
    	, orgd2t_(mD2TModel ? new Well::D2TModel(*mD2TModel) : 0)
{
    tbl_ = new uiTable( this, uiTable::Setup()
	    			.rowdesc(cksh_ ? "Measure point" : "Control Pt")
				.rowgrow(true).defrowlbl(""), "Table" );
    tbl_->setColumnLabels( t2dcollbls );
    tbl_->setNrRows( nremptyrows );
    tbl_->setPrefWidth( 500 );

    uiGroup* actbutgrp = new uiButtonGroup( this, "Action buttons", false );
    if ( !cksh_ )
	new uiPushButton( actbutgrp, "&Update display",
			  mCB(this,uiD2TModelDlg,updNow), true );

    new uiPushButton( actbutgrp, "&Read new", mCB(this,uiD2TModelDlg,readNew),
		      false );
    new uiPushButton( actbutgrp, "&Export", mCB(this,uiD2TModelDlg,expData),
		      false );
    actbutgrp->attach( leftAlignedBelow, tbl_ );

    unitfld_ = new uiCheckBox( this, " Z in feet" );
    unitfld_->setChecked( SI().depthsInFeetByDefault() );
    unitfld_->activated.notify( mCB(this,uiD2TModelDlg,fillTable) );
    unitfld_->attach( rightAlignedBelow, tbl_ );

    fillTable(0);
}


void uiD2TModelDlg::fillTable( CallBacker* )
{
    const Well::D2TModel* d2t = mD2TModel;
    const int sz = d2t ? d2t->size() : 0;
    if ( !sz ) return;
    tbl_->setNrRows( sz + nremptyrows );

    const float zfac = !unitfld_->isChecked() ? 1 : mToFeetFactor;
    for ( int idx=0; idx<sz; idx++ )
    {
	tbl_->setValue( RowCol(idx,0), d2t->dah(idx) * zfac );
	tbl_->setValue( RowCol(idx,1), d2t->t(idx) * 1000 );
    }
}


uiD2TModelDlg::~uiD2TModelDlg()
{
    delete orgd2t_;
}

class uiD2TModelReadDlg : public uiDialog
{
public:

uiD2TModelReadDlg( uiParent* p, Well::Data& wd, bool cksh )
    	: uiDialog(p,uiDialog::Setup("Read new data",
		    		     "Specify input file","107.1.6"))
	, cksh_(cksh)
	, wd_(wd)
{
    uiD2TModelGroup::Setup su( false );
    su.filefldlbl( "File name" );
    d2tgrp = new uiD2TModelGroup( this, su );
}


bool acceptOK( CallBacker* )
{
    const char* errmsg = d2tgrp->getD2T( wd_, cksh_ );
    if ( errmsg && *errmsg )
	{ uiMSG().error( errmsg ); return false; }

    if ( wd_.d2TModel() )
	wd_.d2TModel()->deInterpolate();

    return true;
}

    uiD2TModelGroup*	d2tgrp;
    Well::Data&		wd_;
    const bool		cksh_;

};


void uiD2TModelDlg::readNew( CallBacker* )
{
    uiD2TModelReadDlg dlg( this, wd_, cksh_ );
    if ( !dlg.go() ) return;

    const BufferString errmsg = dlg.d2tgrp->getD2T( wd_, cksh_ );
    if ( !errmsg.isEmpty() )
	uiMSG().error( errmsg );
    else
    {
	tbl_->clearTable();
	fillTable(0);
	wd_.d2tchanged.trigger();
    }
}


void uiD2TModelDlg::expData( CallBacker* )
{
    Well::D2TModel d2t; getModel( d2t );
    if ( d2t.isEmpty() )
	{ uiMSG().error( "No valid data entered" ); return; }

    uiFileDialog dlg( this, false, 0, 0, "Filename for export" );
    dlg.setDirectory( GetDataDir() );
    if ( !dlg.go() )
	return;

    StreamData sd( StreamProvider(dlg.fileName()).makeOStream() );
    if ( !sd.usable() )
	{ uiMSG().error( BufferString("Cannot open '", dlg.fileName(),
		    			"' for write") ); return; }

    const float zfac = !unitfld_->isChecked() ? 1 : mToFeetFactor;
    for ( int idx=0; idx<d2t.size(); idx++ )
	*sd.ostrm << d2t.dah(idx)*zfac << '\t' << d2t.t(idx)*1000 << '\n';

    sd.close();
}


void uiD2TModelDlg::updNow( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    getModel( *d2t );

    if ( d2t->size() > 1 )
	wd_.d2tchanged.trigger();
    else
	uiMSG().error( "Please define at least two control points." );
}


void uiD2TModelDlg::getModel( Well::D2TModel& d2t )
{
    d2t.erase();
    const float zfac = !unitfld_->isChecked() ? 1 : mToFeetFactor;
    const int nrrows = tbl_->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* sval = tbl_->text( RowCol(idx,0) );
	if ( !sval || !*sval ) continue;
	float dah = mUdf(float);
	if ( !SI().zInFeet() && unitfld_->isChecked() )
	    dah = toFloat(sval) / zfac;
	else
	    dah = toFloat(sval) * zfac;

	if ( mIsUdf(dah) ) continue;

	sval = tbl_->text( RowCol(idx,1) );
	if ( !sval || !*sval ) continue;
	float tm = toFloat(sval) * 0.001;
	d2t.add( dah, tm );
    }
}


bool uiD2TModelDlg::rejectOK( CallBacker* )
{
    Well::D2TModel* d2t = mD2TModel;
    if ( d2t )
	*d2t = *orgd2t_;
    wd_.d2tchanged.trigger();
    return true;
}


bool uiD2TModelDlg::acceptOK( CallBacker* )
{
    updNow( 0 );
    return mD2TModel && mD2TModel->size() > 0;
}



// ==================================================================

static const float defundefval = -999.25;
static const float feetfac = 0.3048;
#ifdef __win__
    static const char* lasfileflt = "Las files (*.las *.dat)";
#else
    static const char* lasfileflt = "Las files (*.las *.LAS *.dat *.DAT)";
#endif


uiLoadLogsDlg::uiLoadLogsDlg( uiParent* p, Well::Data& wd_ )
    : uiDialog(p,uiDialog::Setup("Logs","Define log parameters","107.1.2"))
    , wd(wd_)
{
    lasfld = new uiFileInput( this, "Input (pseudo-)LAS logs file",
			      uiFileInput::Setup(uiFileDialog::Gen)
			      .filter(lasfileflt).withexamine(true) );
    lasfld->valuechanged.notify( mCB(this,uiLoadLogsDlg,lasSel) );

    intvfld = new uiGenInput( this, "Depth interval to load (empty=all)",
			      FloatInpIntervalSpec(false) );
    intvfld->attach( alignedBelow, lasfld );
    BoolInpSpec mft( !SI().depthsInFeetByDefault(), "Meter", "Feet" );
    intvunfld = new uiGenInput( this, "", mft );
    intvunfld->attach( rightOf, intvfld );

    unitlbl = new uiLabel( this, "XXXX" );
    unitlbl->attach( rightOf, intvfld );
    unitlbl->display( false );

    istvdfld = new uiGenInput( this, "Depth values are",
	    			BoolInpSpec(false,"TVDSS","MD") );
    istvdfld->attach( alignedBelow, intvfld );

    udffld = new uiGenInput( this, "Undefined value in logs",
                    FloatInpSpec(defundefval));
    udffld->attach( alignedBelow, istvdfld );

    logsfld = new uiLabeledListBox( this, "Select logs", true );
    logsfld->attach( alignedBelow, udffld );
}


void uiLoadLogsDlg::lasSel( CallBacker* )
{
    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) return;

    Well::Data wd_; Well::LASImporter wdai( wd_ );
    Well::LASImporter::FileInfo lfi;
    const char* res = wdai.getLogInfo( lasfnm, lfi );
    if ( res ) { uiMSG().error( res ); return; }

    logsfld->box()->setEmpty();
    logsfld->box()->addItems( lfi.lognms );
    logsfld->box()->selectAll( true );

    BufferString lbl( "(" ); lbl += lfi.zunitstr.buf(); lbl += ")";
    unitlbl->setText( lbl );
    unitlbl->display( true );
    bool isft = *lfi.zunitstr.buf() == 'f' || *lfi.zunitstr.buf() == 'F';
    intvunfld->setValue( !isft );
    intvunfld->display( false );

    udffld->setValue( lfi.undefval );
    if ( isft )
    {
	if ( !mIsUdf(lfi.zrg.start) ) lfi.zrg.start /= feetfac;
	if ( !mIsUdf(lfi.zrg.stop) ) lfi.zrg.stop /= feetfac;
    }
    intvfld->setValue( lfi.zrg );
}


bool uiLoadLogsDlg::acceptOK( CallBacker* )
{
    Well::LASImporter wdai( wd );
    Well::LASImporter::FileInfo lfi;

    lfi.undefval = udffld->getfValue();
    lfi.zrg.setFrom( intvfld->getFInterval() );
    const bool zinft = !intvunfld->getBoolValue();
    if ( zinft )
    {
	if ( !mIsUdf(lfi.zrg.start) ) lfi.zrg.start *= feetfac;
	if ( !mIsUdf(lfi.zrg.stop) ) lfi.zrg.stop *= feetfac;
    }

    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) 
	{ uiMSG().error("Enter valid filename"); return false; }

    BufferStringSet lognms; 	
    for ( int idx=0; idx<logsfld->box()->size(); idx++ )
    {
	if ( logsfld->box()->isSelected(idx) )
	    lognms += new BufferString( logsfld->box()->textOfItem(idx) );
    }

    BufferString existlogmsg; 
    for ( int idx=lognms.size()-1; idx>=0; idx-- )
    { 
	if ( wd.logs().getLog( lognms.get( idx ) ) )
	{
	    if ( !existlogmsg.isEmpty() ) existlogmsg += ", "; 
	    existlogmsg += lognms.get( idx ); 
	    lognms.remove( idx );
	}
    }
    if ( !existlogmsg.isEmpty() )
    {
	existlogmsg += " already exist(s) and will not be loaded.\n\n";
	existlogmsg += "Please remove them from the existing logs before import.";
	uiMSG().warning( existlogmsg );
    }
    else if ( lognms.isEmpty() )
	{ uiMSG().error("Please select at least one log"); return false; }


    lfi.lognms = lognms;
    const char* res = wdai.getLogs( lasfnm, lfi, istvdfld->getBoolValue() );
    if ( res ) { uiMSG().error( res ); return false; }

    return true;
}


// ==================================================================


static const char* exptypes[] =
{
    "MD/Value",
    "XYZ/Value",
    "ICZ/Value",
    0
};


uiExportLogs::uiExportLogs( uiParent* p, const ObjectSet<Well::Data>& wds, 
			  const BufferStringSet& logsel )
    : uiDialog(p,uiDialog::Setup("Export Well logs",
				 "Specify format","107.1.3"))
    , wds_(wds)
    , logsel_(logsel)
    , multiwellsnamefld_(0)
{
    const bool zinft = SI().depthsInFeetByDefault();
    BufferString lbl( "Depth range " ); lbl += zinft ? "(ft)" : "(m)";
    zrangefld_ = new uiGenInput( this, lbl, FloatInpIntervalSpec(true) );
    setDefaultRange( zinft );

    typefld_ = new uiGenInput( this, "Output format", 
	    		      StringListInpSpec(exptypes) );
    typefld_->valuechanged.notify( mCB(this,uiExportLogs,typeSel) );
    typefld_->attach( alignedBelow, zrangefld_ );

    zunitgrp_ = new uiButtonGroup( this, "", false );
    zunitgrp_->attach( alignedBelow, typefld_ );
    uiLabel* zlbl = new uiLabel( this, "Output Z-unit" );
    zlbl->attach( leftOf, zunitgrp_ );
    new uiRadioButton( zunitgrp_, "meter" );
    new uiRadioButton( zunitgrp_, "feet" );
    bool have2dtmodel = true;
    for ( int idwell=0; idwell<wds_.size(); idwell++ )
    {
	if ( !wds_[idwell]->haveD2TModel() ) 
	{ have2dtmodel = false; break; }
    }
    if ( SI().zIsTime() && have2dtmodel)
    {
	new uiRadioButton( zunitgrp_, "sec" );
	new uiRadioButton( zunitgrp_, "msec" );
    }
    zunitgrp_->selectButton( zinft );

    const bool multiwells = wds.size() > 1;
    outfld_ = new uiFileInput( this, multiwells ? "File Directory" 
	    					: "Output file",
			      uiFileInput::Setup().forread(false)
			      			  .directories(multiwells) );
    outfld_->attach( alignedBelow, zunitgrp_ );
    if ( multiwells )
    {
	outfld_->setFileName( IOM().rootDir() );
	multiwellsnamefld_ = new uiGenInput( this, "File name suffix" );
	multiwellsnamefld_->attach( alignedBelow, outfld_ );
	multiwellsnamefld_->setText( "logs.txt" );
    }

    typeSel(0);
}


void uiExportLogs::setDefaultRange( bool zinft )
{
    StepInterval<float> dahintv;
    for ( int idwell=0; idwell<wds_.size(); idwell++ )
    {
	const Well::Data& wd = *wds_[idwell];
	for ( int idx=0; idx<wd.logs().size(); idx++ )
	{
	    const Well::Log& log = wd.logs().getLog(idx);
	    const int logsz = log.size();
	    if ( !logsz ) continue;

	    dahintv.include( wd.logs().dahInterval() );
	    const float width = log.dah(logsz-1) - log.dah(0);
	    dahintv.step = width / (logsz-1);
	    break;
	}
    }

    if ( zinft )
    {
	dahintv.start *= mToFeetFactor;
	dahintv.stop *= mToFeetFactor;
	dahintv.step *= mToFeetFactor;
    }

    zrangefld_->setValue( dahintv );
}


void uiExportLogs::typeSel( CallBacker* )
{
    zunitgrp_->setSensitive( 2, typefld_->getIntValue() );
    zunitgrp_->setSensitive( 3, typefld_->getIntValue() );
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiExportLogs::acceptOK( CallBacker* )
{
    BufferString fname = outfld_->fileName();
    if ( fname.isEmpty() )
	 mErrRet( "Please select valid entry for the output" );

    BufferStringSet fnames;
    if ( wds_.size() > 1 )
    {
	if ( !File::isDirectory(fname) )
	    mErrRet( "Please enter a valid (existing) location" )
	BufferString suffix = multiwellsnamefld_->text();
	if ( suffix.isEmpty() )
	    mErrRet( "Please enter a valid file name" )

	for ( int idx=0; idx<wds_.size(); idx++ )
	{
	    BufferString nm( fname ); 
	    nm += "/"; nm += wds_[idx]->name(); nm += "_"; nm += suffix;
	    fnames.add( nm );
	}
    }
    else
	fnames.add( fname );

    for ( int idx=0; idx<fnames.size(); idx++ )
    {
	StreamData sdo = StreamProvider( fnames.get(idx) ).makeOStream();
	if ( !sdo.usable() )
	{
	    sdo.close();
	    mErrRet( "Cannot open output file" )
	}
	writeHeader( sdo, *wds_[idx] );
	writeLogs( sdo, *wds_[idx] );
	sdo.close();
    }
    return true;
}


void uiExportLogs::writeHeader( StreamData& sdo, const Well::Data& wd )
{
    const char* units[] = { "(m)", "(ft)", "(s)", "(ms)", 0 };
    
    if ( typefld_->getIntValue() == 1 )
	*sdo.ostrm << "X\tY\t";
    else if ( typefld_->getIntValue() == 2 )
	*sdo.ostrm << "Inline\tCrossline\t";

    const int unitid = zunitgrp_->selectedId();
    BufferString zstr( unitid<2 ? "Depth" : "Time" );
    *sdo.ostrm << zstr << units[unitid];

    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
	const Well::Log& log = wd.logs().getLog(idx);
	if ( !logsel_.isPresent( log.name() ) ) continue;
	BufferString lognm( log.name() );
	cleanupString( lognm.buf(), 0, 0, 0 );
	replaceCharacter( lognm.buf(), '+', '_' );
	replaceCharacter( lognm.buf(), '-', '_' );
	*sdo.ostrm << "\t" << lognm;
	if ( *log.unitMeasLabel() )
	    *sdo.ostrm << "(" << log.unitMeasLabel() << ")";
    }
    
    *sdo.ostrm << '\n';
}


void uiExportLogs::writeLogs( StreamData& sdo, const Well::Data& wd )
{
    bool infeet = zunitgrp_->selectedId() == 1;
    bool insec = zunitgrp_->selectedId() == 2;
    bool inmsec = zunitgrp_->selectedId() == 3;

    const bool zinft = SI().depthsInFeetByDefault();
    const int outtypesel = typefld_->getIntValue();
    const bool dobinid = outtypesel == 2;
    const StepInterval<float> intv = zrangefld_->getFStepInterval();
    const int nrsteps = intv.nrSteps();

    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = intv.atIndex( idx );
	if ( zinft ) md *= mFromFeetFactor;
	if ( outtypesel == 0 )
	{
	    const float mdout = infeet ? md*mToFeetFactor : md;
	    *sdo.ostrm << mdout;
	}
	else
	{
	    const Coord3 pos = wd.track().getPos( md );
	    if ( !pos.x && !pos.y && !pos.z ) continue;

	    if ( dobinid )
	    {
		const BinID bid = SI().transform( pos );
		*sdo.ostrm << bid.inl << '\t' << bid.crl;
	    }
	    else
	    {
		char str[255];
		getStringFromDouble( 0, pos.x, str );
		*sdo.ostrm << str << '\t';
		getStringFromDouble( 0, pos.y, str );
		*sdo.ostrm << str;
	    }

	    float z = pos.z;
	    if ( infeet ) z *= mToFeetFactor;
	    else if ( insec ) z = wd.d2TModel()->getTime( md );
	    else if ( inmsec ) z = wd.d2TModel()->getTime( md ) * 1000;
	    *sdo.ostrm << '\t' << z;
	}

	for ( int logidx=0; logidx<wd.logs().size(); logidx++ )
	{
	    const Well::Log& log = wd.logs().getLog( logidx );
	    if ( !logsel_.isPresent( log.name() ) ) continue;
	    const float val = log.getValue( md );
	    if ( mIsUdf(val) )
		*sdo.ostrm << '\t' << "1e30";
	    else
		*sdo.ostrm << '\t' << val;
	}
	*sdo.ostrm << '\n';
    }
}


//============================================================================

uiNewWellDlg::uiNewWellDlg( uiParent* p )
        : uiGetObjectName(p,uiGetObjectName::Setup("New Well",mkWellNms())
	       			.inptxt("New well name") )
{
    colsel_ = new uiColorInput( this, uiColorInput::Setup(getRandStdDrawColor())
	    			      .lbltxt("Color") );
    colsel_->attach( alignedBelow, inpFld() );
}


uiNewWellDlg::~uiNewWellDlg()
{
    delete nms_;
}


const BufferStringSet& uiNewWellDlg::mkWellNms()
{
    nms_ = new BufferStringSet;
    IOObjContext ctxt( WellTranslatorGroup::ioContext() );
    IOM().to( ctxt.getSelKey() );
    IODirEntryList del( IOM().dirPtr(), ctxt );
    for ( int idx=0; idx<del.size(); idx++ )
    {
	const IOObj* ioobj = del[idx]->ioobj;
	if ( ioobj )
	    nms_->add( ioobj->name() );
    }
    return *nms_;
}


bool uiNewWellDlg::acceptOK( CallBacker* )
{
    BufferString tmp( text() );
    char* ptr = tmp.buf();
    mTrimBlanks(ptr);
    if ( !*ptr )
	mErrRet( "Please enter a name" )

    if ( nms_->indexOf(ptr) >= 0 )
	mErrRet( "Please specify a new name.\n"
		 "Wells can be removed in 'Manage wells'" )

    name_ = ptr;
    return true;
}


const Color& uiNewWellDlg::getWellColor()
{
    return colsel_->color();
}


uiWellLogUOMDlg::uiWellLogUOMDlg( uiParent* p, Well::Log& wl )
    : uiDialog(p,uiDialog::Setup("",mNoDlgTitle,mNoHelpID))
    , log_(wl)			
{
    BufferString ttl( "Edit unit of measure :" );
    ttl += wl.name();
    setCaption( ttl.buf() );
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Unit of measure" );
    unfld_ = lcb->box();
    const ObjectSet<const UnitOfMeasure>& uns( UoMR().all() );
    unfld_->addItem( "-" );
    for ( int idx=0; idx<uns.size(); idx++ )
	unfld_->addItem( uns[idx]->name() );

    const char* curruom = log_.unitMeasLabel();
    const UnitOfMeasure* uom = UnitOfMeasure::getGuessed( curruom );
    unfld_->setCurrentItem( uom ? uom->name() : 0 );
}


bool uiWellLogUOMDlg::acceptOK( CallBacker* )
{
    BufferString uiunit = unfld_->text();
    BufferString curlogunit = log_.unitMeasLabel();

    if ( uiunit.isEqual( "-" ) )
	curlogunit.setEmpty();

    const UnitOfMeasure* newuom = UnitOfMeasure::getGuessed( uiunit );
    if ( newuom ) curlogunit = newuom->symbol();

    log_.setUnitMeasLabel( curlogunit );

    return true;
}

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.cc,v 1.26 2004-11-09 13:13:58 nanne Exp $
________________________________________________________________________

-*/

#include "uiwelldlgs.h"
#include "uilistbox.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uitable.h"
#include "uicolor.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "wellimpasc.h"
#include "welldata.h"
#include "wellmarker.h"
#include "welllog.h"
#include "welllogset.h"
#include "welltrack.h"
#include "welld2tmodel.h"
#include "uiwellpartserv.h"
#include "survinfo.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "strmdata.h"
#include "strmprov.h"
#include "filegen.h"


static const char* mrkrcollbls[] = { "Name", "Depth (MD)", "Color", 0 };
static const char* t2dcollbls[] = { "Depth (MD)", "Time (ms)", 0 };

static const int maxrowsondisplay = 10;
static const int initnrrows = 5;
#define mFromFeetFac 0.3048


uiMarkerDlg::uiMarkerDlg( uiParent* p, const Well::Track& t )
	: uiDialog(p,uiDialog::Setup("Well Markers",
				     "Define marker properties",
				     "107.1.1"))
    	, track(t)
{
    table = new uiTable( this, uiTable::Setup().rowdesc("Marker")
	    				       .rowcangrow() 
					       .defrowlbl() );
    table->setColumnLabels( mrkrcollbls );
    table->setColumnReadOnly( 2, true );
    table->setNrRows( initnrrows );
    table->leftClicked.notify( mCB(this,uiMarkerDlg,mouseClick) );

    BoolInpSpec mft( "Meter", "Feet", !SI().depthsInFeetByDefault() );
    unitfld = new uiGenInput( this, "Depth unit", mft );
    unitfld->attach( leftAlignedBelow, table );

    uiButton* rfbut = new uiPushButton( this, "Read file ...",
	    				mCB(this,uiMarkerDlg,rdFile) );
    rfbut->attach( rightTo, unitfld ); rfbut->attach( rightBorder );
}


int uiMarkerDlg::getNrRows() const
{
    for ( int idx=table->nrRows()-1; idx>=0; idx-- )
    {
	const char* name = table->text( uiTable::RowCol(idx,0) );
	if ( name && *name ) return idx+1;
    }
    return 0;
}


void uiMarkerDlg::mouseClick( CallBacker* )
{
    uiTable::RowCol rc = table->notifiedCell();
    if ( rc.col != 2 ) return;

    Color newcol = table->getColor( rc );
    if ( selectColor(newcol,this,"Marker color") )
	table->setColor( rc, newcol );
}


void uiMarkerDlg::setMarkerSet( const ObjectSet<Well::Marker>& markers,
				bool add )
{
    const int nrnew = markers.size();
    if ( !nrnew ) return;

    const float zfac = unitfld->getBoolValue() ? 1 : 1./mFromFeetFac;
    int startrow = add ? getNrRows() : 0;
    const int nrrows = nrnew + initnrrows + startrow;
    table->setNrRows( nrrows );

    for ( int idx=0; idx<nrnew; idx++ )
    {
	int irow = startrow + idx;
	const Well::Marker* marker = markers[idx];
	table->setText( uiTable::RowCol(irow,0), marker->name() );
	table->setValue( uiTable::RowCol(irow,1), marker->dah*zfac );
	table->setColor( uiTable::RowCol(irow,2), marker->color );
    }
    Well::Marker mrk;
    for ( int irow=startrow+nrnew; irow<nrrows; irow++ )
    {
	table->setText( uiTable::RowCol(irow,0), "" );
	table->setText( uiTable::RowCol(irow,1), "" );
	table->setColor( uiTable::RowCol(irow,2), mrk.color );
    }
}


class uiReadMarkerFile : public uiDialog
{
public:

uiReadMarkerFile::uiReadMarkerFile( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Import Markers",
				 "Specify Marker import",
				 "107.1.4"))
{
    fnmfld = new uiFileInput( this, "Input Ascii file",
	    		uiFileInput::Setup().filter("*;;*.dat;;*.txt")
					    .forread(true));
    fnmfld->setDefaultSelectionDir(
	    IOObjContext::getDataDirName(IOObjContext::WllInf) );
    istvdfld = new uiGenInput( this, "Depth (col 1) is",
	    			BoolInpSpec("TVDSS","MD") );
    istvdfld->attach( alignedBelow, fnmfld );
    replfld = new uiGenInput( this, "Replace current markers", BoolInpSpec() );
    replfld->attach( alignedBelow, istvdfld );
}

bool acceptOK( CallBacker* )
{
    fnm = fnmfld->fileName();
    if ( File_isEmpty(fnm) )
	{ uiMSG().error( "Invalid input file" ); return false; }
    istvd = istvdfld->getBoolValue();
    repl = replfld->getBoolValue();
    return true;
}

    BufferString	fnm;
    bool		istvd;
    bool		repl;
    uiFileInput*	fnmfld;
    uiGenInput*		istvdfld;
    uiGenInput*		replfld;

};


void uiMarkerDlg::rdFile( CallBacker* )
{
    uiReadMarkerFile dlg( this );
    if ( !dlg.go() ) return;
    Well::Data wd; wd.track() = track;
    Well::AscImporter wdai( wd );
    const bool inft = !unitfld->getBoolValue();
    const char* res = wdai.getMarkers( dlg.fnm, dlg.istvd, inft );
    if ( res && *res )
	uiMSG().error( res );
    else
	setMarkerSet( wd.markers(), !dlg.repl );
}


void uiMarkerDlg::getMarkerSet( ObjectSet<Well::Marker>& markers ) const
{
    deepErase( markers );

    const float zfac = unitfld->getBoolValue() ? 1 : mFromFeetFac;
    const int nrrows = getNrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* name = table->text( uiTable::RowCol(idx,0) );
	if ( !name || !*name ) continue;

	Well::Marker* marker = new Well::Marker( name );
	marker->dah = table->getfValue( uiTable::RowCol(idx,1) ) * zfac;
	marker->color = table->getColor( uiTable::RowCol(idx,2) );
	markers += marker;
    }
}


bool uiMarkerDlg::acceptOK( CallBacker* )
{
    SI().pars().setYN( SurveyInfo::sKeyDpthInFt, !unitfld->getBoolValue() );
    SI().savePars();
    return true;
}



// ==================================================================


uiD2TModelDlg::uiD2TModelDlg( uiParent* p, Well::Data& d )
	: uiDialog(p,uiDialog::Setup("Depth/Time Model",
				     "Edit velocity model",
				     "107.1.4"))
	, wd(d)
    	, d2t(*d.d2TModel())
    	, orgd2t(new Well::D2TModel(*d.d2TModel()))
{
    table = new uiTable( this, uiTable::Setup().rowdesc("Control Pt")
	    				       .rowcangrow() 
					       .defrowlbl(), "Table" );
    table->setColumnLabels( t2dcollbls );
    table->setNrRows( initnrrows );

    uiButton* updnowbut = new uiPushButton( this, "Update display",
	    				    mCB(this,uiD2TModelDlg,updNow) );
    updnowbut->attach( centeredBelow, table );

    BoolInpSpec mft( "Meter", "Feet", !SI().depthsInFeetByDefault() );
    unitfld = new uiGenInput( this, "Depth unit", mft );
    unitfld->attach( leftAlignedBelow, table );
    unitfld->attach( ensureBelow, updnowbut );

    const int sz = d2t.size();
    if ( !sz ) return;

    int nrrows = sz + initnrrows;
    table->setNrRows( nrrows );
    const float zfac = unitfld->getBoolValue() ? 1 : 1./mFromFeetFac;
    for ( int idx=0; idx<sz; idx++ )
    {
	table->setValue( uiTable::RowCol(idx,0), d2t.dah(idx) * zfac );
	table->setValue( uiTable::RowCol(idx,1), d2t.t(idx) * 1000 );
    }
}


uiD2TModelDlg::~uiD2TModelDlg()
{
    delete orgd2t;
}


void uiD2TModelDlg::updNow( CallBacker* )
{
    d2t.erase();
    const float zfac = unitfld->getBoolValue() ? 1 : mFromFeetFac;
    const int nrrows = table->nrRows();
    for ( int idx=0; idx<nrrows; idx++ )
    {
	const char* sval = table->text( uiTable::RowCol(idx,0) );
	if ( !sval || !*sval ) continue;
	float dah = atof(sval) * zfac;
	sval = table->text( uiTable::RowCol(idx,1) );
	if ( !sval || !*sval ) continue;
	float tm = atof(sval) * 0.001;
	d2t.add( dah, tm );
    }
    if ( d2t.size() > 1 )
	wd.d2tchanged.trigger();
    else
	uiMSG().error( "Please define at least two control points." );
}


bool uiD2TModelDlg::rejectOK( CallBacker* )
{
    d2t = *orgd2t;
    wd.d2tchanged.trigger();
    return true;
}


bool uiD2TModelDlg::acceptOK( CallBacker* )
{
    SI().pars().setYN( SurveyInfo::sKeyDpthInFt, !unitfld->getBoolValue() );
    SI().savePars();

    updNow( 0 );
    return d2t.size() > 1;
}



// ==================================================================

static const char* lasfilefilt = "*.las;;*.LAS;;*.txt;;*";
static const float defundefval = -999.25;
static const float feetfac = 0.3048;


uiLoadLogsDlg::uiLoadLogsDlg( uiParent* p, Well::Data& wd_ )
    : uiDialog(p,uiDialog::Setup("Logs","Define log parameters","107.1.2"))
    , wd(wd_)
{
    lasfld = new uiFileInput( this, "Input (pseudo-)LAS logs file",
			      uiFileInput::Setup().filter(lasfilefilt)
			      			  .withexamine() );
    lasfld->setDefaultSelectionDir( GetDataDir() );
    lasfld->valuechanged.notify( mCB(this,uiLoadLogsDlg,lasSel) );

    intvfld = new uiGenInput( this, "Depth interval to load (empty=all)",
			      FloatInpIntervalSpec(false) );
    intvfld->attach( alignedBelow, lasfld );
    BoolInpSpec mft( "Meter", "Feet", !SI().depthsInFeetByDefault() );
    intvunfld = new uiGenInput( this, "", mft );
    intvunfld->attach( rightOf, intvfld );

    unitlbl = new uiLabel( this, "XXXX" );
    unitlbl->attach( rightOf, intvfld );
    unitlbl->display( false );

    istvdfld = new uiGenInput( this, "Depth values are",
	    			BoolInpSpec("TVDSS","MD",false) );
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

    Well::Data wd_; Well::AscImporter wdai( wd_ );
    Well::AscImporter::LasFileInfo lfi;
    const char* res = wdai.getLogInfo( lasfnm, lfi );
    if ( res ) { uiMSG().error( res ); return; }

    logsfld->box()->empty();
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
	if ( !mIsUndefined(lfi.zrg.start) ) lfi.zrg.start /= feetfac;
	if ( !mIsUndefined(lfi.zrg.stop) ) lfi.zrg.stop /= feetfac;
    }
    intvfld->setValue( lfi.zrg );
}


bool uiLoadLogsDlg::acceptOK( CallBacker* )
{
    Well::AscImporter wdai( wd );
    Well::AscImporter::LasFileInfo lfi;

    lfi.undefval = udffld->getValue();
    lfi.zrg.start = *intvfld->text(0) ? intvfld->getFInterval().start 
				      : mUndefValue;
    lfi.zrg.stop = *intvfld->text(1) ? intvfld->getFInterval().stop
				     : mUndefValue;
    const bool zinft = !intvunfld->getBoolValue();
    if ( zinft )
    {
	if ( !mIsUndefined(lfi.zrg.start) ) lfi.zrg.start *= feetfac;
	if ( !mIsUndefined(lfi.zrg.stop) ) lfi.zrg.stop *= feetfac;
    }
    SI().pars().setYN( SurveyInfo::sKeyDpthInFt, zinft );
    SI().savePars();

    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) 
	{ uiMSG().error("Enter valid filename"); return false; }

    BufferStringSet lognms;
    for ( int idx=0; idx<logsfld->box()->size(); idx++ )
    {
	if ( logsfld->box()->isSelected(idx) )
	    lognms += new BufferString( logsfld->box()->textOfItem(idx) );
    }
    if ( lognms.size() == 0 )
	{ uiMSG().error("Please select at least one log"); return false; }
    lfi.lognms = lognms;

    const char* res = wdai.getLogs( lasfnm, lfi, istvdfld->getBoolValue() );
    if ( res ) { uiMSG().error( res ); return false; }

    return true;
}


// ==================================================================

uiLogSelDlg::uiLogSelDlg( uiParent* p, const Well::LogSet& logs )
    : uiDialog(p,uiDialog::Setup("Display Well logs",
				 "Select log","107.2.0"))
    , logset(logs)
{
    logsfld = new uiLabeledListBox( this, "Select Log" );
    logsfld->box()->setHSzPol( uiObject::wide );
    logsfld->box()->selectionChanged.notify( mCB(this,uiLogSelDlg,logSel) );
    for ( int idx=0; idx<logs.size(); idx++ )
	logsfld->box()->addItem( logs.getLog(idx).name() );

    rangefld = new uiGenInput( this, "Specify data range", 
	    			FloatInpIntervalSpec() );
    rangefld->attach( alignedBelow, logsfld );

    logscfld = new uiCheckBox( this, "Logarithmic" );
    logscfld->setChecked( false );
    logscfld->attach( rightOf, rangefld );

    uiPushButton* resetbut = new uiPushButton( this, "Reset",
	    				mCB(this,uiLogSelDlg,resetPush));
    resetbut->attach( rightOf, logscfld );

    dispfld = new uiGenInput( this, "Display", BoolInpSpec("Left","Right") );
    dispfld->attach( alignedBelow, rangefld );

    logsfld->box()->setCurrentItem(0);
}


void uiLogSelDlg::resetPush( CallBacker* )
{
    setLogRg( true );
}


void uiLogSelDlg::logSel( CallBacker* )
{
    setLogRg( false );
}


void uiLogSelDlg::setLogRg( bool def )
{
    const int logidx = logsfld->box()->currentItem();
    if ( logidx < 0 || logidx >= logset.size() )
	return;

    const Well::Log& log = logset.getLog(logidx);
    rangefld->setValue( def ? log.valueRange() : log.selValueRange() );
    logscfld->setChecked( def ? false : log.dispLogarithmic() );
}


bool uiLogSelDlg::acceptOK( CallBacker* )
{
    const int logidx = logsfld->box()->currentItem();
    if ( logidx < 0 || logidx >= logset.size() )
	return false;

    Well::Log& log = const_cast<Well::Log&>(logset.getLog(logidx));
    log.setSelValueRange( selRange() );
    log.setDispLogarithmic( scaleLogarithmic() );
    return true;
}


int uiLogSelDlg::logNumber() const
{
    return dispfld->getBoolValue() ? 1 : 2;
}


int uiLogSelDlg::selectedLog() const
{
    return logsfld->box()->currentItem();
}


Interval<float> uiLogSelDlg::selRange() const
{
    return rangefld->getFInterval();
}


bool uiLogSelDlg::scaleLogarithmic() const
{
    return logscfld->isChecked();
}


// ==================================================================


static const char* exptypes[] =
{
    "MD/Value",
    "XYZ/Value",
    "ICZ/Value",
    0
};


uiExportLogs::uiExportLogs( uiParent* p, const Well::Data& wd_, 
			  const BoolTypeSet& sel_ )
    : uiDialog(p,uiDialog::Setup("Export Well logs",
				 "Specify format","107.1.3"))
    , wd(wd_)
    , logsel(sel_)
{
    const bool zinft = SI().depthsInFeetByDefault();
    BufferString lbl( "Depth range " ); lbl += zinft ? "(ft)" : "(m)";
    zrangefld = new uiGenInput( this, lbl, FloatInpIntervalSpec(true) );
    setDefaultRange( zinft );

    typefld = new uiGenInput( this, "Output format", 
	    		      StringListInpSpec(exptypes) );
    typefld->valuechanged.notify( mCB(this,uiExportLogs,typeSel) );
    typefld->attach( alignedBelow, zrangefld );

    zunitgrp = new uiButtonGroup( this, "Output Z-unit", false );
    zunitgrp->attach( alignedBelow, typefld );
    uiRadioButton* meterbut = new uiRadioButton( zunitgrp, "meter" );
    uiRadioButton* feetbut = new uiRadioButton( zunitgrp, "feet" );
    if ( SI().zIsTime() )
    {
	uiRadioButton* secbut = new uiRadioButton( zunitgrp, "sec" );
	uiRadioButton* msecbut = new uiRadioButton( zunitgrp, "msec" );
    }
    zunitgrp->selectButton( zinft );

    outfld = new uiFileInput( this, "Output file",
	   			uiFileInput::Setup().forread(false) );
    outfld->setDefaultSelectionDir(
			IOObjContext::getDataDirName(IOObjContext::WllInf) );
    outfld->attach( alignedBelow, zunitgrp );
    typeSel(0);
}


void uiExportLogs::setDefaultRange( bool zinft )
{
    StepInterval<float> dahintv;
    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
        const Well::Log& log = wd.logs().getLog(idx);
        const int logsz = log.size();
        if ( !logsz ) continue;

        assign( dahintv, wd.logs().dahInterval() );
        const float width = log.dah(logsz-1) - log.dah(0);
        dahintv.step = width / (logsz-1);
	break;
    }

    if ( zinft )
    {
	dahintv.start /= mFromFeetFac;
	dahintv.stop /= mFromFeetFac;
	dahintv.step /= mFromFeetFac;
    }

    zrangefld->setValue( dahintv );
}


void uiExportLogs::typeSel( CallBacker* )
{
    zunitgrp->setSensitive( 2, typefld->getIntValue() );
    zunitgrp->setSensitive( 3, typefld->getIntValue() );
}


#define mErrRet(msg) \
{ uiMSG().error(msg); return false; }

bool uiExportLogs::acceptOK( CallBacker* )
{
    BufferString fname = outfld->fileName();
    if ( !*fname ) mErrRet( "Please select filename" )

    StreamData sdo = StreamProvider( fname ).makeOStream();
    if ( !sdo.usable() )
    {
	sdo.close();
	mErrRet( "Cannot open output file" )
    }

    writeHeader( sdo );
    writeLogs( sdo );
    sdo.close();
    return true;
}


void uiExportLogs::writeHeader( StreamData& sdo )
{
    const char* units[] = { "(m)", "(ft)", "(s)", "(ms)", 0 };
    
    if ( typefld->getIntValue() == 1 )
	*sdo.ostrm << "X\tY\t";
    else if ( typefld->getIntValue() == 2 )
	*sdo.ostrm << "Inline\tCrossline\t";

    const int unitid = zunitgrp->selectedId();
    BufferString zstr( unitid<2 ? "Depth" : "Time" );
    *sdo.ostrm << zstr << units[unitid];
    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
	if ( !logsel[idx] ) continue;
	const Well::Log& log = wd.logs().getLog(idx);
	BufferString lognm( log.name() );
	cleanupString( lognm.buf(), 0, 0, 0 );
	replaceCharacter( lognm.buf(), '+', '_' );
	replaceCharacter( lognm.buf(), '-', '_' );
	*sdo.ostrm << '\t' << lognm;
	if ( *log.unitMeasLabel() )
	    *sdo.ostrm << "(" << log.unitMeasLabel() << ")";
    }
    
    *sdo.ostrm << '\n';
}


void uiExportLogs::writeLogs( StreamData& sdo )
{
    bool inmeter = zunitgrp->selectedId() == 0;
    bool infeet = zunitgrp->selectedId() == 1;
    bool insec = zunitgrp->selectedId() == 2;
    bool inmsec = zunitgrp->selectedId() == 3;

    const bool zinft = SI().depthsInFeetByDefault();

    StepInterval<float> intv = zrangefld->getFStepInterval();
    const int nrsteps = intv.nrSteps();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = intv.atIndex( idx );
	if ( zinft ) md *= mFromFeetFac;
	if ( !typefld->getIntValue() )
	{
	    const float mdout = infeet ? md/mFromFeetFac : md;
	    *sdo.ostrm << mdout;
	}
	else
	{
	    const Coord3 pos = wd.track().getPos( md );
	    if ( !pos.x && !pos.y && !pos.z ) continue;

	    bool dobinid = typefld->getIntValue() == 2;
	    if ( dobinid )
	    {
		const BinID bid = SI().transform( pos );
		*sdo.ostrm << bid.inl << '\t' << bid.crl;
	    }
	    else
		*sdo.ostrm << pos.x << '\t' << pos.y;

	    float z = pos.z;
	    if ( infeet ) z /= mFromFeetFac;
	    else if ( insec ) z = wd.d2TModel()->getTime( md );
	    else if ( inmsec ) z = wd.d2TModel()->getTime( md ) * 1000;
	    *sdo.ostrm << '\t' << z;
	}

	for ( int logidx=0; logidx<wd.logs().size(); logidx++ )
	{
	    if ( !logsel[logidx] ) continue;
	    *sdo.ostrm << '\t' << wd.logs().getLog(logidx).getValue( md );
	}

	*sdo.ostrm << '\n';
    }
}

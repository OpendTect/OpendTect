/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          October 2003
 RCS:           $Id: uiwelldlgs.cc,v 1.13 2004-03-19 14:28:24 nanne Exp $
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


static const char* collbls[] =
{
    "Name", "Depth", "Color", 0
};

static const int maxnrrows = 10;
static const int initnrrows = 5;

uiMarkerDlg::uiMarkerDlg( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Well Markers",
				 "Define marker properties",
				 "107.1.1"))
{
    table = new uiTable( this, uiTable::Setup().rowdesc("Marker")
	    				       .rowcangrow() 
					       .defrowlbl(), "Table" );
    table->setColumnLabels( collbls );
    table->setColumnReadOnly( 2, true );
    table->setNrRows( initnrrows );
    table->leftClicked.notify( mCB(this,uiMarkerDlg,mouseClick) );

    bool zinft = SI().zInFeet();
    SI().pars().getYN( uiWellPartServer::unitstr, zinft );
    unitfld = new uiGenInput( this, "Depth unit", BoolInpSpec("Meter","Feet") );
    unitfld->attach( leftAlignedBelow, table );
    unitfld->setValue( !zinft );
}


void uiMarkerDlg::mouseClick( CallBacker* )
{
    uiTable::RowCol rc = table->notifiedCell();
    if ( rc.col != 2 ) return;

    Color newcol = table->getColor( rc );
    if ( select(newcol,this,"Marker color") )
	table->setColor( rc, newcol );
}


void uiMarkerDlg::setMarkerSet( const ObjectSet<Well::Marker>& markers )
{
    const int nrmarkers = markers.size();
    if ( !nrmarkers ) return;

    const float zfac = unitfld->getBoolValue() ? 1 : 0.3048;
    int nrrows = nrmarkers + initnrrows < maxnrrows ? nrmarkers + initnrrows 
						    : nrmarkers;
    table->setNrRows( nrrows );
    for ( int idx=0; idx<nrmarkers; idx++ )
    {
	const Well::Marker* marker = markers[idx];
	table->setText( uiTable::RowCol(idx,0), marker->name() );
	table->setValue( uiTable::RowCol(idx,1), marker->dah/zfac );
	table->setColor( uiTable::RowCol(idx,2), marker->color );
    }
}


void uiMarkerDlg::getMarkerSet( ObjectSet<Well::Marker>& markers ) const
{
    deepErase( markers );

    const float zfac = unitfld->getBoolValue() ? 1 : 0.3048;
    const int nrrows = table->nrRows();
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
    SI().pars().setYN( uiWellPartServer::unitstr, !unitfld->getBoolValue() );
    SI().savePars();
    return true;
}



// ==================================================================

static const char* lasfilefilt = "*.las;;*.LAS;;*.txt;;*";
static const float defundefval = -999.25;


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

    unitlbl = new uiLabel( this, "XXXX" );
    unitlbl->attach( rightOf, intvfld );
    unitlbl->display( false );

    udffld = new uiGenInput( this, "Undefined value in logs",
                    FloatInpSpec(defundefval));
    udffld->attach( alignedBelow, intvfld );

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

    BufferString lbl( "(" ); lbl += lfi.zunitstr.buf(); lbl += ")";
    unitlbl->setText( lbl );
    unitlbl->display( true );

    udffld->setValue( lfi.undefval );
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

    for ( int idx=0; idx<logsfld->box()->size(); idx++ )
    {
	if ( logsfld->box()->isSelected(idx) )
	    lfi.lognms += new BufferString( logsfld->box()->textOfItem(idx) );
    }

    const char* lasfnm = lasfld->text();
    if ( !lasfnm || !*lasfnm ) 
    { uiMSG().error("Enter valid filename"); return false; }

    const char* res = wdai.getLogs( lasfnm, lfi, false );
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

    autofld = new uiCheckBox( this, "Auto" );
    autofld->setChecked( false );
    autofld->attach( rightTo, rangefld );
    autofld->activated.notify( mCB(this,uiLogSelDlg,logSel) );

    dispfld = new uiGenInput( this, "Display", BoolInpSpec("Left","Right") );
    dispfld->attach( alignedBelow, rangefld );

    logsfld->box()->setCurrentItem(0);
}


void uiLogSelDlg::logSel( CallBacker* )
{
    const int logidx = logsfld->box()->currentItem();
    if ( logidx < 0 || logidx >= logset.size() )
	return;

    const Well::Log& log = logset.getLog(logidx);
    rangefld->setValue( autofld->isChecked() ? log.valueRange() 
	    				     : log.selValueRange() );
}


bool uiLogSelDlg::acceptOK( CallBacker* )
{
    const int logidx = logsfld->box()->currentItem();
    if ( logidx < 0 || logidx >= logset.size() )
	return false;

    Well::Log& log = const_cast<Well::Log&>(logset.getLog(logidx));
    log.setSelValueRange( rangefld->getFInterval() );
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
    bool zinft = SI().zInFeet();
    SI().pars().getYN( uiWellPartServer::unitstr, zinft );
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
	dahintv.start /= 0.3048;
	dahintv.stop /= 0.3048;
	dahintv.step /= 0.3048;
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

    bool zinft = SI().zInFeet();
    SI().pars().getYN( uiWellPartServer::unitstr, zinft );

    StepInterval<float> intv = zrangefld->getFStepInterval();
    const int nrsteps = intv.nrSteps();
    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = intv.atIndex( idx );
	if ( zinft ) md *= 0.3048;
	if ( !typefld->getIntValue() )
	{
	    const float mdout = infeet ? md/0.3048 : md;
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
	    if ( infeet ) z /= 0.3048;
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

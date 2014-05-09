/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          October 2003
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelllogimpexp.h"

#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uiwellsel.h"
#include "uiunitsel.h"

#include "od_iostream.h"
#include "od_helpids.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "ioobj.h"
#include "ioman.h"


static const float defundefval = -999.25;
#ifdef __win__
    static const char* lasfileflt = "Las files (*.las *.dat)";
#else
    static const char* lasfileflt = "Las files (*.las *.LAS *.dat *.DAT)";
#endif


uiImportLogsDlg::uiImportLogsDlg( uiParent* p, const IOObj* ioobj )
    : uiDialog(p,uiDialog::Setup("Import Well Logs",mNoDlgTitle,
				 mODHelpKey(mImportLogsHelpID)))
{
    setOkText( uiStrings::sImport() );

    lasfld_ = new uiFileInput( this, "Input (pseudo-)LAS logs file",
			       uiFileInput::Setup(uiFileDialog::Gen)
			       .filter(lasfileflt).withexamine(true) );
    lasfld_->valuechanged.notify( mCB(this,uiImportLogsDlg,lasSel) );

    intvfld_ = new uiGenInput( this, "Depth interval to load (empty=all)",
			      FloatInpIntervalSpec(false) );
    intvfld_->attach( alignedBelow, lasfld_ );

    BoolInpSpec mft( !SI().depthsInFeet(), "Meter", "Feet" );
    intvunfld_ = new uiGenInput( this, "", mft );
    intvunfld_->attach( rightOf, intvfld_ );
    intvunfld_->display( false );

    unitlbl_ = new uiLabel( this, "XXXX" );
    unitlbl_->attach( rightOf, intvfld_ );
    unitlbl_->display( false );

    istvdfld_ = new uiGenInput( this, "Depth values are",
				BoolInpSpec(false,"TVDSS","MD") );
    istvdfld_->attach( alignedBelow, intvfld_ );

    udffld_ = new uiGenInput( this, "Undefined value in logs",
                    FloatInpSpec(defundefval));
    udffld_->attach( alignedBelow, istvdfld_ );

    uiLabeledListBox* llb = new uiLabeledListBox( this, "Logs to import" );
    logsfld_ = llb->box();
    llb->attach( alignedBelow, udffld_ );
    logsfld_->setMultiChoice( true );

    wellfld_ = new uiWellSel( this, true, "Add to Well" );
    if ( ioobj ) wellfld_->setInput( *ioobj );
    wellfld_->attach( alignedBelow, llb );

}


void uiImportLogsDlg::lasSel( CallBacker* )
{
    const char* lasfnm = lasfld_->text();
    if ( !lasfnm || !*lasfnm ) return;

    Well::Data wd; Well::LASImporter wdai( wd );
    Well::LASImporter::FileInfo lfi;
    const char* res = wdai.getLogInfo( lasfnm, lfi );
    if ( res ) { uiMSG().error( res ); return; }

    logsfld_->setEmpty();
    logsfld_->addItems( lfi.lognms );
    logsfld_->chooseAll( true );

    BufferString lbl( "(" ); lbl += lfi.zunitstr.buf(); lbl += ")";
    unitlbl_->setText( lbl );
    unitlbl_->display( true );
    const bool isft = *lfi.zunitstr.buf() == 'f' || *lfi.zunitstr.buf() == 'F';
    intvunfld_->setValue( !isft );
    intvunfld_->display( false );

    udffld_->setValue( lfi.undefval );

    Interval<float> usrzrg = lfi.zrg;
    if ( isft )
    {
	if ( !mIsUdf(lfi.zrg.start) )
	    usrzrg.start *= mToFeetFactorF;
	if ( !mIsUdf(lfi.zrg.stop) )
	    usrzrg.stop *= mToFeetFactorF;
    }
    intvfld_->setValue( usrzrg );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiImportLogsDlg::acceptOK( CallBacker* )
{
    const IOObj* wellioobj = wellfld_->ioobj();
    if ( !wellioobj ) return false;

    Well::Data wd;
    Well::Reader rdr( wellioobj->fullUserExpr(true), wd );
    if ( !rdr.getLogs() )
	mErrRet( "Cannot read logs for selected well" )

    Well::LASImporter wdai( wd );
    Well::LASImporter::FileInfo lfi;

    lfi.undefval = udffld_->getfValue();

    Interval<float> usrzrg = intvfld_->getFInterval();
    const bool zinft = !intvunfld_->getBoolValue();
    if ( zinft )
    {
	if ( !mIsUdf(usrzrg.start) )
	    usrzrg.start *= mFromFeetFactorF;
	if ( !mIsUdf(usrzrg.stop) )
	    usrzrg.stop *= mFromFeetFactorF;
    }
    lfi.zrg.setFrom( usrzrg );

    const char* lasfnm = lasfld_->text();
    if ( !lasfnm || !*lasfnm )
	mErrRet( "Please enter a valid file name" )

    BufferStringSet lognms; logsfld_->getChosen( lognms );
    if ( lognms.isEmpty() )
	mErrRet( "Please select at least one log to import" )

    BufferStringSet existlogs;
    for ( int idx=lognms.size()-1; idx>=0; idx-- )
    {
	const char* lognm = lognms.get(idx).buf();
	if ( wd.logs().getLog(lognm) )
	{
	    existlogs.add( lognm );
	    lognms.removeSingle( idx );
	}
    }
    const int nrexisting = existlogs.size();
    if ( nrexisting > 0 )
    {
	const bool issingle = nrexisting == 1;
	BufferString msg( "Existing log", issingle ? ":" : "s:\n",
			  existlogs.getDispString() );
	msg.add( "\nalready exist" ).add( issingle ? "" : "s" )
	    .add( " and will not be loaded.\n\nPlease remove " )
	    .add( issingle ? "it" : "these" )
	    .add( " from the existing logs before import." );
	if ( lognms.isEmpty() )
	    mErrRet( msg )
	uiMSG().warning( msg );
    }

    lfi.lognms = lognms;
    const char* res = wdai.getLogs( lasfnm, lfi, istvdfld_->getBoolValue() );
    if ( res )
	mErrRet( res )

    Well::Writer wtr( wellioobj->fullUserExpr(true), wd );
    if ( !wtr.putLogs() )
	mErrRet( "Cannot write logs to disk" )

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
static const float cTWTFac  = 1000.f;


static BufferString getDlgTitle( const ObjectSet<Well::Data>& wds,
				 const BufferStringSet& lognms )
{
    BufferStringSet wllnms; addNames( wds, wllnms );
    const int nrwells = wllnms.size();
    const int nrlogs = lognms.size();
    if ( nrwells < 1 || nrlogs < 1 )
	return BufferString("No wells/logs selected");

    const BufferString wllstxt( wllnms.getDispString(3) );
    const BufferString logstxt( lognms.getDispString(3) );

    BufferString ret;
    if ( nrlogs == 1 )
	ret.set( "Export " ).add( logstxt ).add( " for " ).add( wllstxt );
    else if ( nrwells == 1 )
	ret.set( wllstxt ).add( ": export " ).add( logstxt );
    else
	ret.set( "For " ).add( wllstxt ).add( " export " ).add( logstxt );

    return ret;
}


uiExportLogs::uiExportLogs( uiParent* p, const ObjectSet<Well::Data>& wds,
			  const BufferStringSet& logsel )
    : uiDialog(p,uiDialog::Setup("Export Well logs",getDlgTitle(wds,logsel),
				 mODHelpKey(mExportLogsHelpID)))
    , wds_(wds)
    , logsel_(logsel)
    , multiwellsnamefld_(0)
{
    const bool zinft = SI().depthsInFeet();
    BufferString lbl( "Depth range " ); lbl += zinft ? "(ft)" : "(m)";
    zrangefld_ = new uiGenInput( this, lbl, FloatInpIntervalSpec(true) );
    setDefaultRange( zinft );

    typefld_ = new uiGenInput( this, "Output format",
			      StringListInpSpec(exptypes) );
    typefld_->valuechanged.notify( mCB(this,uiExportLogs,typeSel) );
    typefld_->attach( alignedBelow, zrangefld_ );

    zunitgrp_ = new uiButtonGroup( this, "Z-unit buttons", OD::Horizontal );
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
	dahintv.start *= mToFeetFactorF;
	dahintv.stop *= mToFeetFactorF;
	dahintv.step *= mToFeetFactorF;
    }

    zrangefld_->setValue( dahintv );
}


void uiExportLogs::typeSel( CallBacker* )
{
    zunitgrp_->setSensitive( 2, typefld_->getIntValue() );
    zunitgrp_->setSensitive( 3, typefld_->getIntValue() );
}

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
	const BufferString fnm( fnames.get(idx) );
	od_ostream strm( fnm );
	if ( !strm.isOK() )
	{
	    BufferString msg( "Cannot open output file ", fnm );
	    strm.addErrMsgTo( msg );
	    mErrRet( msg );
	}
	writeHeader( strm, *wds_[idx] );
	writeLogs( strm, *wds_[idx] );
    }
    return true;
}


void uiExportLogs::writeHeader( od_ostream& strm, const Well::Data& wd )
{
    const char* units[] = { "(m)", "(ft)", "(s)", "(ms)", 0 };

    if ( typefld_->getIntValue() == 1 )
	strm << "X\tY\t";
    else if ( typefld_->getIntValue() == 2 )
	strm << "Inline\tCrossline\t";

    const int unitid = zunitgrp_->selectedId();
    BufferString zstr( unitid<2 ? "Depth" : "Time" );
    strm << zstr << units[unitid];

    for ( int idx=0; idx<wd.logs().size(); idx++ )
    {
	const Well::Log& log = wd.logs().getLog(idx);
	if ( !logsel_.isPresent( log.name() ) ) continue;
	BufferString lognm( log.name() );
	lognm.clean();
	lognm.replace( '+', '_' );
	lognm.replace( '-', '_' );
	strm << od_tab << lognm;
	if ( *log.unitMeasLabel() )
	    strm << "(" << log.unitMeasLabel() << ")";
    }

    strm << od_newline;
}


void uiExportLogs::writeLogs( od_ostream& strm, const Well::Data& wd )
{
    const bool infeet = zunitgrp_->selectedId() == 1;
    const bool insec = zunitgrp_->selectedId() == 2;
    const bool inmsec = zunitgrp_->selectedId() == 3;
    const bool intime = insec || inmsec;
    if ( intime && !wd.d2TModel() )
    {
	uiMSG().error( "No depth-time model found, cannot export with time" );
	return;
    }

    const bool zinft = SI().depthsInFeet();
    const int outtypesel = typefld_->getIntValue();
    const bool dobinid = outtypesel == 2;
    const StepInterval<float> intv = zrangefld_->getFStepInterval();
    const int nrsteps = intv.nrSteps();

    for ( int idx=0; idx<nrsteps; idx++ )
    {
	float md = intv.atIndex( idx );
	if ( zinft ) md *= mFromFeetFactorF;
	if ( outtypesel == 0 )
	{
	    const float mdout = infeet ? md*mToFeetFactorF : md;
	    strm << mdout;
	}
	else
	{
	    const Coord3 pos = wd.track().getPos( md );
	    if ( !pos.x && !pos.y && !pos.z ) continue;

	    if ( dobinid )
	    {
		const BinID bid = SI().transform( pos );
		strm << bid.inl() << od_tab << bid.crl();
	    }
	    else
	    {
		strm << pos.x << od_tab; // keep sep from next line
		strm << pos.y;
	    }

	    float z = (float) pos.z;
	    if ( infeet ) z *= mToFeetFactorF;
	    else if (intime )
	    {
		z = wd.d2TModel()->getTime( md, wd.track() );
		if ( inmsec && !mIsUdf(z) ) z *= cTWTFac;
	    }

	    strm << od_tab << z;
	}

	for ( int logidx=0; logidx<wd.logs().size(); logidx++ )
	{
	    const Well::Log& log = wd.logs().getLog( logidx );
	    if ( !logsel_.isPresent( log.name() ) ) continue;
	    const float val = log.getValue( md );
	    if ( mIsUdf(val) )
		strm << od_tab << "1e30";
	    else
		strm << od_tab << val;
	}
	strm << od_newline;
    }
}

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwelllogimpexp.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uistring.h"
#include "uitable.h"
#include "uiunitsel.h"
#include "uiwellsel.h"

#include "oddirs.h"
#include "od_iostream.h"
#include "od_helpids.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "wellman.h"
#include "welltrack.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellreader.h"
#include "wellwriter.h"
#include "ioobj.h"
#include "ioman.h"


static const float defundefval = -999.25;

uiImportLogsDlg::uiImportLogsDlg( uiParent* p, const IOObj* ioobj, bool wtable )
    : uiDialog(p,uiDialog::Setup(tr("Import Well Logs"),mNoDlgTitle,
				 mODHelpKey(mImportLogsHelpID)))
{
    setOkText( uiStrings::sImport() );

    lasfld_ = new uiASCIIFileInput( this, tr("Input (pseudo-)LAS logs file"),
				    true );
    lasfld_->setFilter( Well::LASImporter::fileFilter() );
    lasfld_->valuechanged.notify( mCB(this,uiImportLogsDlg,lasSel) );

    intvfld_ = new uiGenInput( this, tr("Depth interval to load (empty=all)"),
			      FloatInpIntervalSpec(false) );
    intvfld_->attach( alignedBelow, lasfld_ );

    unitlbl_ = new uiLabel( this, tr("XXXX") );
    unitlbl_->attach( rightOf, intvfld_ );
    unitlbl_->display( false );

    istvdfld_ = new uiGenInput( this, tr("Depth values are"),
				BoolInpSpec(false,tr("TVDSS"),tr("MD")) );
    istvdfld_->attach( alignedBelow, intvfld_ );

    udffld_ = new uiGenInput( this, tr("Undefined value in logs"),
				FloatInpSpec(defundefval) );
    udffld_->attach( alignedBelow, istvdfld_ );

    uiObject* attachobj = nullptr;
    if ( wtable )
    {
	uiStringSet colnms;
	colnms.add( uiStrings::sCurve() );
	colnms.add( uiStrings::sUnit() );
	colnms.add( uiStrings::sDescription() );
	logstable_ = new uiTable( this, uiTable::Setup(3,3), "Logs in file" );
	logstable_->setColumnLabels( colnms );
	logstable_->setSelectionMode( uiTable::Multi );
	logstable_->setSelectionBehavior( uiTable::SelectRows );
	logstable_->attach( ensureBelow, udffld_ );

	lognmfld_ = new uiGenInput( this, tr("Name log after"),
		BoolInpSpec(false,tr("Curve"),tr("Description")) );
	lognmfld_->attach( alignedBelow, udffld_ );
	lognmfld_->attach( ensureBelow, logstable_ );
	attachobj = lognmfld_->attachObj();
    }
    else
    {
	uiListBox::Setup su( OD::ChooseAtLeastOne, tr("Logs to import") );
	logsfld_ = new uiListBox( this, su );
	logsfld_->attach( alignedBelow, udffld_ );
	attachobj = logsfld_->attachObj();
    }

    wellfld_ = new uiWellSel( this, true, tr("Add to Well"), false );
    if ( ioobj )
	wellfld_->setInput( *ioobj );

    wellfld_->attach( alignedBelow, attachobj );
}


void uiImportLogsDlg::lasSel( CallBacker* )
{
    const char* lasfnm = lasfld_->text();
    if ( !lasfnm || !*lasfnm ) return;

    RefMan<Well::Data> wd = new Well::Data;
    Well::LASImporter wdai( *wd );
    Well::LASImporter::FileInfo lfi;
    const char* res = wdai.getLogInfo( lasfnm, lfi );
    if ( res ) { uiMSG().error( mToUiStringTodo(res) ); return; }

    if ( logstable_ )
    {
	logstable_->setNrRows( lfi.size() );
	for ( int idx=0; idx<lfi.size(); idx++ )
	{
	    logstable_->setCellChecked( RowCol(idx,0), true );
	    logstable_->setText( RowCol(idx,0), lfi.logcurves_.get(idx) );
	    logstable_->setText( RowCol(idx,1), lfi.logunits_.get(idx) );
	    logstable_->setText( RowCol(idx,2), lfi.lognms_.get(idx) );
	}

	logstable_->setColumnResizeMode( uiTable::ResizeToContents );
	logstable_->setColumnStretchable( 2, true );
    }
    else if ( logsfld_ )
    {
	logsfld_->setEmpty();
	logsfld_->addItems( lfi.lognms_ );
	logsfld_->chooseAll( true );
    }

    const uiString lbl = toUiString("(%1)").arg( lfi.zunitstr_.buf() );
    unitlbl_->setText( lbl );
    unitlbl_->display( true );

    udffld_->setValue( lfi.undefval_ );

    const UnitOfMeasure* uom = UoMR().get( lfi.zunitstr_ );
    Interval<float> usrzrg = lfi.zrg_;
    if ( uom )
    {
	usrzrg.start = uom->userValue( usrzrg.start );
	usrzrg.stop = uom->userValue( usrzrg.stop );
    }

    intvfld_->setValue( usrzrg );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiImportLogsDlg::acceptOK( CallBacker* )
{
    const MultiID wmid = wellfld_->key();
    RefMan<Well::Data> wd = new Well::Data;
    wd = Well::MGR().get( wmid, Well::LoadReqs(Well::LogInfos) );
    if ( !wd )
	mErrRet( mToUiStringTodo(Well::MGR().errMsg()) )

    const char* lasfnm = lasfld_->text();
    if ( !lasfnm || !*lasfnm )
	mErrRet( tr("Please enter a valid file name") )

    Well::LASImporter wdai( *wd );
    Well::LASImporter::FileInfo lfi;
    wdai.getLogInfo( lasfnm, lfi );
    lfi.logcurves_.setEmpty();
    lfi.logunits_.setEmpty();
    lfi.lognms_.setEmpty();

    lfi.undefval_ = udffld_->getFValue();

    const UnitOfMeasure* uom = UoMR().get( lfi.zunitstr_ );
    const Interval<float> usrzrg = intvfld_->getFInterval();
    if ( uom )
    {
	lfi.zrg_.start = uom->internalValue( usrzrg.start );
	lfi.zrg_.stop = uom->internalValue( usrzrg.stop );
    }

    const bool usecurvenms = lognmfld_ ? lognmfld_->getBoolValue() : false;
    BufferStringSet lognms;
    if ( logstable_ )
    {
	const int colidx = usecurvenms ? 0 : 2;
	for ( int idx=0; idx<logstable_->nrRows(); idx++ )
	{
	    if ( logstable_->isCellChecked(RowCol(idx,0)))
		lognms.add( logstable_->text(RowCol(idx,colidx)) );
	}
    }
    else if ( logsfld_ )
	logsfld_->getChosen( lognms );

    if ( lognms.isEmpty() )
	mErrRet( tr("Please select at least one log to import") )

    BufferStringSet existlogs;
    for ( int idx=lognms.size()-1; idx>=0; idx-- )
    {
	const char* lognm = lognms.get(idx).buf();
	if ( wd->logs().getLog(lognm) )
	{
	    existlogs.add( lognm );
	    lognms.removeSingle( idx );
	}
    }

    const int nrexisting = existlogs.size();
    if ( nrexisting > 0 )
    {
	uiString msg = tr("The following logs already exist and will not "
			  "be imported:\n\n%1\n\nPlease remove them before "
			  "import.").arg(existlogs.getDispString());
	if ( lognms.isEmpty() )
	    mErrRet( msg )

	uiMSG().warning( msg );
    }

    lfi.lognms_ = lognms;
    const char* res = wdai.getLogs( lasfnm, lfi, istvdfld_->getBoolValue(),
				    usecurvenms );
    if ( res )
	mErrRet( mToUiStringTodo(res) )

    uiString errmsg = tr("Cannot write following logs to disk");
    bool failed = false;
    Well::Writer wtr( wmid, *wd );
    for ( const auto* lognm : lognms )
    {
	if ( !wtr.putLog(*wd->logs().getLog(lognm->buf())) )
	{
	    errmsg.addMoreInfo( tr("lognm"), true );
	    failed = true;
	}
    }

    if ( failed )
	mErrRet( errmsg )

    uiString msg = tr("Well Log successfully imported."
		      "\n\nDo you want to import more Well Logs?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


// ==================================================================


static const char* exptypes[] =
{
    "MD/Value",
    "XYZ/Value",
    "ICZ/Value",
    nullptr
};

static const float cTWTFac  = 1000.f;


uiString uiExportLogs::getDlgTitle( const ObjectSet<Well::Data>& wds,
				    const BufferStringSet& lognms )
{
    BufferStringSet wllnms;
    addNames( wds, wllnms );
    const int nrwells = wllnms.size();
    const int nrlogs = lognms.size();
    if ( nrwells < 1 || nrlogs < 1 )
	return tr("No wells/logs selected");

    const BufferString wllstxt( wllnms.getDispString(3) );
    const BufferString logstxt( lognms.getDispString(3) );

    BufferString ret;
    if ( nrlogs == 1 )
	return tr("Export %1 for %2").arg( logstxt ).arg( wllstxt );
    else if ( nrwells == 1 )
	return tr( "%1: export %2" ).arg( wllstxt ).arg( logstxt );

    return tr( "For %1 export %2" ).arg( wllstxt ).arg( logstxt );
}



uiExportLogs::uiExportLogs( uiParent* p, const ObjectSet<Well::Data>& wds,
			  const BufferStringSet& logsel )
    : uiDialog(p,uiDialog::Setup( uiStrings::phrExport( uiStrings::sWellLog() ),
				  getDlgTitle(wds,logsel),
				  mODHelpKey(mExportLogsHelpID)))
    , wds_(wds)
    , logsel_(logsel)
    , multiwellsnamefld_(nullptr)
    , coordsysselfld_(nullptr)
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sCancel() );

    const bool zinft = SI().depthsInFeet();
    const uiString lbl = tr( "Depth range %1" ).
	arg( uiStrings::sDistUnitString( zinft, true, true) );
    zrangefld_ = new uiGenInput( this, lbl, FloatInpIntervalSpec(true) );
    setDefaultRange( zinft );

    typefld_ = new uiGenInput( this, uiStrings::sFormat(),
			      StringListInpSpec(exptypes) );
    typefld_->valuechanged.notify( mCB(this,uiExportLogs,typeSel) );
    typefld_->attach( alignedBelow, zrangefld_ );

    uiObject* attachobj = typefld_->attachObj();
    if ( SI().hasProjection() )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, attachobj );
	attachobj = coordsysselfld_->attachObj();
    }

    zunitgrp_ = new uiButtonGroup( this, "Z-unit buttons", OD::Horizontal );
    zunitgrp_->attach( alignedBelow, attachobj );
    uiLabel* zlbl = new uiLabel( this,
				 uiStrings::phrOutput( uiStrings::sZUnit() ));
    zlbl->attach( leftOf, zunitgrp_ );
    new uiRadioButton( zunitgrp_,
		       uiStrings::sDistUnitString( false, false, false ) );
    new uiRadioButton( zunitgrp_,
		      uiStrings::sDistUnitString( true, false, false ) );
    bool have2dtmodel = true;
    for ( int idwell=0; idwell<wds_.size(); idwell++ )
    {
	if ( !wds_[idwell]->haveD2TModel() )
	{
	    have2dtmodel = false;
	    break;
	}
    }

    if ( SI().zIsTime() && have2dtmodel)
    {
	new uiRadioButton( zunitgrp_, uiStrings::sSec().toLower() );
	new uiRadioButton( zunitgrp_, uiStrings::sMsec().toLower() );
    }

    zunitgrp_->selectButton( zinft );

    const bool multiwells = wds.size() > 1;
    outfld_ = new uiFileInput( this,
			multiwells ? tr("Output folder") : tr("Output file"),
			uiFileInput::Setup().forread(false)
					.directories(multiwells)
					.defseldir(GetSurveyExportDir()) );
    outfld_->attach( alignedBelow, zunitgrp_ );
    if ( multiwells )
    {
	outfld_->setFileName( GetSurveyExportDir() );
	multiwellsnamefld_ = new uiGenInput( this, tr("File name suffix") );
	multiwellsnamefld_->attach( alignedBelow, outfld_ );
	multiwellsnamefld_->setText( "logs.dat" );
    }

    typeSel( nullptr );
}


uiExportLogs::~uiExportLogs()
{
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
	    if ( logsz==0 )
		continue;

	    dahintv.include( wd.logs().dahInterval() );
	    const float width = log.dah(logsz-1) - log.dah(0);
	    dahintv.step = width / (logsz-1);
	    break;
	}
    }

    StepInterval<float> disprg = dahintv;
    const UnitOfMeasure* storunit = UnitOfMeasure::surveyDefDepthStorageUnit();
    const UnitOfMeasure* outunit = UnitOfMeasure::surveyDefDepthUnit();
    disprg.start = getConvertedValue( dahintv.start, storunit, outunit );
    disprg.stop = getConvertedValue( dahintv.stop, storunit, outunit );
    disprg.step = getConvertedValue( dahintv.step, storunit, outunit );

    zrangefld_->setValue( disprg );
}


void uiExportLogs::typeSel( CallBacker* )
{
    zunitgrp_->setSensitive( 2, typefld_->getIntValue() );
    zunitgrp_->setSensitive( 3, typefld_->getIntValue() );
    if ( coordsysselfld_ )
	coordsysselfld_->display( typefld_->getIntValue() == 1 );
}


bool uiExportLogs::acceptOK( CallBacker* )
{
    BufferString fname = outfld_->fileName();
    if ( fname.isEmpty() )
	 mErrRet( tr("Please select valid entry for the output") );

    BufferStringSet fnames;
    if ( wds_.size() > 1 )
    {
	if ( !File::isDirectory(fname) )
	    mErrRet( tr("Please enter a valid (existing) location") )
	BufferString suffix = multiwellsnamefld_->text();
	if ( suffix.isEmpty() )
	    mErrRet( tr("Please enter a valid file name") )

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
	    uiString msg = tr("Cannot open output file %1").arg(fnm);
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
	if ( !logsel_.isPresent(log.name()) )
	    continue;

	BufferString lognm( log.name() );
	lognm.clean();
	lognm.replace( '+', '_' );
	lognm.replace( '-', '_' );
	strm << od_tab << lognm;
	if ( log.haveUnit() )
	    strm << "(" << log.unitMeasLabel() << ")";
    }

    strm << od_newline;
}


void uiExportLogs::writeLogs( od_ostream& strm, const Well::Data& wd )
{
    const bool outinm = zunitgrp_->selectedId() == 0;
    const bool outinft = zunitgrp_->selectedId() == 1;
    const bool outinsec = zunitgrp_->selectedId() == 2;
    const bool outinmsec = zunitgrp_->selectedId() == 3;
    const bool outindepth = outinm || outinft;
    const bool outintime = outinsec || outinmsec;
    if ( outintime && !wd.d2TModel() )
    {
	uiMSG().error( tr("No depth-time model found, "
			  "cannot export with time") );
	return;
    }

    const int outtypesel = typefld_->getIntValue();
    const bool dobinid = outtypesel == 2;
    const StepInterval<float> intv = zrangefld_->getFStepInterval();
    const int nrsteps = intv.nrSteps();

    const UnitOfMeasure* storunit = UnitOfMeasure::surveyDefDepthStorageUnit();
    const UnitOfMeasure* userunit = UnitOfMeasure::surveyDefDepthUnit();
    const UnitOfMeasure* outunit =
	outinft ? UoMR().get( "Feet" ) : UoMR().get( "Meter" );

    const Coords::CoordSystem* outcrs =
	coordsysselfld_ ? coordsysselfld_->getCoordSystem() : nullptr;
    const Coords::CoordSystem* syscrs = SI().getCoordSystem();
    const bool needsconversion = outcrs && !(*outcrs == *syscrs);

    for ( int idx=0; idx<nrsteps; idx++ )
    {
	const float md = intv.atIndex( idx );
	const float mdstor = getConvertedValue( md, userunit, storunit );
	if ( outtypesel == 0 )
	{
	    const float mdout = getConvertedValue( md, userunit, outunit );
	    strm << mdout;
	}
	else
	{
	    const Coord3 pos = wd.track().getPos( mdstor );
	    if ( !pos.x && !pos.y && !pos.z )
		continue;

	    if ( dobinid )
	    {
		const BinID bid = SI().transform( pos );
		strm << bid.inl() << od_tab << bid.crl();
	    }
	    else
	    {
		Coord convcoord;
		if ( needsconversion )
		    convcoord = outcrs->convertFrom( pos.coord(), *syscrs );
		strm << convcoord.x << od_tab; // keep sep from next line
		strm << convcoord.y;
	    }

	    float z = float(pos.z);
	    if ( outindepth )
		z = getConvertedValue( z, storunit, outunit );
	    else if ( outintime )
	    {
		z = wd.d2TModel()->getTime( mdstor, wd.track() );
		if ( outinmsec && !mIsUdf(z) )
		    z *= cTWTFac;
	    }

	    strm << od_tab << z;
	}

	for ( int logidx=0; logidx<wd.logs().size(); logidx++ )
	{
	    const Well::Log& log = wd.logs().getLog( logidx );
	    if ( !logsel_.isPresent(log.name()) )
		continue;

	    const float val = log.getValue( mdstor );
	    if ( mIsUdf(val) )
		strm << od_tab << mUdf(float);
	    else
		strm << od_tab << val;
	}

	strm << od_newline;
    }
}

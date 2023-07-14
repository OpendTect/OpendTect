/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uilaswriter.h"

#include "uicombobox.h"
#include "uifileinput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiwellsel.h"

#include "filepath.h"
#include "ioobj.h"
#include "laswriter.h"
#include "oddirs.h"
#include "welldata.h"
#include "wellimpasc.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "wellreader.h"


uiLASWriter::uiLASWriter( uiParent* p )
    : uiDialog(p,Setup(tr("Export to LAS"),mNoDlgTitle,
		 mODHelpKey(mLASWriterHelpID)))
{
    setOkText( uiStrings::sExport() );

    wellfld_ = new uiWellSel( this, true );
    mAttachCB( wellfld_->selectionDone, uiLASWriter::wellSelCB );

    uiListBox::Setup lbsu( OD::ChooseAtLeastOne, tr("Select Log(s)") );
    logsfld_ = new uiListBox( this, lbsu );
    logsfld_->setStretch( 2, 1 );
    logsfld_->setHSzPol( uiObject::Wide );
    logsfld_->attach( alignedBelow, wellfld_ );
    mAttachCB( logsfld_->selectionChanged, uiLASWriter::logSelCB );

    lognmfld_ = new uiGenInput( this, tr("In LAS file MNEM column write"),
			BoolInpSpec(true,tr("Mnemonic"),tr("Log name")) );
    lognmfld_->attach( alignedBelow, logsfld_ );

    mdrangefld_ = new uiGenInput( this, tr("MD range"),
				 FloatInpIntervalSpec(true) );
    mdrangefld_->attach( alignedBelow, lognmfld_ );
    mdrangefld_->setValue( SI().depthsInFeet() ? 0.5f : 0.1524f, 2 );

    zunitfld_ = new uiComboBox( this, "Z units" );
    zunitfld_->addItem( uiStrings::sMeter() );
    zunitfld_->addItem( uiStrings::sFeet() );
    zunitfld_->attach( rightTo, mdrangefld_ );
    zunitfld_->setCurrentItem( SI().depthsInFeet() ? 1 : 0 );

    nullfld_ = new uiGenInput( this, tr("Null value"),
				FloatInpSpec(-999.25) );
    nullfld_->attach( alignedBelow, mdrangefld_ );

    colwidthfld_ = new uiGenInput( this, tr("Log data column width"),
				   IntInpSpec(14,10) );
    colwidthfld_->attach( rightOf, nullfld_ );

    lasfld_ = new uiASCIIFileInput( this, false );
    lasfld_->setTitleText( tr("Output LAS file") );
    lasfld_->setFilter( Well::LASImporter::fileFilter() );
    lasfld_->setDefaultExtension( "las" );
    lasfld_->attach( alignedBelow, nullfld_ );

    mAttachCB( postFinalize(), uiLASWriter::finalizeCB );
}


uiLASWriter::~uiLASWriter()
{
    detachAllNotifiers();
}


void uiLASWriter::finalizeCB( CallBacker* )
{
    mdrangefld_->setNrDecimals( 4, 0 );
    mdrangefld_->setNrDecimals( 4, 1 );
    wellSelCB( nullptr );
}


void uiLASWriter::wellSelCB( CallBacker* )
{
    logsfld_->setEmpty();

    const IOObj* ioobj = wellfld_->ioobj( true );
    if ( !ioobj )
	return;

    const MultiID wellid = ioobj->key();
    if ( wellid.isUdf() )
	return;

    const Well::LoadReqs reqs( Well::Trck, Well::LogInfos );
    wd_ = Well::MGR().get( wellid, reqs );
    if ( !wd_ )
    {
	uiMSG().error( toUiString(Well::MGR().errMsg()) );
	return;
    }

    BufferStringSet lognms;
    Well::MGR().getLogNamesByID( wellid, lognms );
    lognms.sort();
    logsfld_->addItems( lognms );
    logsfld_->resizeToContents();

    const FilePath fp = ioobj->fullUserExpr();
    BufferString fnm( fp.baseName(), "_logs" );
    FilePath laspath( GetSurveyExportDir(), fnm );
    laspath.setExtension( "las" );
    lasfld_->setFileName( laspath.fullPath() );
}


void uiLASWriter::logSelCB( CallBacker* )
{
    if ( !wd_ )
	return;

    Interval<float> mdrg;
    mdrg.setUdf();
    BufferStringSet lognms;
    logsfld_->getChosen( lognms );
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const Well::Log* log = wd_->logs().getLog( lognms.get(idx).buf() );
	if ( !log )
	    continue;

	const Interval<float> rg = log->dahRange();
	mdrg.include( rg );
    }

    mdrg.start = Well::storageToDisplayDepth( mdrg.start );
    mdrg.stop = Well::storageToDisplayDepth( mdrg.stop );
    mdrangefld_->setValue( mdrg );
}


bool uiLASWriter::acceptOK( CallBacker* )
{
    const IOObj* ioobj = wellfld_->ioobj();
    if ( !ioobj )
	return false;

    const BufferString nullvalue = nullfld_->text();
    if ( nullvalue.isEmpty() )
    {
	uiMSG().error( tr("Null value can not be empty.") );
	return false;
    }

    const BufferString lasfnm = lasfld_->fileName();
    if ( lasfnm.isEmpty() )
    {
	uiMSG().error( tr("Please enter an output file name") );
	return false;
    }

    Well::Reader rdr( ioobj->key(), *wd_ );
    if ( !rdr.isUsable() )
    {
	uiMSG().error( rdr.errMsg() );
	return false;
    }

    BufferStringSet lognms;
    logsfld_->getChosen( lognms );
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const char* lognm = lognms.get( idx ).buf();
	rdr.getLog( lognm );
    }

    LASWriter laswriter( *wd_, lognms, lasfnm );
    laswriter.setNullValue( nullvalue );
    laswriter.setZInFeet( zunitfld_->currentItem()==1 );
    laswriter.setMDRange( mdrangefld_->getFStepInterval() );
    laswriter.setColumnWidth( colwidthfld_->getIntValue() );
    laswriter.writeLogName( !lognmfld_->getBoolValue() );
    bool res = laswriter.execute();
    if ( !res )
    {
	uiString errmsg = tr("Error occured while writing LAS file.");
	const uiString writermsg = laswriter.uiMessage();
	if ( !errmsg.isEmpty() )
	    errmsg.append( writermsg, true );

	return false;
    }

    const uiString msg = tr("Logs successfully exported.\n\n"
			    "Do you want to export more?");
    res = uiMSG().askGoOn( msg );
    return !res;
}

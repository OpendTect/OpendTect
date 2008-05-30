/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Raman Singh
 Date:          May 2008
 RCS:           $Id: uiimphorizon2d.cc,v 1.1 2008-05-30 07:08:52 cvsraman Exp $
________________________________________________________________________

-*/

#include "uiimphorizon2d.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uiempartserv.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseispartserv.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"

#include "emmanager.h"
#include "emsurfacetr.h"
#include "horizon2dscanner.h"
#include "randcolor.h"
#include "strmdata.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "filegen.h"
#include "emhorizon2d.h"

#include <math.h>


uiImportHorizon2D::uiImportHorizon2D( uiParent* p ) 
    : uiDialog(p,uiDialog::Setup("Import 2D Horizon",
				 "Specify parameters",
				 "104.0.0").oktext("Import"))
    , displayfld_(0)
    , dataselfld_(0)
    , scanner_(0)
    , linesetnms_(*new BufferStringSet)
    , fd_(*EM::Horizon2DAscIO::getDesc())
{
    uiEMPartServer::getAllSurfaceInfo( horinfos_, true );
    TypeSet<BufferStringSet> linenms;
    uiSeisPartServer::get2DLineInfo( linesetnms_, setids_, linenms );
    inpfld_ = new uiFileInput( this, "Input ASCII File", uiFileInput::Setup()
					    .withexamine(true)
					    .forread(true) );
    inpfld_->setDefaultSelectionDir( 
			    IOObjContext::getDataDirName(IOObjContext::Surf) );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    inpfld_->valuechanged.notify( mCB(this,uiImportHorizon2D,formatSel) );

    uiLabeledComboBox* lsetbox = new uiLabeledComboBox( this, "Select Line Set",
	    						"Line Set Selector" );
    lsetbox->attach( alignedBelow, inpfld_ );
    linesetfld_ = lsetbox->box();
    linesetfld_->addItems( linesetnms_ );
    linesetfld_->selectionChanged.notify( mCB(this,uiImportHorizon2D,setSel) );

    BufferStringSet hornms;
    for ( int idx=0; idx<horinfos_.size(); idx++ )
	hornms.add( horinfos_[idx]->name );

    uiLabeledListBox* horbox = new uiLabeledListBox( this, hornms,
	   				"Select Horizons to import", true ); 
    horbox->attach( alignedBelow, lsetbox );
    horselfld_ = horbox->box();
    horselfld_->selectionChanged.notify(mCB(this,uiImportHorizon2D,formatSel));

    uiPushButton* addbut = new uiPushButton( this, "Add new",
	    			mCB(this,uiImportHorizon2D,addHor), false );
    addbut->attach( rightTo, horbox );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "100.0.0" );
    dataselfld_->attach( alignedBelow, horbox );
    dataselfld_->descChanged.notify( mCB(this,uiImportHorizon2D,descChg) );

    scanbut_ = new uiPushButton( this, "Scan Input Files",
	    			 mCB(this,uiImportHorizon2D,scanPush), false );
    scanbut_->attach( alignedBelow, dataselfld_);

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, scanbut_ );

    interpolfld_ = new uiGenInput( this, "Interpolate",
	    			   BoolInpSpec(true,"Yes","No",true) );
    interpolfld_->attach( alignedBelow, scanbut_ );
    interpolfld_->attach( ensureBelow, sep );

    displayfld_ = new uiCheckBox( this, "Display after import" );
    displayfld_->attach( alignedBelow, interpolfld_ );
    finaliseDone.notify( mCB(this,uiImportHorizon2D,formatSel) );
}


uiImportHorizon2D::~uiImportHorizon2D()
{
    delete &linesetnms_;
    deepErase( horinfos_ );
}


void uiImportHorizon2D::descChg( CallBacker* cb )
{
    if ( scanner_ ) delete scanner_;
    scanner_ = 0;
}


void uiImportHorizon2D::formatSel( CallBacker* cb )
{
    if ( !dataselfld_ ) return;

    BufferStringSet hornms;
    horselfld_->getSelectedItems( hornms );
    const int nrhors = hornms.size();
    EM::Horizon2DAscIO::updateDesc( fd_, hornms );
    dataselfld_->updateSummary();
    dataselfld_->setSensitive( nrhors );
    scanbut_->setSensitive( *inpfld_->fileName() && nrhors );
}


void uiImportHorizon2D::setSel( CallBacker* )
{
}


void uiImportHorizon2D::addHor( CallBacker* )
{
    uiGenInputDlg* dlg = new uiGenInputDlg( this, "Add Horizon",
	    				    "Name", new StringInpSpec() );
    if ( !dlg->go() ) return;

    const char* hornm = dlg->text();
    horselfld_->addItem( hornm );
    const int idx = horselfld_->size() - 1;
    horselfld_->setSelected( idx, true );
}


void uiImportHorizon2D::scanPush( CallBacker* )
{
    if ( !horselfld_->nrSelected() )
    { uiMSG().error("Please select at least one horizon"); return; }

    if ( !dataselfld_->commit() ) return;
    if ( scanner_ ) return;

    BufferStringSet filenms;
    if ( !getFileNames(filenms) ) return;

    const char* setnm = linesetfld_->text();
    const int setidx = linesetnms_.indexOf( setnm );
    scanner_ = new Horizon2DScanner( filenms, setids_[setidx], fd_ );
    uiTaskRunner taskrunner( this );
    taskrunner.execute( *scanner_ );
    scanner_->launchBrowser();
}


bool uiImportHorizon2D::doDisplay() const
{
    return displayfld_ && displayfld_->isChecked();
}


#define mErrRet(s) { uiMSG().error(s); return 0; }
#define mErrRetUnRef(s) { horizon->unRef(); mErrRet(s) }
#define mSave() \
    if ( !exec ) \
    { \
	delete exec; \
	horizon->unRef(); \
	return false; \
    } \
    uiTaskRunner taskrunner( this ); \
    rv = taskrunner.execute( *exec ); \
    delete exec; 

bool uiImportHorizon2D::doImport()
{
/*    BufferStringSet attrnms;
    attrlistfld_->box()->getSelectedItems( attrnms );
    if ( isgeom_ ) attrnms.insertAt( new BufferString(sZVals), 0 );
    if ( !attrnms.size() ) mErrRet( "No Attributes Selected" );

    EM::Horizon3D* horizon = isgeom_ ? createHor() : loadHor();
    if ( !horizon ) return false;

    if ( !scanner_ && !doScan() ) return false;

    ObjectSet<BinIDValueSet> sections = scanner_->getSections();

    if ( sections.isEmpty() )
    {
	horizon->unRef();
	mErrRet( "Nothing to import" );
    }

    const bool dofill = filludffld_ && filludffld_->getBoolValue();
    if ( dofill )
	fillUdfs( sections );

    HorSampling hs = subselfld_->envelope().hrg;
    ExecutorGroup importer( "Importing horizon" );
    importer.setNrDoneText( "Nr positions done" );
    int startidx = 0;
    if ( isgeom_ )
    {
	importer.add( horizon->importer(sections,hs) );
	attrnms.remove( 0 );
	startidx = 1;
    }

    if ( attrnms.size() )
	importer.add( horizon->auxDataImporter(sections,attrnms,startidx,hs) );

    uiTaskRunner taskrunner( this );
    const bool success = taskrunner.execute( importer );
    deepErase( sections );
    if ( !success )
	mErrRetUnRef("Cannot import horizon")

    bool rv;
    if ( isgeom_ )
    {
	Executor* exec = horizon->saver();
	mSave();
    }
    else
    {
	mDynamicCastGet(ExecutorGroup*,exec,horizon->auxdata.auxDataSaver(-1))
	mSave();
    }
    if ( !doDisplay() )
	horizon->unRef();
    else
	horizon->unRefNoDelete();
    return rv;*/
    return false;
}


bool uiImportHorizon2D::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    return doImport();
}


bool uiImportHorizon2D::getFileNames( BufferStringSet& filenames ) const
{
    if ( !*inpfld_->fileName() )
	mErrRet( "Please select input file(s)" )
    
    inpfld_->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File_exists(fnm) )
	{
	    BufferString errmsg( "Cannot find input file:\n" );
	    errmsg += fnm;
	    deepErase( filenames );
	    mErrRet( errmsg );
	}
    }

    return true;
}


bool uiImportHorizon2D::checkInpFlds()
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    if ( !dataselfld_->commit() )
	mErrRet( "Please define data format" );

    return true;
}



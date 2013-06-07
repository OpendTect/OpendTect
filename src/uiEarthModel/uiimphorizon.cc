/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";

#include "uiimphorizon.h"
#include "uiarray2dinterpol.h"
#include "array2dinterpolimpl.h"
#include "uipossubsel.h"

#include "uicombobox.h"
#include "uicompoundparsel.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uitaskrunner.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiscaler.h"
#include "uiseparator.h"
#include "uistratlvlsel.h"
#include "uitblimpexpdatasel.h"

#include "arrayndimpl.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"
#include "horizonscanner.h"
#include "ioobj.h"
#include "pickset.h"
#include "randcolor.h"
#include "strmdata.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "file.h"
#include "emhorizon3d.h"

#include <math.h>

static const char* sZVals = "Z values";



uiImportHorizon::uiImportHorizon( uiParent* p, bool isgeom )
    : uiDialog(p,uiDialog::Setup("Import Horizon","Specify parameters",
				 "104.0.2").modal(false))
    , ctio_(*mMkCtxtIOObj(EMHorizon3D))
    , isgeom_(isgeom)
    , filludffld_(0)
    , interpol_(0)
    , colbut_(0)
    , stratlvlfld_(0)
    , displayfld_(0)
    , fd_(*EM::Horizon3DAscIO::getDesc())
    , scanner_(0)
    , importReady(this)
{
    setCtrlStyle( DoAndStay );
    setDeleteOnClose( false );
    ctio_.ctxt.forread = !isgeom_;

    BufferString fltr( "Text (*.txt *.dat);;XY/IC (*.*xy* *.*ic* *.*ix*)" );
    inpfld_ = new uiFileInput( this, "Input ASCII File",
	    uiFileInput::Setup(uiFileDialog::Gen)
	    .withexamine(true).forread(true).filter(fltr) );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    inpfld_->valuechanged.notify( mCB(this,uiImportHorizon,formatSel) );

    attrlistfld_ = new uiLabeledListBox( this, "Select Attribute(s) to import",
	   				 true );
    attrlistfld_->box()->setNrLines( 4 );
    attrlistfld_->attach( alignedBelow, inpfld_ );
    attrlistfld_->box()->selectionChanged.notify(
	    			mCB(this,uiImportHorizon,formatSel) );

    addbut_ = new uiPushButton( this, "Add new",
	    			mCB(this,uiImportHorizon,addAttrib), false );
    addbut_->attach( rightTo, attrlistfld_ );
    uiPushButton* rmbut = new uiPushButton( this, "Remove",
				mCB(this,uiImportHorizon,rmAttrib), true );
    rmbut->attach( alignedBelow, addbut_ );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, attrlistfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_, "104.0.8" );
    dataselfld_->attach( alignedBelow, attrlistfld_ );
    dataselfld_->attach( ensureBelow, sep );
    dataselfld_->descChanged.notify( mCB(this,uiImportHorizon,descChg) );

    scanbut_ = new uiPushButton( this, "Scan &Input File",
	    			 mCB(this,uiImportHorizon,scanPush), true );
    scanbut_->attach( alignedBelow, dataselfld_);

    sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, scanbut_ );

    subselfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    subselfld_->attach( alignedBelow, attrlistfld_ );
    subselfld_->attach( ensureBelow, sep );
    subselfld_->setSensitive( false );

    outputfld_ = new uiIOObjSel( this, ctio_ );
    outputfld_->setLabelText( isgeom_ ? "Output Horizon" : "Add to Horizon" );

    if ( isgeom_ )
    {
	setHelpID("104.0.0");
	filludffld_ = new uiGenInput( this, "Fill undefined parts",
				      BoolInpSpec(true) );
	filludffld_->valuechanged.notify(mCB(this,uiImportHorizon,fillUdfSel));
	filludffld_->setValue(false);
	filludffld_->setSensitive( false );
	filludffld_->attach( alignedBelow, subselfld_ );
	interpolparbut_ = new uiPushButton( this, "Settings", 
	       mCB(this,uiImportHorizon,interpolSettingsCB), false );
	interpolparbut_->attach( rightOf, filludffld_ );

	outputfld_->attach( alignedBelow, filludffld_ );

	stratlvlfld_ = new uiStratLevelSel( this, true );
	stratlvlfld_->attach( alignedBelow, outputfld_ );
	stratlvlfld_->selChange.notify( mCB(this,uiImportHorizon,stratLvlChg) );

	colbut_ = new uiColorInput( this,
		  		   uiColorInput::Setup(getRandStdDrawColor())
	       			   .lbltxt("Base color") );
	colbut_->attach( alignedBelow, stratlvlfld_ );

	displayfld_ = new uiCheckBox( this, "Display after import" );
	displayfld_->attach( alignedBelow, colbut_ );
	
	fillUdfSel(0);
    }
    else
	outputfld_->attach( alignedBelow, subselfld_ );

    postFinalise().notify( mCB(this,uiImportHorizon,formatSel) );
}


uiImportHorizon::~uiImportHorizon()
{
    delete ctio_.ioobj; delete &ctio_;
    delete interpol_;
}


void uiImportHorizon::descChg( CallBacker* cb )
{
    if ( scanner_ ) delete scanner_;
    scanner_ = 0;
}


void uiImportHorizon::interpolSettingsCB( CallBacker* )
{
    uiSingleGroupDlg dlg( this, uiDialog::Setup("Interpolation settings",
			  (const char*) 0, (const char*) 0 ) );

    uiArray2DInterpolSel* arr2dinterpfld =
	new uiArray2DInterpolSel( &dlg, true, true, false, interpol_ );
    arr2dinterpfld->setDistanceUnit( SI().xyInFeet() ? "[ft]" : "[m]" );
    dlg.setGroup( arr2dinterpfld );

    if ( dlg.go() )
    {
	delete interpol_;
	interpol_ = arr2dinterpfld->getResult();
    }
}
	
void uiImportHorizon::formatSel( CallBacker* cb )
{
    BufferStringSet attrnms;
    attrlistfld_->box()->getSelectedItems( attrnms );
    if ( isgeom_ ) attrnms.insertAt( new BufferString(sZVals), 0 );
    const int nrattrib = attrnms.size();
    const bool keepdef = cb==inpfld_ && fd_.isGood();
    if ( !keepdef )
    {
	EM::Horizon3DAscIO::updateDesc( fd_, attrnms );
	dataselfld_->updateSummary();
    }
    dataselfld_->setSensitive( nrattrib );

    const FixedString fnm = inpfld_->fileName();
    scanbut_->setSensitive( !fnm.isEmpty() && nrattrib );
    if ( !scanner_ ) 
    {
	subselfld_->setSensitive( false );
	if ( filludffld_ )
	    filludffld_->setSensitive( false );
    }
    else
    {
	delete scanner_;
	scanner_ = 0;
    }
}


void uiImportHorizon::addAttrib( CallBacker* )
{
    uiGenInputDlg dlg( this, "Add Attribute", "Name", new StringInpSpec() );
    if ( !dlg.go() ) return;

    const char* attrnm = dlg.text();
    attrlistfld_->box()->addItem( attrnm );
    const int idx = attrlistfld_->box()->size() - 1;
    attrlistfld_->box()->setSelected( idx, true );
}


void uiImportHorizon::rmAttrib( CallBacker* )
{
    int selidx = attrlistfld_->box()->currentItem();
    attrlistfld_->box()->removeItem( selidx );
    selidx--;
    if ( selidx < 0 ) selidx = 0;
    attrlistfld_->box()->setSelected( selidx );
}


void uiImportHorizon::scanPush( CallBacker* )
{
    if ( !isgeom_ && !attrlistfld_->box()->nrSelected() )
    { uiMSG().error("Please select at least one attribute"); return; }
    if ( !dataselfld_->commit() || !doScan() )
	return;

    if ( isgeom_ ) 
    {
	filludffld_->setSensitive( scanner_->gapsFound(true) ||
	    		   	   scanner_->gapsFound(false) );
	fillUdfSel(0);
    }

    subselfld_->setSensitive( true );

    scanner_->launchBrowser();
}


#define mNotCompatibleRet(ic) \
    const int df = n##ic##lnrg.start - ic##rg.start; \
    if ( df%2 && !(ic##rg.step%2) && !(n##ic##lnrg.step%2) ) \
    { \
	BufferString msg = "The horizon is not compatible with survey "; \
	msg +=  "trace, do you want to continue?"; \
	if ( !uiMSG().askGoOn(msg) ) \
    	    return false; \
    }


bool uiImportHorizon::doScan()
{
    BufferStringSet filenms;
    if ( !getFileNames(filenms) ) return false;

    scanner_ = new HorizonScanner( filenms, fd_, isgeom_ );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute( *scanner_ ) )
	return false;

    const StepInterval<int> nilnrg = scanner_->inlRg();
    const StepInterval<int> nclnrg = scanner_->crlRg();
    CubeSampling cs( true );
    const StepInterval<int> irg = cs.hrg.inlRange();
    const StepInterval<int> crg = cs.hrg.crlRange();
    if ( irg.start>nilnrg.stop || crg.start>nclnrg.stop ||
	 irg.stop<nilnrg.start || crg.stop<nclnrg.start )
	uiMSG().warning( "Your horizon is out of the survey range." );
    else if ( irg.step > 1 )
    {
	mNotCompatibleRet(i);
    }
    else if ( crg.step > 1 )
    {
	mNotCompatibleRet(c);
    }
    
    cs.hrg.set( nilnrg, nclnrg );
    subselfld_->setInput( cs );
    return true;
}


void uiImportHorizon::fillUdfSel( CallBacker* )
{
    if ( interpolparbut_ )
    {
	interpolparbut_->display( filludffld_->getBoolValue() );
	if ( !interpol_ && filludffld_->getBoolValue() )
	{
	    InverseDistanceArray2DInterpol* templ =
		new InverseDistanceArray2DInterpol;
	    templ->setSearchRadius( 10*(SI().inlDistance()+SI().crlDistance()));
	    templ->setFillType( Array2DInterpol::ConvexHull );
	    interpol_ = templ;
	}
    }
}


bool uiImportHorizon::doDisplay() const
{
    return displayfld_ && displayfld_->isChecked();
}


MultiID uiImportHorizon::getSelID() const
{
    MultiID mid = ctio_.ioobj ? ctio_.ioobj->key() : -1;
    return mid;
}


void uiImportHorizon::stratLvlChg( CallBacker* )
{
    if ( !stratlvlfld_ ) return;
    const Color col( stratlvlfld_->getColor() );
    if ( col != Color::NoColor() )
	colbut_->setColor( col );
}
    
#define mErrRet(s) { uiMSG().error(s); return 0; }
#define mErrRetUnRef(s) { horizon->unRef(); mErrRet(s) }
#define mSave(taskrunner) \
    if ( !exec ) \
    { \
	delete exec; \
	horizon->unRef(); \
	return false; \
    } \
    rv = taskrunner.execute( *exec ); \
    delete exec; 

bool uiImportHorizon::doImport()
{
    BufferStringSet attrnms;
    attrlistfld_->box()->getSelectedItems( attrnms );
    if ( isgeom_ ) attrnms.insertAt( new BufferString(sZVals), 0 );
    if ( !attrnms.size() ) mErrRet( "No Attributes Selected" );

    EM::Horizon3D* horizon = isgeom_ ? createHor() : loadHor();
    if ( !horizon ) return false;

    if ( !scanner_ && !doScan() ) return false;

    if ( scanner_->nrPositions() == 0 )
    {
	BufferString msg( "No valid positions found\n" );
	msg.add( "Please re-examine input file and format definition" );
	uiMSG().message( msg );
	return false;
    }

    ManagedObjectSet<BinIDValueSet> sections(false);
    deepCopy( sections, scanner_->getSections() );

    if ( sections.isEmpty() )
    {
	horizon->unRef();
	mErrRet( "Nothing to import" );
    }

    const bool dofill = filludffld_ && filludffld_->getBoolValue();
    if ( dofill )
    {
	if ( !interpol_ )
	{
	    uiMSG().error("No interpolation selected" );
	    return false;
	}

	fillUdfs( sections );
    }

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
    if ( !success )
	mErrRetUnRef("Cannot import horizon")

    bool rv;
    if ( isgeom_ )
    {
	Executor* exec = horizon->saver();
	mSave(taskrunner);
    }
    else
    {
	mDynamicCastGet(ExecutorGroup*,exec,horizon->auxdata.auxDataSaver(-1))
	mSave(taskrunner);
    }

    if ( !doDisplay() )
	horizon->unRef();
    else
	horizon->unRefNoDelete();

    return rv;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    const bool res = doImport();
    if ( res )
    {
	uiMSG().message( "Horizon successfully imported" );
	if ( doDisplay() )
	    importReady.trigger();
    }

    return false;
}


bool uiImportHorizon::getFileNames( BufferStringSet& filenames ) const
{
    if ( !*inpfld_->fileName() )
	mErrRet( "Please select input file(s)" )
    
    inpfld_->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File::exists(fnm) )
	{
	    BufferString errmsg( "Cannot find input file:\n" );
	    errmsg += fnm;
	    deepErase( filenames );
	    mErrRet( errmsg );
	}
    }

    return true;
}


bool uiImportHorizon::checkInpFlds()
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) ) return false;

    if ( !dataselfld_->commit() )
	mErrRet( "Please define data format" );

    const char* outpnm = outputfld_->getInput();
    if ( !outpnm || !*outpnm )
	mErrRet( "Please select output horizon" )

    if ( !outputfld_->commitInput() )
	return false;

    return true;
}


bool uiImportHorizon::fillUdfs( ObjectSet<BinIDValueSet>& sections )
{
    if ( !interpol_ )
	return false;
    HorSampling hs = subselfld_->envelope().hrg;

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();
    interpol_->setRowStep( inldist*hs.step.inl );
    interpol_->setColStep( crldist*hs.step.crl);
    uiTaskRunner taskrunner( this );
    Array2DImpl<float> arr( hs.nrInl(), hs.nrCrl() );
    if ( !arr.isOK() )
	return false;

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	arr.setAll( mUdf(float) );
	BinIDValueSet& data = *sections[idx];
	BinID bid;
	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl = hs.start.inl + inl*hs.step.inl;
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl = hs.start.crl + crl*hs.step.crl;
		BinIDValueSet::Pos pos = data.findFirst( bid );
		if ( pos.j >= 0 )
		{
		    const float* vals = data.getVals( pos );
		    if ( vals )
			arr.set( inl, crl, vals[0] );
		}
	    }
	}

	if ( !interpol_->setArray( arr, &taskrunner ) )
	    return false;

	if ( !taskrunner.execute(*interpol_) )
	    return false;

	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl = hs.start.inl + inl*hs.step.inl;
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl = hs.start.crl + crl*hs.step.crl;
		BinIDValueSet::Pos pos = data.findFirst( bid );
		if ( pos.j >= 0 ) continue;

		TypeSet<float> vals( data.nrVals(), mUdf(float) );
		vals[0] = arr.get( inl, crl );
		data.add( bid, vals.arr() );
	    }
	}
    }

    return true;
}


EM::Horizon3D* uiImportHorizon::createHor() const
{
    const char* horizonnm = outputfld_->getInput();
    EM::EMManager& em = EM::EMM();
    const MultiID mid = getSelID();
    EM::ObjectID objid = em.getObjectID( mid );
    if ( objid < 0 )
	objid = em.createObject( EM::Horizon3D::typeStr(), horizonnm );

    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(objid));
    if ( !horizon )
	mErrRet( "Cannot create horizon" );

    horizon->change.disable();
    horizon->setMultiID( mid );
    horizon->setStratLevelID( stratlvlfld_->getID() );
    horizon->setPreferredColor( colbut_->color() );
    horizon->ref();
    return horizon;
}


EM::Horizon3D* uiImportHorizon::loadHor()
{
    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.createTempObject( EM::Horizon3D::typeStr() );
    emobj->setMultiID( ctio_.ioobj->key() );
    Executor* loader = emobj->loader();
    if ( !loader ) mErrRet( "Cannot load horizon");

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*loader) )
	return 0;

    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    if ( !horizon ) mErrRet( "Error loading horizon");

    horizon->ref();
    delete loader;
    return horizon;
}




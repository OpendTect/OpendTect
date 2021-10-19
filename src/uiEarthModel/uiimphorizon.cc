/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		June 2002
________________________________________________________________________

-*/

#include "uiimphorizon.h"

#include "uiarray2dinterpol.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uicompoundparsel.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiscaler.h"
#include "uiseparator.h"
#include "uistratlvlsel.h"
#include "uistrings.h"
#include "uit2dconvsel.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "uitoolbutton.h"
#include "uiunitsel.h"

#include "arrayndimpl.h"
#include "array2dinterpolimpl.h"
#include "array2dconverter.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "emhorizonascio.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "emzmap.h"
#include "file.h"
#include "filepath.h"
#include "horizonscanner.h"
#include "ioman.h"
#include "oddirs.h"
#include "pickset.h"
#include "randcolor.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "zaxistransform.h"
#include "od_helpids.h"

#include <math.h>

static BufferString sImportFromPath;


void uiImportHorizon::initClass()
{}


uiImportHorizon::uiImportHorizon( uiParent* p, bool isgeom )
    : uiDialog(p,uiDialog::Setup(uiString::emptyString(),mNoDlgTitle,
				 mODHelpKey(mImportHorAttribHelpID) )
				 .modal(false))
    , ctio_(*mMkCtxtIOObj(EMHorizon3D))
    , isgeom_(isgeom)
    , filludffld_(0)
    , interpol_(0)
    , colbut_(0)
    , stratlvlfld_(0)
    , fd_(*EM::Horizon3DAscIO::getDesc())
    , scanner_(0)
    , tdsel_(0)
    , transfld_(0)
    , importReady(this)
{
    setVideoKey( mODVideoKey(mImportHorAttribHelpID) );
    setCaption(isgeom ? uiStrings::phrImport(uiStrings::sHorizon()) :
			uiStrings::phrImport(mJoinUiStrs(sHorizon(),sData())));
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );
    if ( isgeom )
	enableSaveButton( tr("Display after import") );

    setDeleteOnClose( false );
    ctio_.ctxt_.forread_ = !isgeom_;

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setSelectMode( uiFileDialog::ExistingFile );
    inpfld_->valuechanged.notify( mCB(this,uiImportHorizon,inputChgd) );

    OD::ChoiceMode mode =
	isgeom ? OD::ChooseZeroOrMore : OD::ChooseAtLeastOne;
    uiListBox::Setup su( mode, tr("Attribute(s) to import") );
    attrlistfld_ = new uiListBox( this, su );
    attrlistfld_->attach( alignedBelow, inpfld_ );
    attrlistfld_->setNrLines( 5 );
    attrlistfld_->itemChosen.notify( mCB(this,uiImportHorizon,inputChgd) );

    uiButtonGroup* butgrp =
		new uiButtonGroup( attrlistfld_, "Buttons", OD::Vertical );
    butgrp->attach( rightTo, attrlistfld_->box() );
    new uiToolButton( butgrp, "addnew", tr("Add new"),
				mCB(this,uiImportHorizon,addAttribCB) );
    new uiToolButton( butgrp, "remove", uiStrings::sRemove(),
				mCB(this,uiImportHorizon,rmAttribCB) );
    new uiToolButton( butgrp, "clear", tr("Clear list"),
				mCB(this,uiImportHorizon,clearListCB) );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, attrlistfld_ );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
		  mODHelpKey(mTableImpDataSel3DSurfacesHelpID) );
    dataselfld_->descChanged.notify( mCB(this,uiImportHorizon,descChg) );
    if ( isgeom && SI().zIsTime() )
    {
	tdsel_ = new uiCheckBox( this, tr("Horizon is in Depth domain"),
				mCB(this,uiImportHorizon,zDomSel) );
	tdsel_->attach( alignedBelow, attrlistfld_ );
	tdsel_->attach( ensureBelow, sep );

	uiT2DConvSel::Setup t2dsu( 0, false );
	t2dsu.ist2d( !SI().zIsTime() );
	transfld_ = new uiT2DConvSel( this, t2dsu );
	transfld_->display( false );
	transfld_->attach( alignedBelow, tdsel_ );
	dataselfld_->attach( alignedBelow, transfld_ );
    }
    else
    {
	dataselfld_->attach( alignedBelow, attrlistfld_ );
	dataselfld_->attach( ensureBelow, sep );
    }

    scanbut_ = new uiPushButton( this, tr("Scan Input File"),
				 mCB(this,uiImportHorizon,scanPush), true );
    scanbut_->attach( alignedBelow, dataselfld_);

    sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, scanbut_ );

    subselfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    subselfld_->attach( alignedBelow, attrlistfld_ );
    subselfld_->attach( ensureBelow, sep );
    subselfld_->setSensitive( false );

    outputfld_ = new uiIOObjSel( this, ctio_ );
    outputfld_->setLabelText( isgeom_
			     ? uiStrings::phrOutput( uiStrings::sHorizon() )
			     : tr("Add to Horizon") );

    if ( !isgeom_ )
    {
	fd_.setName( EM::Horizon3DAscIO::sKeyAttribFormatStr() );
	outputfld_->attach( alignedBelow, subselfld_ );
    }
    else
    {
	setHelpKey(mODHelpKey(mImportHorizonHelpID) );
	filludffld_ = new uiGenInput( this, tr("Fill undefined parts"),
				      BoolInpSpec(true) );
	filludffld_->valuechanged.notify(mCB(this,uiImportHorizon,fillUdfSel));
	filludffld_->setValue(false);
	filludffld_->setSensitive( false );
	filludffld_->attach( alignedBelow, subselfld_ );
	interpolparbut_ = new uiPushButton( this, uiStrings::sSettings(),
	       mCB(this,uiImportHorizon,interpolSettingsCB), false );
	interpolparbut_->attach( rightOf, filludffld_ );

	outputfld_->attach( alignedBelow, filludffld_ );

	stratlvlfld_ = new uiStratLevelSel( this, true );
	stratlvlfld_->attach( alignedBelow, outputfld_ );
	stratlvlfld_->selChange.notify( mCB(this,uiImportHorizon,stratLvlChg) );

	colbut_ = new uiColorInput( this,
				    uiColorInput::Setup(getRandStdDrawColor())
				    .lbltxt(tr("Base color")) );
	colbut_->attach( alignedBelow, stratlvlfld_ );

	fillUdfSel(0);
    }

    postFinalise().notify( mCB(this,uiImportHorizon,inputChgd) );
}


uiImportHorizon::~uiImportHorizon()
{
    delete ctio_.ioobj_; delete &ctio_;
    delete interpol_;
}


void uiImportHorizon::descChg( CallBacker* )
{
    deleteAndZeroPtr( scanner_ );
}


void uiImportHorizon::zDomSel( CallBacker* )
{
    if ( transfld_ )
	transfld_->display( tdsel_->isChecked() );

    inputChgd( 0 );
}


void uiImportHorizon::interpolSettingsCB( CallBacker* )
{
    uiSingleGroupDlg dlg( this, uiDialog::Setup(tr("Interpolation settings"),
			  uiStrings::sEmptyString(), mNoHelpKey ) );

    uiArray2DInterpolSel* arr2dinterpfld =
	new uiArray2DInterpolSel( &dlg, true, true, false, interpol_ );
    arr2dinterpfld->setDistanceUnit( SI().xyInFeet() ? tr("[ft]") : tr("[m]") );
    dlg.setGroup( arr2dinterpfld );

    if ( dlg.go() )
    {
	delete interpol_;
	interpol_ = arr2dinterpfld->getResult();
    }
}


void uiImportHorizon::inputChgd( CallBacker* cb )
{
    BufferStringSet attrnms;
    attrlistfld_->getChosen( attrnms );
    if ( isgeom_ )
    {
	BufferString zvalstr( sKey::ZValue() );
	if ( tdsel_ && tdsel_->isChecked() )
	    zvalstr = SI().zIsTime() ? sKey::Depth() : sKey::Time();

	attrnms.insertAt( new BufferString(zvalstr), 0 );
    }

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

    FilePath fnmfp( fnm );
    SetImportFromDir( fnmfp.pathOnly() );
    if ( isgeom_ )
	outputfld_->setInputText( fnmfp.baseName() );
}


void uiImportHorizon::addAttribCB( CallBacker* )
{
    uiGenInputDlg dlg( this, uiStrings::phrAdd(uiStrings::sAttribute()),
			     uiStrings::sName(),
			     new StringInpSpec() );
    if ( !dlg.go() ) return;

    const char* attrnm = dlg.text();
    attrlistfld_->addItem( toUiString(attrnm) );
    attrlistfld_->setChosen( attrlistfld_->size()-1, true );
}


void uiImportHorizon::rmAttribCB( CallBacker* )
{
    if ( attrlistfld_->isEmpty() )
	return;

    int selidx = attrlistfld_->currentItem();
    const bool updatedef = attrlistfld_->isChosen( selidx );

    attrlistfld_->removeItem( selidx );
    selidx--;
    if ( selidx < 0 ) selidx = 0;
    attrlistfld_->setChosen( selidx );

    if ( updatedef )
	inputChgd( 0 );
}


void uiImportHorizon::clearListCB( CallBacker* )
{
    const bool updatedef = attrlistfld_->nrChosen() > 0;
    attrlistfld_->setEmpty();

    if ( updatedef )
	inputChgd( 0 );
}


void uiImportHorizon::scanPush( CallBacker* )
{
    if ( !isgeom_ && !attrlistfld_->nrChosen() )
	{ uiMSG().error(tr("Please select at least one attribute")); return; }
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
	uiString msg = goOnMsg();  \
	if ( !uiMSG().askGoOn(msg) ) \
	    return false; \
    }


uiString uiImportHorizon::goOnMsg()
{
    uiString msg(tr("The horizon is not compatible with survey "
		      "trace, do you want to continue?"));
    return msg;
}


bool uiImportHorizon::doScan()
{
    BufferStringSet filenms;
    if ( !getFileNames(filenms) ) return false;

    uiTaskRunner taskrunner( this );
    ZAxisTransform* zatf = nullptr;
    int zatfvoi = -1;
    if ( tdsel_ && tdsel_->isChecked() && transfld_ && transfld_->acceptOK() )
    {
	zatf = transfld_->getSelection();
	if ( zatf->needsVolumeOfInterest() )
	{
	    //TODO: Use a smarter TrcKeyZSampling for VOI
	    zatfvoi = zatf->addVolumeOfInterest( SI().sampling(true), false );
	    if ( !zatf->loadDataIfMissing( zatfvoi, &taskrunner ) )
	    {
		uiMSG().error( tr("Cannot load data for depth-time transform"));
		return false;
	    }
	}
    }

    scanner_ = new HorizonScanner( filenms, fd_, isgeom_, zatf );
    if ( !scanner_->uiMessage().isEmpty() )
    {
	const bool res = uiMSG().askGoOn( tr("%1\nDo you want to continue?")
				.arg(scanner_->uiMessage()),
				tr("Continue with current Selection"),
				tr("I want to change the format definition") );
	if ( !res )
	    return false;
    }

    if ( !TaskRunner::execute( &taskrunner, *scanner_ ) )
	return false;

    if ( zatf && zatfvoi >= 0 )
	zatf->removeVolumeOfInterest( zatfvoi );

    const StepInterval<int> nilnrg = scanner_->inlRg();
    const StepInterval<int> nclnrg = scanner_->crlRg();
    TrcKeyZSampling cs( true );
    const StepInterval<int> irg = cs.hsamp_.inlRange();
    const StepInterval<int> crg = cs.hsamp_.crlRange();
    if ( irg.start>nilnrg.stop || crg.start>nclnrg.stop ||
	 irg.stop<nilnrg.start || crg.stop<nclnrg.start )
	uiMSG().warning( tr("Your horizon is out of the survey range.") );
    else if ( irg.step > 1 )
    {
	mNotCompatibleRet(i);
    }
    else if ( crg.step > 1 )
    {
	mNotCompatibleRet(c);
    }

    if ( nilnrg.step==0 || nclnrg.step==0 )
    {
	uiMSG().error( tr("Cannot have '0' as a step value") );
	return false;
    }

    cs.hsamp_.set( nilnrg, nclnrg );
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


MultiID uiImportHorizon::getSelID() const
{
    MultiID mid = ctio_.ioobj_ ? ctio_.ioobj_->key() : -1;
    return mid;
}


void uiImportHorizon::stratLvlChg( CallBacker* )
{
    if ( !stratlvlfld_ ) return;
    const OD::Color col( stratlvlfld_->getColor() );
    if ( col != OD::Color::NoColor() )
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
    rv = TaskRunner::execute( &taskrunner, *exec ); \
    delete exec;

bool uiImportHorizon::doImport()
{
    BufferStringSet attrnms;
    attrlistfld_->getChosen( attrnms );
    if ( isgeom_ )
    {
	BufferString zvalstr( sKey::ZValue() );
	if ( tdsel_ && tdsel_->isChecked() )
	    zvalstr = SI().zIsTime() ? sKey::Depth() : sKey::Time();

	attrnms.insertAt( new BufferString(zvalstr), 0 );
    }

    if ( attrnms.isEmpty() )
	mErrRet( tr("No Attributes Selected") );

    EM::Horizon3D* horizon = isgeom_ ? createHor() : loadHor();
    if ( !horizon ) return false;

    if ( !scanner_ && !doScan() )
    {
	horizon->unRef();
	return false;
    }

    if ( scanner_->nrPositions() == 0 )
    {
	uiString msg( tr("No valid positions found\n"
		      "Please re-examine input file and format definition") );
	mErrRetUnRef( msg );
    }

    ManagedObjectSet<BinIDValueSet> sections;
    deepCopy( sections, scanner_->getSections() );

    if ( sections.isEmpty() )
	mErrRetUnRef( tr("Nothing to import") );

    const bool dofill = filludffld_ && filludffld_->getBoolValue();
    if ( dofill )
    {
	if ( !interpol_ )
	    mErrRetUnRef( tr("No interpolation selected") );
	fillUdfs( sections );
    }

    TrcKeySampling hs = subselfld_->envelope().hsamp_;
    if ( hs.lineRange().step==0 || hs.trcRange().step==0 )
	mErrRetUnRef( tr("Cannot have '0' as a step value") )
    ExecutorGroup importer( "Importing horizon" );
    importer.setNrDoneText( tr("Nr positions done") );
    int startidx = 0;
    if ( isgeom_ )
    {
	importer.add( horizon->importer(sections,hs) );
	attrnms.removeSingle( 0 );
	startidx = 1;
    }

    if ( attrnms.size() )
	importer.add( horizon->auxDataImporter(sections,attrnms,startidx,hs) );

    uiTaskRunner taskrunner( this );
    const bool success = TaskRunner::execute( &taskrunner, importer );
    if ( !success )
	mErrRetUnRef(tr("Cannot import horizon"))

    bool rv;
    if ( isgeom_ )
    {
	Executor* exec = horizon->saver();
	mSave(taskrunner);
	horizon->setPreferredColor( colbut_->color() );
    }
    else
    {
	mDynamicCastGet(ExecutorGroup*,exec,
			horizon->auxdata.auxDataSaver(-1,true));
	mSave(taskrunner);
    }

    if ( saveButtonChecked() )
	horizon->unRefNoDelete();
    else
	horizon->unRef();

    return rv;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    if ( !doImport() )
	return false;

    if ( isgeom_ )
    {
	const IOObj* ioobj = outputfld_->ioobj();
	if ( ioobj )
	{
	    ioobj->pars().update( sKey::CrFrom(), inpfld_->fileName() );
	    ioobj->updateCreationPars();
	    IOM().commitChanges( *ioobj );
	}
    }

    if ( saveButtonChecked() )
	importReady.trigger();

    uiString msg = tr("3D Horizon successfully imported."
		      "\n\nDo you want to import more 3D Horizons?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}


bool uiImportHorizon::getFileNames( BufferStringSet& filenames ) const
{
    if ( !*inpfld_->fileName() )
	mErrRet( tr("Please select input file(s)") )

    inpfld_->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File::exists(fnm) )
	{
	    uiString errmsg = tr("Cannot find input file:\n%1")
			    .arg(fnm);
	    filenames.setEmpty();
	    mErrRet( errmsg );
	}
    }

    return true;
}


bool uiImportHorizon::checkInpFlds()
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) || !dataselfld_->commit() )
	return false;

    const char* outpnm = outputfld_->getInput();
    if ( !outpnm || !*outpnm )
	mErrRet( tr("Please select output horizon") )

    if ( !outputfld_->commitInput() )
	return false;

    const EM::IOObjInfo ioobjinfo( outputfld_->key() );
    BufferStringSet attribnms, chosennms, existingnms;
    ioobjinfo.getAttribNames( attribnms );
    attrlistfld_->getChosen( chosennms );

    for ( int idx=0; idx<chosennms.size(); idx++ )
	if ( attribnms.isPresent(chosennms.get(idx)) )
	    existingnms.add( chosennms.get(idx) );

    if ( !existingnms.isEmpty() )
    {
	const uiString msg =
	    (existingnms.size()>1 ? tr("%1 %2 already exist on disk.")
				  : tr("%1 %2 already exists on disk."))
				.arg(uiStrings::sAttribute(existingnms.size()))
				.arg(existingnms.cat(", "))
				.append("\nDo you want to overwrite?");
	if ( !uiMSG().askGoOn(msg,uiStrings::sYes(),uiStrings::sNo()) )
	    return false;
    }

    return true;
}


bool uiImportHorizon::fillUdfs( ObjectSet<BinIDValueSet>& sections )
{
    if ( !interpol_ )
	return false;
    TrcKeySampling hs = subselfld_->envelope().hsamp_;

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();
    interpol_->setRowStep( inldist*hs.step_.inl() );
    interpol_->setColStep( crldist*hs.step_.crl());
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
	    bid.inl() = hs.start_.inl() + inl*hs.step_.inl();
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl() = hs.start_.crl() + crl*hs.step_.crl();
		BinIDValueSet::SPos pos = data.find( bid );
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

	if ( !TaskRunner::execute( &taskrunner, *interpol_ ) )
	    return false;

	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl() = hs.start_.inl() + inl*hs.step_.inl();
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl() = hs.start_.crl() + crl*hs.step_.crl();
		BinIDValueSet::SPos pos = data.find( bid );
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
	mErrRet( uiStrings::sCantCreateHor() );

    horizon->change.disable();
    horizon->setMultiID( mid );
    horizon->setStratLevelID( stratlvlfld_->getID() );
    horizon->ref();
    return horizon;
}


EM::Horizon3D* uiImportHorizon::loadHor()
{
    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.createTempObject( EM::Horizon3D::typeStr() );
    emobj->setMultiID( ctio_.ioobj_->key() );
    Executor* loader = emobj->loader();
    if ( !loader ) mErrRet( uiStrings::sCantReadHor());

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *loader ) )
	return 0;

    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    if ( !horizon ) mErrRet( tr("Error loading horizon"));

    horizon->ref();
    delete loader;
    return horizon;
}


// uiImpHorFromZMap
uiImpHorFromZMap::uiImpHorFromZMap( uiParent* p )
    : uiDialog(p,uiDialog::Setup(tr("Import Horizon from ZMap"),
				 mNoDlgTitle,mODHelpKey(mFromZMap) )
				 .modal(false))
    , importReady(this)
    , crsfld_(nullptr)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );
    enableSaveButton( tr("Display after import") );

    inpfld_ = new uiASCIIFileInput( this, tr("ZMap file"), true );
    mAttachCB( inpfld_->valuechanged, uiImpHorFromZMap::inputChgd );

    uiObject* attachobj = inpfld_->attachObj();
    if ( SI().getCoordSystem() && SI().getCoordSystem()->isProjection() )
    {
	crsfld_ = new Coords::uiCoordSystemSel(this, false);
	mAttachCB( crsfld_->changed, uiImpHorFromZMap::inputChgd );
	crsfld_->attach(alignedBelow, inpfld_);
	attachobj = crsfld_->attachObj();
    }

    subselfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    subselfld_->attach( alignedBelow, attachobj );

    uiUnitSel::Setup uusu( Mnemonic::Dist, uiStrings::sUnit() );
    uusu.allowneg( true );
    unitfld_ = new uiUnitSel( this, uusu );
    unitfld_->attach( alignedBelow, subselfld_ );

    IOObjContext ctxt = mIOObjContext( EMHorizon3D );
    ctxt.forread_ = false;
    outputfld_ = new uiIOObjSel( this, ctxt, tr("Output Horizon") );
    outputfld_->attach( alignedBelow, unitfld_ );
}


uiImpHorFromZMap::~uiImpHorFromZMap()
{
    detachAllNotifiers();
}


MultiID uiImpHorFromZMap::getSelID() const
{
    const MultiID mid = outputfld_->key();
    return mid;
}


static void getCoordinates( const EM::ZMapImporter& importer,
			    TrcKeySampling& tks, Coord& mincrd, Coord& maxcrd )
{
    mincrd = importer.minCoord();
    maxcrd = importer.maxCoord();
    tks.init( false );
    tks.include( SI().transform(mincrd) );
    tks.include( SI().transform(maxcrd) );
    tks.include( SI().transform(Coord(mincrd.x,maxcrd.y)) );
    tks.include( SI().transform(Coord(maxcrd.x,mincrd.y)) );
    tks.step_ = SI().sampling(false).hsamp_.step_;
}


void uiImpHorFromZMap::inputChgd( CallBacker* )
{
    const FixedString horfnm = inpfld_->fileName();
    if ( horfnm.isEmpty() )
	return;

    EM::ZMapImporter importer( horfnm );
    if ( crsfld_ )
	importer.setCoordSystem( crsfld_->getCoordSystem() );

    TrcKeySampling tks; Coord mincrd, maxcrd;
    getCoordinates( importer, tks, mincrd, maxcrd );
    TrcKeyZSampling tkzs; tkzs.hsamp_ = tks;
    subselfld_->setInputLimit( tkzs );
    subselfld_->setInput( tkzs );

    const FilePath fnmfp( horfnm );
    SetImportFromDir( fnmfp.pathOnly() );
    outputfld_->setInputText( fnmfp.baseName() );
}


EM::Horizon3D* uiImpHorFromZMap::createHor() const
{
    const char* horizonnm = outputfld_->getInput();
    EM::EMManager& em = EM::EMM();
    const MultiID mid = getSelID();
    EM::ObjectID objid = em.getObjectID( mid );
    if ( objid < 0 )
	objid = em.createObject( EM::Horizon3D::typeStr(), horizonnm );

    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(objid));
    if ( !horizon )
	mErrRet( uiStrings::sCantCreateHor() );

    horizon->change.disable();
    horizon->setMultiID( mid );
    horizon->ref();
    return horizon;
}


bool uiImpHorFromZMap::acceptOK( CallBacker* )
{
    const FixedString horfnm = inpfld_->fileName();
    if ( !File::exists(horfnm) )
    {
	uiMSG().error( tr("Can not read input file.") );
	return false;
    }

    const UnitOfMeasure* zuom = unitfld_->getUnit();

    uiTaskRunner uitr( this );
    EM::ZMapImporter importer( horfnm );
    if ( crsfld_ )
	importer.setCoordSystem( crsfld_->getCoordSystem() );
    importer.setUOM( zuom );
    if ( !uitr.execute(importer) )
    {
	uiMSG().error( tr("Error reading ZMap file.") );
	return false;
    }

    TrcKeySampling tks; Coord mincrd, maxcrd;
    getCoordinates( importer, tks, mincrd, maxcrd );

    tks = subselfld_->envelope().hsamp_;

    const Array2D<float>* arr2d = importer.data();
    Array2DFromXYConverter conv( *arr2d, mincrd, importer.step() );
    conv.setOutputSampling( tks );
    if ( !uitr.execute(conv) )
    {
	uiMSG().error( tr("Can not convert ZMap grid to",
			  " Inline/Crossline domain") );
	return false;
    }

    EM::Horizon3D* hor3d = createHor();
    hor3d->setArray2D( conv.getOutput(), tks.start_, tks.step_, false );
    PtrMan<Executor> saver = hor3d->saver();
    if ( !uitr.execute(*saver) )
    {
	uiMSG().error( tr("Can not save output horizon.") );
	hor3d->unRef();
	return false;
    }

    const IOObj* ioobj = outputfld_->ioobj();
    if ( ioobj )
    {
	ioobj->pars().update( sKey::CrFrom(), inpfld_->fileName() );
	ioobj->updateCreationPars();
	IOM().commitChanges( *ioobj );
    }

    if ( saveButtonChecked() )
    {
	importReady.trigger();
	hor3d->unRefNoDelete();
    }
    else
	hor3d->unRef();

    uiString msg = tr("ZMap grid successfully imported."
		      "\n\nDo you want to import more grids?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiimphorizon.h"

#include "uiarray2dinterpol.h"
#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicolor.h"
#include "uicoordsystem.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiseparator.h"
#include "uistratlvlsel.h"
#include "uistrings.h"
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
#include "randcolor.h"
#include "survinfo.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"
#include "od_helpids.h"

#include <math.h>


uiImportHorizon::uiImportHorizon( uiParent* p, bool isgeom )
    : uiDialog(p,Setup(uiString::emptyString(),
		       mODHelpKey(mImportHorAttribHelpID)).modal(false))
    , importReady(this)
    , fd_(*EM::Horizon3DAscIO::getDesc())
    , isgeom_(isgeom)
{
    setVideoKey( mODVideoKey(mImportHorAttribHelpID) );
    setCaption(isgeom ? uiStrings::phrImport(uiStrings::sHorizon()) :
			uiStrings::phrImport(mJoinUiStrs(sHorizon(),sData())));
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );
    if ( isgeom )
	enableSaveButton( tr("Display after import") );

    setDeleteOnClose( false );

    inpfld_ = new uiASCIIFileInput( this, true );
    inpfld_->setSelectMode( uiFileDialog::ExistingFile );
    mAttachCB( inpfld_->valueChanged, uiImportHorizon::inputChgd );

    OD::ChoiceMode mode =
	isgeom ? OD::ChooseZeroOrMore : OD::ChooseAtLeastOne;
    uiListBox::Setup su( mode, tr("Attribute(s) to import") );
    attrlistfld_ = new uiListBox( this, su, "attributes" );
    attrlistfld_->attach( alignedBelow, inpfld_ );
    attrlistfld_->setNrLines( 5 );
    mAttachCB( attrlistfld_->itemChosen, uiImportHorizon::inputChgd );

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
    mAttachCB( dataselfld_->descChanged, uiImportHorizon::descChg );
    if ( isgeom )
    {
	zdomainfld_ = new uiGenInput( this, tr("Horizon is in"),
		    BoolInpSpec(true,uiStrings::sTime(),uiStrings::sDepth()) );
	zdomainfld_->attach( alignedBelow, attrlistfld_ );
	zdomainfld_->attach( ensureBelow, sep );
	zdomainfld_->setValue( SI().zIsTime() );
	mAttachCB( zdomainfld_->valueChanged, uiImportHorizon::zDomainCB );

	dataselfld_->attach( alignedBelow, zdomainfld_ );
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

    outputfld_ = new uiHorizon3DSel( this, !isgeom_ );
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
	mAttachCB( filludffld_->valueChanged, uiImportHorizon::fillUdfSel );
	filludffld_->setValue(false);
	filludffld_->setSensitive( false );
	filludffld_->attach( alignedBelow, subselfld_ );

	interpolparbut_ = new uiPushButton( this, uiStrings::sSettings(),
	       mCB(this,uiImportHorizon,interpolSettingsCB), false );
	interpolparbut_->attach( rightOf, filludffld_ );

	outputfld_->attach( alignedBelow, filludffld_ );

	stratlvlfld_ = new uiStratLevelSel( this, true );
	stratlvlfld_->attach( alignedBelow, outputfld_ );
	mAttachCB( stratlvlfld_->selChange, uiImportHorizon::stratLvlChg );

	colbut_ = new uiColorInput( this,
				  uiColorInput::Setup(OD::getRandStdDrawColor())
				    .lbltxt(tr("Base color")) );
	colbut_->attach( alignedBelow, stratlvlfld_ );

	fillUdfSel( nullptr );
    }

    mAttachCB( postFinalize(), uiImportHorizon::inputChgd );
}


uiImportHorizon::~uiImportHorizon()
{
    detachAllNotifiers();
    delete scanner_;
    delete interpol_;
}


void uiImportHorizon::descChg( CallBacker* )
{
    deleteAndNullPtr( scanner_ );
}


void uiImportHorizon::zDomainCB( CallBacker* )
{
    inputChgd( nullptr );
}


void uiImportHorizon::interpolSettingsCB( CallBacker* )
{
    uiSingleGroupDlg dlg( this, uiDialog::Setup(tr("Interpolation settings"),
			  uiStrings::sEmptyString(),mNoHelpKey ) );

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
	auto* zvalstr = new BufferString(
	    zdomainfld_->getBoolValue() ? sKey::Time() : sKey::Depth() );
	attrnms.insertAt( zvalstr, 0 );
    }

    const int nrattrib = attrnms.size();
    const bool keepdef = cb==inpfld_ && fd_.isGood();
    if ( !keepdef )
    {
	EM::Horizon3DAscIO::updateDesc( fd_, attrnms );
	dataselfld_->updateSummary();
    }

    dataselfld_->setSensitive( nrattrib );
    const StringView fnm = inpfld_->fileName();
    scanbut_->setSensitive( !fnm.isEmpty() && nrattrib );
    if ( !scanner_ )
    {
	subselfld_->setSensitive( false );
	if ( filludffld_ )
	    filludffld_->setSensitive( false );
    }

    deleteAndNullPtr( scanner_ );

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
    if ( !dlg.go() )
	return;

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
	inputChgd( nullptr );
}


void uiImportHorizon::clearListCB( CallBacker* )
{
    const bool updatedef = attrlistfld_->nrChosen() > 0;
    attrlistfld_->setEmpty();

    if ( updatedef )
	inputChgd( nullptr );
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
	fillUdfSel( nullptr );
    }

    subselfld_->setSensitive( true );

    scanner_->launchBrowser();
}


    #define mNotCompatibleRet(ic) \
    const int df = n##ic##lnrg.start_ - ic##rg.start_; \
    if ( df%2 && !(ic##rg.step_%2) && !(n##ic##lnrg.step_%2) ) \
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
    if ( !getFileNames(filenms) )
	return false;

    scanner_ = new HorizonScanner( filenms, fd_, isgeom_, zDomain() );
    if ( !scanner_->uiMessage().isEmpty() )
    {
	const bool res = uiMSG().askGoOn( scanner_->uiMessage(),
				tr("Continue with selected Z unit"),
				tr("I want to change the Format definition") );
	if ( !res )
	    return false;
    }

    uiTaskRunner uitr( this );
    if ( !TaskRunner::execute(&uitr,*scanner_) )
	return false;

    const StepInterval<int> nilnrg = scanner_->inlRg();
    const StepInterval<int> nclnrg = scanner_->crlRg();
    TrcKeyZSampling cs( true );
    const StepInterval<int> irg = cs.hsamp_.inlRange();
    const StepInterval<int> crg = cs.hsamp_.crlRange();
    if ( irg.start_>nilnrg.stop_ || crg.start_>nclnrg.stop_ ||
	 irg.stop_<nilnrg.start_ || crg.stop_<nclnrg.start_ )
	uiMSG().warning( tr("Your horizon is out of the survey range.") );
    else if ( irg.step_ > 1 )
    {
	mNotCompatibleRet(i);
    }
    else if ( crg.step_ > 1 )
    {
	mNotCompatibleRet(c);
    }

    if ( nilnrg.step_==0 || nclnrg.step_==0 )
    {
	uiMSG().error( tr("Cannot have '0' as a step value") );
	return false;
    }

    cs.hsamp_.set( nilnrg, nclnrg );
    subselfld_->setInput( cs );
    return true;
}


const ZDomain::Info& uiImportHorizon::zDomain() const
{
    if ( !zdomainfld_ )
	return SI().zDomainInfo();

    uiRetVal ret;
    return Table::AscIO::zDomain( fd_, 1, ret );
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
    return outputfld_->key( true );
}


void uiImportHorizon::stratLvlChg( CallBacker* )
{
    if ( !stratlvlfld_ ) return;
    const OD::Color col( stratlvlfld_->getColor() );
    if ( col != OD::Color::NoColor() )
	colbut_->setColor( col );
}

#define mErrRet(s) { uiMSG().error(s); return 0; }
bool uiImportHorizon::doImport()
{
    BufferStringSet attrnms;
    attrlistfld_->getChosen( attrnms );
    if ( isgeom_ )
    {
	auto* zvalstr = new BufferString(
	    zdomainfld_->getBoolValue() ? sKey::Time() : sKey::Depth() );
	attrnms.insertAt( zvalstr, 0 );
    }

    if ( attrnms.isEmpty() )
	mErrRet( tr("No Attributes Selected") );

    RefMan<EM::Horizon3D> horizon = isgeom_ ? createHor() : loadHor();
    if ( !horizon )
	return false;

    if ( !scanner_ && !doScan() )
	return false;

    if ( scanner_->nrPositions() == 0 )
    {
	uiString msg( tr("No valid positions found\n"
		      "Please re-examine input file and format definition") );
	mErrRet( msg );
    }

    ManagedObjectSet<BinIDValueSet> sections;
    deepCopy( sections, scanner_->getSections() );

    if ( sections.isEmpty() )
	mErrRet( tr("Nothing to import") );

    const bool dofill = filludffld_ && filludffld_->getBoolValue();
    if ( dofill )
    {
	if ( !interpol_ )
	    mErrRet( tr("No interpolation selected") );
	fillUdfs( sections );
    }

    TrcKeySampling hs = subselfld_->envelope().hsamp_;
    if ( hs.lineRange().step_==0 || hs.trcRange().step_==0 )
	mErrRet( tr("Cannot have '0' as a step value") )

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
	mErrRet(tr("Cannot import horizon"))

    horizon->setZDomain( zDomain() );
    PtrMan<Executor> exec;
    if ( isgeom_ )
    {
	horizon->setPreferredColor( colbut_->color() );
	exec = horizon->saver();
    }
    else
	exec = horizon->auxdata.auxDataSaver( -1, true );

    if ( !exec || !TaskRunner::execute(&taskrunner,*exec) )
	return false;

    exec = nullptr;
    if ( saveButtonChecked() )
	horizon.setNoDelete( true );

    return true;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() )
	return false;

    if ( !doImport() )
	return false;

    if ( isgeom_ )
    {
	const IOObj* ioobj = outputfld_->ioobj();
	if ( ioobj )
	{
	    EM::EMManager& em = EM::EMM();
	    EM::ObjectID objid = em.getObjectID( ioobj->key() );
	    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(objid))
	    const ZDomain::Info& info = horizon ? horizon->zDomain() :
								zDomain();
	    info.fillPar( ioobj->pars() );
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

    if ( !outputfld_->ioobj() )
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
    if ( !objid.isValid() )
	objid = em.createObject( EM::Horizon3D::typeStr(), horizonnm );

    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(objid))
    if ( !horizon )
	mErrRet( uiStrings::sCantCreateHor() );

    horizon->change.disable();
    horizon->setMultiID( mid );
    horizon->setStratLevelID( stratlvlfld_->getID() );
    horizon->setZDomain( zDomain() );
    horizon->ref();
    return horizon;
}


EM::Horizon3D* uiImportHorizon::loadHor()
{
    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.createTempObject( EM::Horizon3D::typeStr() );
    emobj->setMultiID( outputfld_->key(true) );
    Executor* loader = emobj->loader();
    if ( !loader )
	mErrRet( uiStrings::sCantReadHor());

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *loader ) )
	return nullptr;

    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    if ( !horizon )
	mErrRet( tr("Error loading horizon"));

    horizon->ref();
    delete loader;
    return horizon;
}


// uiImpHorFromZMap
uiImpHorFromZMap::uiImpHorFromZMap( uiParent* p )
    : uiDialog(p,Setup(tr("Import Horizon from ZMap"),mODHelpKey(mFromZMap))
		    .modal(false))
    , importReady(this)
    , crsfld_(nullptr)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );
    enableSaveButton( tr("Display after import") );

    inpfld_ = new uiASCIIFileInput( this, tr("ZMap file"), true );
    mAttachCB( inpfld_->valueChanged, uiImpHorFromZMap::inputChgd );

    uiObject* attachobj = inpfld_->attachObj();
    if ( SI().getCoordSystem() && SI().getCoordSystem()->isProjection() )
    {
	crsfld_ = new Coords::uiCoordSystemSel( this );
	mAttachCB( crsfld_->changed, uiImpHorFromZMap::inputChgd );
	crsfld_->attach( alignedBelow, inpfld_ );
	attachobj = crsfld_->attachObj();
    }

    uiUnitSel::Setup uusu( tr("Z unit in file") );
    uusu.allowneg( true );
    unitfld_ = new uiUnitSel( this, uusu );
    unitfld_->setUnit( UnitOfMeasure::surveyDefZUnit() );
    unitfld_->attach( alignedBelow, attachobj );

    outputfld_ = new uiHorizon3DSel( this, false );
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


void uiImpHorFromZMap::inputChgd( CallBacker* )
{
    const StringView horfnm = inpfld_->fileName();
    if ( horfnm.isEmpty() )
	return;

    EM::ZMapImporter importer( horfnm );
    if ( crsfld_ )
    {
	ConstRefMan<Coords::CoordSystem> coordsystem
				= crsfld_->getCoordSystem();
	importer.setCoordSystem( coordsystem.ptr() );
    }

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
    if ( !objid.isValid() )
	objid = em.createObject( EM::Horizon3D::typeStr(), horizonnm );

    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(objid))
    if ( !horizon )
	mErrRet( uiStrings::sCantCreateHor() );

    horizon->change.disable();
    horizon->setMultiID( mid );
    horizon->ref();
    return horizon;
}


bool uiImpHorFromZMap::acceptOK( CallBacker* )
{
    const StringView horfnm = inpfld_->fileName();
    if ( !File::exists(horfnm) )
    {
	uiMSG().error( tr("Can not read input file.") );
	return false;
    }

    const UnitOfMeasure* zuom = unitfld_->getUnit();

    uiTaskRunner uitr( this );
    EM::ZMapImporter importer( horfnm );
    if ( crsfld_ )
	importer.setCoordSystem( crsfld_->getCoordSystem().ptr() );
    importer.setUOM( zuom );
    if ( !uitr.execute(importer) )
    {
	uiMSG().error( tr("Error reading ZMap file.") );
	return false;
    }

    const TrcKeySampling tks = importer.sampling();
    const Array2D<float>* arr2d = importer.data();
    Array2DFromXYConverter conv( *arr2d, importer.minCoord(), importer.step() );
    conv.setOutputSampling( tks );
    if ( !uitr.execute(conv) )
    {
	uiMSG().error( tr("Can not convert ZMap grid to",
			  " Inline/Crossline domain") );
	return false;
    }

    RefMan<EM::Horizon3D> hor3d = createHor();
    hor3d->setArray2D( conv.getOutput(), tks.start_, tks.step_, false );
    PtrMan<Executor> saver = hor3d->saver();
    if ( !saver || !uitr.execute(*saver) )
    {
	uiMSG().error( tr("Can not save output horizon.") );
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
	hor3d.setNoDelete( true );
	importReady.trigger();
    }

    uiString msg = tr("ZMap grid successfully imported."
		      "\n\nDo you want to import more grids?");
    bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				tr("No, close window") );
    return !ret;
}

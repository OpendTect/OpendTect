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
#include "uifilesel.h"
#include "uifiledlg.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiscaler.h"
#include "uiseparator.h"
#include "uistratlvlsel.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uitblimpexpdatasel.h"
#include "uitoolbutton.h"

#include "arrayndimpl.h"
#include "array2dinterpolimpl.h"
#include "binnedvalueset.h"
#include "ctxtioobj.h"
#include "emhorizonascio.h"
#include "emhorizon3d.h"
#include "emioobjinfo.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "file.h"
#include "filepath.h"
#include "horizonscanner.h"
#include "oddirs.h"
#include "pickset.h"
#include "randcolor.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "od_helpids.h"

#include <math.h>

static const char* sZVals = "Z values";

static BufferString sImportFromPath;


void uiImportHorizon::initClass()
{ sImportFromPath = GetDataDir(); }


uiImportHorizon::uiImportHorizon( uiParent* p, bool isgeom )
    : uiDialog(p,uiDialog::Setup(uiString::empty(),mNoDlgTitle,
				 mODHelpKey(mImportHorAttribHelpID) )
				 .modal(false))
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
    setCaption(isgeom ? uiStrings::phrImport(uiStrings::sHorizon()) :
			uiStrings::phrImport(tr("Horizon Data")));
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );
    setDeleteOnClose( false );
    ctio_.ctxt_.forread_ = !isgeom_;

    uiFileSel::Setup fssu;
    fssu.withexamine( true ).initialselectiondir( sImportFromPath );
    fssu.formats_.addFormat( File::Format(tr("Text file"),"txt","dat") );
    fssu.formats_.addFormat( File::Format(tr("Position file"),"xy","ic","ix") );
    inpfld_ = new uiFileSel( this, uiStrings::phrInput(uiStrings::sASCIIFile()),
			     fssu );
    inpfld_->newSelection.notify( mCB(this,uiImportHorizon,inputChgd) );

    OD::ChoiceMode mode =
	isgeom ? OD::ChooseZeroOrMore : OD::ChooseAtLeastOne;
    uiListBox::Setup su( mode, tr("Attribute(s) to import") );
    attrlistfld_ = new uiListBox( this, su );
    attrlistfld_->attach( alignedBelow, inpfld_ );
    attrlistfld_->setNrLines( 6 );
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
    dataselfld_->attach( alignedBelow, attrlistfld_ );
    dataselfld_->attach( ensureBelow, sep );
    dataselfld_->descChanged.notify( mCB(this,uiImportHorizon,descChg) );

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
	interpolparbut_ = uiPushButton::getStd( this, OD::Settings,
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

	displayfld_ = new uiCheckBox( this, tr("Display after import") );
	displayfld_->attach( alignedBelow, colbut_ );

	fillUdfSel(0);
    }

    postFinalise().notify( mCB(this,uiImportHorizon,inputChgd) );
}


uiImportHorizon::~uiImportHorizon()
{
    delete ctio_.ioobj_; delete &ctio_;
    delete interpol_;
}


void uiImportHorizon::descChg( CallBacker* cb )
{
    if ( scanner_ ) delete scanner_;
    scanner_ = 0;
}


void uiImportHorizon::interpolSettingsCB( CallBacker* )
{
    uiSingleGroupDlg<uiArray2DInterpolSel> dlg( this,
                new uiArray2DInterpolSel( 0, true, true, false, interpol_ ));

    dlg.setCaption( uiStrings::sInterpolation() );
    dlg.setTitleText( uiStrings::sSettings() );

    if ( dlg.go() )
    {
	delete interpol_;
	interpol_ = dlg.getDlgGroup()->getResult();
    }
}


void uiImportHorizon::inputChgd( CallBacker* cb )
{
    BufferStringSet attrnms;
    attrlistfld_->getChosen( attrnms );
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

    File::Path fnmfp( fnm );
    sImportFromPath = fnmfp.pathOnly();
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
    {
	uiMSG().error(uiStrings::phrPlsSelectAtLeastOne(
			    uiStrings::sAttribute()));
	return;
    }
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

    scanner_ = new HorizonScanner( filenms, fd_, isgeom_ );
    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *scanner_ ) )
	return false;

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


bool uiImportHorizon::doDisplay() const
{
    return displayfld_ && displayfld_->isChecked();
}


DBKey uiImportHorizon::getSelID() const
{
    return ctio_.ioobj_ ? ctio_.ioobj_->key() : DBKey::getInvalid();
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
    rv = TaskRunner::execute( &taskrunner, *exec ); \
    delete exec;

bool uiImportHorizon::doImport()
{
    BufferStringSet attrnms;
    attrlistfld_->getChosen( attrnms );
    if ( isgeom_ )
	attrnms.insertAt( new BufferString(sZVals), 0 );
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

    ManagedObjectSet<BinnedValueSet> sections;
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

    if ( !doDisplay() )
	horizon->unRef();
    else
	horizon->unRefNoDelete();

    return rv;
}


bool uiImportHorizon::acceptOK()
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
	    ioobj->commitChanges();
	}
    }

    if ( doDisplay() )
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
	mErrRet( uiStrings::phrSelect(tr("input files")) )

    inpfld_->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File::exists(fnm) )
	{
	    uiString errmsg = uiStrings::phrCannotFind(toUiString("%1:\n%2")
			      .arg(uiStrings::sInputFile().toLower())
			      .arg(fnm));
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
	mErrRet( uiStrings::phrSelect(tr("output horizon")) );
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
	    (existingnms.size()>1 ? tr("%1 %2 already exist on disk")
				  : tr("%1 %2 already exists on disk"))
				.arg(uiStrings::sAttribute(existingnms.size()))
				.arg(existingnms.cat(", "))
				.appendPhrase(tr("Do you want to overwrite?"));
	if ( !uiMSG().askGoOn(msg,uiStrings::sYes(),uiStrings::sNo()) )
	    return false;
    }

    return true;
}


bool uiImportHorizon::fillUdfs( ObjectSet<BinnedValueSet>& sections )
{
    if ( !interpol_ )
	return false;
    TrcKeySampling hs = subselfld_->envelope().hsamp_;

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();
    interpol_->setRowStep( inldist*hs.step_.inl() );
    interpol_->setColStep( crldist*hs.step_.crl());
    uiTaskRunnerProvider trprov( this );
    Array2DImpl<float> arr( hs.nrInl(), hs.nrCrl() );
    if ( !arr.isOK() )
	return false;

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	arr.setAll( mUdf(float) );
	BinnedValueSet& data = *sections[idx];
	BinID bid;
	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl() = hs.start_.inl() + inl*hs.step_.inl();
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl() = hs.start_.crl() + crl*hs.step_.crl();
		const auto pos = data.find( bid );
		if ( pos.isValid() )
		{
		    const float* vals = data.getVals( pos );
		    if ( vals )
			arr.set( inl, crl, vals[0] );
		}
	    }
	}

	if ( !interpol_->setArray( arr, trprov ) )
	    return false;

	if ( !trprov.execute( *interpol_ ) )
	    return false;

	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl() = hs.start_.inl() + inl*hs.step_.inl();
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl() = hs.start_.crl() + crl*hs.step_.crl();
		const auto pos = data.find( bid );
		if ( pos.isValid() ) continue;

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
    EM::ObjectManager& mgr = EM::Hor3DMan();
    RefMan<EM::Object> obj =
	mgr.createObject( EM::Horizon3D::typeStr(), horizonnm );

    mDynamicCastGet(EM::Horizon3D*,horizon,obj.ptr());
    if ( !horizon )
	mErrRet( uiStrings::phrCannotCreateHor() );

    horizon->objectChanged().disable();
    horizon->setStratLevelID( stratlvlfld_->getID() );
    horizon->ref();
    return horizon;
}


EM::Horizon3D* uiImportHorizon::loadHor()
{
    EM::ObjectManager& mgr = EM::Hor3DMan();
    EM::Object* emobj = mgr.createTempObject( EM::Horizon3D::typeStr() );
    emobj->setDBKey( ctio_.ioobj_->key() );
    Executor* loader = emobj->loader();
    if ( !loader )
	mErrRet( uiStrings::phrCannotReadHor());

    if ( !uiTaskRunner(this).execute( *loader ) )
	return 0;

    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    if ( !horizon )
	mErrRet( toUiString("Internal: object is not a Horizon3D") );

    horizon->ref();
    delete loader;
    return horizon;
}

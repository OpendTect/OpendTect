/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiimppickset.h"

#include "iodir.h"
#include "iodirentry.h"
#include "oddirs.h"
#include "tabledef.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uipickpartserv.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"

#include "ctxtioobj.h"
#include "file.h"
#include "filepath.h"
#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "picksettr.h"
#include "randcolor.h"
#include "od_istream.h"
#include "od_ostream.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "od_helpids.h"
#include "picklocation.h"

#include <math.h>

mImplFactory1Param( uiPickSetImportGroup, uiParent*,
		    uiPickSetImportGroup::factory );

uiPickSetImportGroup::uiPickSetImportGroup( uiParent* p )
    : uiGroup(p)
{}


uiPickSetImportGroup::~uiPickSetImportGroup()
{}


void uiPickSetImportGroup::setPartServer( uiPickPartServer* pps )
{
    serv_ = pps;
}


bool uiPickSetImportGroup::init()
{
    if ( !serv_ )
	return false;

    if ( horinpfld_ )
    {
	serv_->fetchHors( false );
	const ObjectSet<SurfaceInfo> hinfos = serv_->horInfos();
	for ( int idx=0; idx<hinfos.size(); idx++ )
	    horinpfld_->box()->addItem( hinfos[idx]->name_ );
    }

    return true;
}


bool uiPickSetImportGroup::handleDuplicateNames(
					ObjectSet<Pick::Set>& pointsets,
					BufferStringSet& duplicatenames )
{
    const CtxtIOObj ctio( mIOObjContext(PickSet) );
    const IOObjContext ctxt = ctio.ctxt_;
    const IODir iodir( ctxt.getSelKey() ) ;
    const IODirEntryList pointsetlist( iodir, ctxt );
    BufferStringSet existingpointsetnames;
    pointsetlist.getIOObjNames( existingpointsetnames );

    for ( const auto* pointset : pointsets )
    {
	const BufferString pointsetname = pointset->name();
	if( existingpointsetnames.isPresent(pointsetname) )
	    duplicatenames.add( pointsetname );
    }

    if ( !duplicatenames.isEmpty() )
    {
	const uiString shape = getPointSetShape();
	const int duplicatesize = duplicatenames.size();

	return uiMSG().askGoOn( tr("The following %1 name%2 already exist%3 in "
				  "the survey: %4\n\nDo you still want"
				  " to add %5?\n(If added, %6 will appear"
				  " as \"name (#)\")")
				   .arg( shape )
				   .arg( duplicatesize == 1 ? "" : "s" )
				   .arg( duplicatesize == 1 ? "s" : "" )
				   .arg( duplicatenames.getDispString(10) )
				   .arg( duplicatesize == 1 ? "it" : "them" )
				   .arg( duplicatesize == 1 ? "it" : "they" ),
			       tr("Add"),tr("Skip") );
    }

    return true;
}


bool uiPickSetImportGroup::setToNextAvailableName( Pick::Set& pointset )
{
    BufferString currentname = pointset.name(), tempname;
    int filenumber = 1;

    const CtxtIOObj ctio( mIOObjContext(PickSet) );
    const IOObjContext ctxt = ctio.ctxt_;
    const IODir iodir( ctxt.getSelKey() ) ;
    const IODirEntryList pointsetlist( iodir, ctxt );
    BufferStringSet existingpicksetnames;
    pointsetlist.getIOObjNames( existingpicksetnames );

    while ( filenumber<999 )
    {
	tempname = currentname;
	tempname.add( " (" ).add( filenumber++ ).add(")");
	if ( !existingpicksetnames.isPresent(tempname) )
	    break;
    }

    if ( filenumber==999 )
	return false;

    pointset.setName( tempname );
    return true;
}


static IOObj* mkEntry( const CtxtIOObj& ctio, const char* nm )
{
    CtxtIOObj newctio( ctio );
    newctio.ioobj_ = nullptr;
    newctio.setName( nm );
    newctio.fillObj();
    return newctio.ioobj_;
}


uiRetVal uiPickSetImportGroup::addPointSet( Pick::Set& pointset, IOObj* ioobj,
					    OD::Color col )
{
    const int seloption = shapefld_->getIntValue();
    const bool ispoly = seloption==1 || seloption==2;
    const int pointsetsize = pointset.size();
    const BufferString pointsetname = pointset.getName();

    if ( (pointsetsize < 2 && seloption==1)
	 || (pointsetsize < 3 && seloption==2) )
    {
	const uiString msg = tr("- Cannot import %1 since it has %2 point%3 "
				"but needs at least %4 points to be imported "
				"as a %5")
			       .arg( pointsetname.buf() ).arg( pointsetsize )
			       .arg( pointsetsize == 1 ? "" : "s" )
			       .arg( seloption == 1 ? "2" : "3" )
			       .arg( getPointSetShape() );
	return msg;
    }

    if ( zfld_->box()->currentItem() == 2 )
	serv_->fillZValsFromHor( pointset, horinpfld_->box()->currentItem() );

    if ( !ioobj )
    {
	CtxtIOObj ctio( mIOObjContext(PickSet) );
	ioobj = mkEntry( ctio, pointsetname.buf() );

	pointset.setDefaultDispPars( ispoly );
    }

    pointset.disp_.color_ = col;

    if ( ispoly )
    {
	pointset.disp_.connect_ = seloption==1
					? Pick::Set::Disp::Connection::Open
					: Pick::Set::Disp::Connection::Close;
	pointset.disp_.linestyle_.color_ = col;
	ioobj->pars().set( sKey::Type(), sKey::Polygon() );

	if ( seloption==2 )
	    pointset.findStartIdxs();
    }
    else
	ioobj->pars().set(sKey::Type(), PickSetTranslatorGroup::sKeyPickSet());

    IOM().commitChanges( *ioobj );
    uiString errmsg;
    if ( !PickSetTranslator::store(pointset,ioobj,errmsg) )
	 return errmsg;

    storedIDs().add( ioobj->key() );
    return uiRetVal::OK();
}


void uiPickSetImportGroup::displayPointSet( Pick::Set& ps, MultiID mid )
{
    Pick::SetMgr& psmgr = Pick::Mgr();
    int setidx = psmgr.indexOf( mid );
    if ( setidx < 0 )
    {
	auto* newps = new Pick::Set( ps );
	psmgr.set( mid, newps );
	setidx = psmgr.indexOf( mid );
	importready_ = true;
    }
    else
    {
	RefMan<Pick::Set> oldps = psmgr.get( setidx );
	*oldps = ps;
	psmgr.reportChange( nullptr, *oldps );
	psmgr.reportDispChange( nullptr, *oldps );
    }

    psmgr.setUnChanged( setidx, true );
}


bool uiPickSetImportGroup::isPointSet() const
{
    return shapefld_ && shapefld_->getIntValue()==0;
}


bool uiPickSetImportGroup::isPolyLine() const
{
    return shapefld_ && shapefld_->getIntValue()==1;
}


bool uiPickSetImportGroup::isPolygon() const
{
    return shapefld_ && shapefld_->getIntValue()==2;
}


uiString uiPickSetImportGroup::getPointSetShape()
{
    switch ( shapefld_->getIntValue() )
    {
	case 0: return uiStrings::sPointSet();
	case 1: return uiStrings::sPolyLine();
	case 2: return uiStrings::sPolygon();
	default: return uiStrings::sEmptyString();
    }
}


int uiPickSetImportGroup::getImportCount()
{
    return impcount_;
}


void uiPickSetImportGroup::resetImportCount()
{
    impcount_ = 0;
}


static const char* zoptions[] =
{
    "Input file",
    "Constant Z",
    "Horizon",
    nullptr
};


static uiString sPicksetPolygon() { return toUiString("Pointset/Polygon"); }

uiSinglePickSetImportGroup::uiSinglePickSetImportGroup( uiParent* p )
    : uiPickSetImportGroup(p)
    , fd_(PickSetAscIO::getDesc(true))
{
    filefld_ = new uiASCIIFileInput( this, true );
    filefld_->setSelectMode( uiFileDialog::ExistingFile );
    filefld_->setDefaultSelectionDir( GetSurveyProcDir() );
    mAttachCB( filefld_->valueChanged, uiSinglePickSetImportGroup::inputChgd );
    setHAlignObj( filefld_ );

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.forread_ = false;

    zfld_ = new uiLabeledComboBox( this, zoptions, tr("Get Z values from") );
    mAttachCB( zfld_->box()->selectionChanged,
	       uiSinglePickSetImportGroup::formatSelCB );
    zfld_->attach( alignedBelow, filefld_ );

    const uiString constzlbl = tr("Specify constant Z value %1")
				.arg( SI().getZUnitString() );
    constzfld_ = new uiGenInput( this, constzlbl, FloatInpSpec(0) );
    constzfld_->attach( rightTo, zfld_ );
    constzfld_->display( zfld_->box()->currentItem() == 1 );

    horinpfld_ = new uiLabeledComboBox( this,
			    uiStrings::phrSelect(uiStrings::sHorizon()) );
    horinpfld_->attach( rightTo, zfld_ );
    horinpfld_->display( zfld_->box()->currentItem() == 2 );
    horinpfld_->box()->setHSzPol( uiObject::MedVar );

    const uiStringSet impoptions { uiStrings::sPointSet(),
				   uiStrings::sPolyLine(),
				   uiStrings::sPolygon() };
    shapefld_ = new uiGenInput( this, tr("Import as"),
			       StringListInpSpec(impoptions) );
    shapefld_->attach( alignedBelow, zfld_ );

    auto* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, shapefld_ );

    dataselfld_ = new uiTableImpDataSel( this, *fd_,
		      mODHelpKey(mTableImpDataSelpicksHelpID) );
    dataselfld_->attach( alignedBelow, shapefld_ );
    dataselfld_->attach( ensureBelow, sep );

    sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, dataselfld_ );

    const auto label = uiStrings::phrOutput( uiStrings::sPicksetPolygon() );
    uiIOObjSel::Setup ioobjsetup( label );
    ioobjsetup.withinserters(false).withwriteopts(false);
    objfld_ = new uiIOObjSel( this, ctxt, ioobjsetup );
    objfld_->attach( alignedBelow, dataselfld_ );
    objfld_->attach( ensureBelow, sep );

    colorfld_ = new uiColorInput( this,
				  uiColorInput::Setup(OD::getRandStdDrawColor())
						.lbltxt(uiStrings::sColor()) );
    colorfld_->attach( alignedBelow, objfld_ );
}


uiSinglePickSetImportGroup::~uiSinglePickSetImportGroup()
{
    detachAllNotifiers();
    delete fd_;
}


void uiSinglePickSetImportGroup::inputChgd( CallBacker* )
{
    storedid_.setUdf();
    const FilePath fnmfp( filefld_->fileName() );
    objfld_->setInputText( fnmfp.baseName() );
}


void uiSinglePickSetImportGroup::formatSelCB( CallBacker* )
{
    const int zchoice = zfld_->box()->currentItem();
    const bool iszreq = zchoice == 0;
    constzfld_->display( zchoice == 1 );
    horinpfld_->display( zchoice == 2 );
    PickSetAscIO::updateDesc( *fd_, iszreq );
    dataselfld_->updateSummary();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiSinglePickSetImportGroup::doImport( bool display_on_import )
{
    if ( !checkInpFlds() )
	return false;

    const char* fname = filefld_->fileName();
    od_istream strm( fname );
    if ( !strm.isOK() )
	mErrRet( tr("Could not open input file") );

    const char* psnm = objfld_->getInput();
    const int seloption = shapefld_->getIntValue();
    const bool ispoly = seloption == 1 || seloption == 2;
    RefMan<Pick::Set> ps = new Pick::Set( psnm, ispoly );
    const int zchoice = zfld_->box()->currentItem();
    float constz = zchoice==1 ? constzfld_->getFValue() : 0;
    if ( SI().zIsTime() )
	constz /= 1000;

    PickSetAscIO aio( *fd_ );
    aio.get( strm, *ps, zchoice==0, constz );

    if ( zchoice == 2 )
	serv_->fillZValsFromHor( *ps, horinpfld_->box()->currentItem() );

    const IOObj* objfldioobj = objfld_->ioobj();
    if ( !objfldioobj )
	return false;

    PtrMan<IOObj> ioobj = objfldioobj->clone();
    auto ret = addPointSet( *ps, ioobj.ptr(), colorfld_->color() );
    if ( !ret.isOK() )
	mErrRet( ret );

    if ( display_on_import )
	displayPointSet( *ps, storedid_ );

    impcount_++;

    return true;
}


bool uiSinglePickSetImportGroup::checkInpFlds()
{
    const BufferString filenm = filefld_->fileName();
    if ( !File::exists(filenm) )
	mErrRet( tr("Please select input file") );

    objfld_->reset();
    if ( !objfld_->ioobj() )
	return false;

    if ( !dataselfld_->commit() )
	mErrRet( tr("Please specify data format") );

    const int zchoice = zfld_->box()->currentItem();
    if ( zchoice == 1 )
    {
	float constz = constzfld_->getFValue();
	if ( SI().zIsTime() )
	    constz /= 1000;

	if ( !SI().zRange(false).includes( constz,false ) )
	    mErrRet( tr("Please enter a valid Z value") );
    }

    return true;
}


TypeSet<MultiID> uiSinglePickSetImportGroup::storedIDs() const
{
    TypeSet<MultiID> ids;
    ids.add( storedid_ );
    return ids;
}



uiImportPickSet::uiImportPickSet( uiParent* p, uiPickPartServer* pps )
    : uiDialog(p,Setup( uiStrings::phrImport(sPicksetPolygon()),
			mODHelpKey(mImpPickSetHelpID) ).modal(false) )
    , importReady(this)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );

    const auto& fact = uiPickSetImportGroup::factory();
    const uiStringSet& options = fact.getUserNames();

    uiObject* topobj = nullptr;
    if ( options.size()>1 )
    {
	auto* lcc = new uiLabeledComboBox( this, tr("Import"),
					   "import_options" );
	optionfld_ = lcc->box();
	optionfld_->addItems( options );
	optionfld_->setHSzPol( uiObject::MedVar );
	optionfld_->setSensitive( options.size()!=1 );
	topobj = lcc->attachObj();
	mAttachCB( optionfld_->selectionChanged, uiImportPickSet::optionSelCB );
    }
    else
    {
	auto* label = new uiLabel( this, tr("Import Single PointSet") );
	topobj = label;
    }

    const BufferStringSet& names = fact.getNames();
    for ( const auto* name : names )
    {
	auto* grp = fact.create( name->buf(), this );
	if ( !grp )
	    continue;

	grp->setPartServer( pps );
	grp->init();
	groups_.add( grp );
	grp->attach( alignedBelow, topobj );
    }

    enableSaveButton( tr("Display after import") );

    mAttachCB( postFinalize(), uiImportPickSet::optionSelCB );
}


uiImportPickSet::~uiImportPickSet()
{
    detachAllNotifiers();
}


TypeSet<MultiID> uiImportPickSet::getStoredIDs() const
{
    TypeSet<MultiID> ids;
    const int sel = optionfld_ ? optionfld_->currentItem() : 0;
    if ( !groups_.validIdx(sel) )
	return ids;

    return groups_[sel]->storedIDs();
}


void uiImportPickSet::optionSelCB( CallBacker* )
{
    const int sel = optionfld_ ? optionfld_->currentItem() : 0;
    for ( int idx=0; idx<groups_.size(); idx++ )
	groups_[idx]->display( sel==idx );

    //Do not display the save button if there are more than 1 pointsets
    const bool display = sel==0;
    setButtonSensitive( SAVE, display );
    displayButton( SAVE, display );
}


bool uiImportPickSet::acceptOK( CallBacker* )
{
    uiMsgMainWinSetter mws( this );

    const int sel = optionfld_ ? optionfld_->currentItem() : 0;
    if ( !groups_.validIdx(sel) )
	return false;

    auto* selgroup = groups_[sel];
    const bool res = selgroup->doImport( saveButtonChecked() );
    if ( !res )
	return false;

    if ( selgroup->triggerImportReady() )
	importReady.trigger();

    const int impcount = selgroup->getImportCount();
    const auto shape = selgroup->getPointSetShape();

    if ( impcount > 0 )
    {
	selgroup->resetImportCount();

	const auto msg = tr("%1 %2%3 successfully imported.\n\n"
			    "Do you want to import more PointSet(s)?")
				.arg( impcount )
				.arg( shape )
				.arg( impcount==1 ? uiStrings::sEmptyString()
						  : tr("s") );

	return !uiMSG().askGoOn( msg, uiStrings::sYes(),
				      tr("No, close window") );
    }
    else
	mErrRet( tr("Not able to import the %1(s) requested").arg(shape) );
}


uiExportPickSet::uiExportPickSet( uiParent* p )
    : uiDialog(p,Setup( uiStrings::phrExport(sPicksetPolygon()),
			mODHelpKey(mExpPickSetHelpID) ).modal(false) )
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.forread_ = true;

    const uiString label = uiStrings::phrInput( sPicksetPolygon() );
    uiIOObjSel::Setup ioobjsetup( label );
    ioobjsetup.withinserters(false).withwriteopts(false);
    objfld_ = new uiIOObjSel( this, ctxt, ioobjsetup );

    filefld_ = new uiASCIIFileInput( this, false );
    filefld_->attach( alignedBelow, objfld_ );

    if ( SI().hasProjection() )
    {
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach( alignedBelow, filefld_ );
    }
}


uiExportPickSet::~uiExportPickSet()
{}


bool uiExportPickSet::doExport()
{
    const IOObj* objfldioobj = objfld_->ioobj();
    if ( !objfldioobj )
	return false;

    PtrMan<IOObj> ioobj = objfldioobj->clone();
    uiString errmsg;
    RefMan<Pick::Set> ps = new Pick::Set;
    if ( !PickSetTranslator::retrieve(*ps,ioobj.ptr(),true,errmsg) )
	mErrRet( errmsg );

    const char* fname = filefld_->fileName();
    od_ostream strm( fname );
    if ( !strm.isOK() )
    {
	mErrRet(uiStrings::phrCannotOpen(uiStrings::phrOutput(
	    uiStrings::sFile())));
    }

    strm.stdStream() << std::fixed;
    BufferString buf;
    for ( int locidx=0; locidx<ps->size(); locidx++ )
    {
	ps->get(locidx).toString( buf, true, coordsysselfld_ ?
			    coordsysselfld_->getCoordSystem().ptr() : nullptr );
	strm << buf.buf() << '\n';
    }

    strm << od_endl;
    return true;
}


bool uiExportPickSet::checkInpFlds()
{
    BufferString filenm = filefld_->fileName();
    if ( filenm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() );

    objfld_->reset();
    if ( !objfld_->ioobj() )
	return false;

    return true;
}


bool uiExportPickSet::acceptOK( CallBacker* )
{
    uiMsgMainWinSetter mws( this );

    if ( !checkInpFlds() )
	return false;

    if ( !doExport() )
	return false;

    const uiString msg = tr("Data successfully exported.\n\n"
			"Do you want to export more PointSets or Polygons?");
    return !uiMSG().askGoOn( msg, uiStrings::sYes(), tr("No, close window") );
}


uiImpExpPickSet::uiImpExpPickSet(uiParent* p, uiPickPartServer* pps, bool imp )
    : uiDialog(p,Setup(imp ? uiStrings::phrImport(sPicksetPolygon() )
			   : uiStrings::phrExport(sPicksetPolygon() ),
		       imp ? mODHelpKey(mImpPickSetHelpID)
			   : mODHelpKey(mExpPickSetHelpID)).modal(false))
    , serv_(pps)
    , import_(imp)
    , fd_(*PickSetAscIO::getDesc(true))
    , zfld_(nullptr)
    , constzfld_(nullptr)
    , dataselfld_(nullptr)
    , coordsysselfld_(nullptr)
    , importReady(this)
    , storedid_(MultiID::udf())
    , polyfld_(nullptr)
{
    setOkCancelText( import_ ? uiStrings::sImport() : uiStrings::sExport(),
		     uiStrings::sClose() );
    if ( import_ ) enableSaveButton( tr("Display after import") );

    filefld_ = new uiASCIIFileInput( this, import_ );
    if ( import_ )
	filefld_->valueChanged.notify( mCB(this,uiImpExpPickSet,inputChgd) );

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.forread_ = !import_;

    uiString label = import_
	? uiStrings::phrOutput( sPicksetPolygon() )
	: uiStrings::phrInput( sPicksetPolygon() );
    uiIOObjSel::Setup ioobjsetup( label );
    ioobjsetup.withinserters(false).withwriteopts(false);
    objfld_ = new uiIOObjSel( this, ctxt, ioobjsetup );

    if ( import_ )
    {
	zfld_ = new uiLabeledComboBox( this, zoptions,tr("Get Z values from") );
	zfld_->box()->selectionChanged.notify( mCB(this,uiImpExpPickSet,
				formatSel) );
	zfld_->attach( alignedBelow, filefld_ );

	uiString constzlbl = tr("Specify constant Z value %1")
				.arg( SI().getZUnitString() );
	constzfld_ = new uiGenInput( this, constzlbl, FloatInpSpec(0) );
	constzfld_->attach( rightTo, zfld_ );
	constzfld_->display( zfld_->box()->currentItem() == 1 );

	horinpfld_ = new uiLabeledComboBox( this,
			    uiStrings::phrSelect( uiStrings::sHorizon() ) );
	serv_->fetchHors( false );
	const ObjectSet<SurfaceInfo> hinfos = serv_->horInfos();
	for ( int idx=0; idx<hinfos.size(); idx++ )
	    horinpfld_->box()->addItem( toUiString(hinfos[idx]->name_) );
	horinpfld_->attach( rightTo, zfld_ );
	horinpfld_->display( zfld_->box()->currentItem() == 2 );

	uiStringSet impoptions;
	impoptions.add( uiStrings::sPointSet() ).add( uiStrings::sPolyLine() )
		  .add( uiStrings::sPolygon() );
	polyfld_ = new uiGenInput( this, tr("Import as"),
				   StringListInpSpec(impoptions) );
	polyfld_->attach( alignedBelow, zfld_ );

	uiSeparator* sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, polyfld_ );

	dataselfld_ = new uiTableImpDataSel( this, fd_,
		      mODHelpKey(mTableImpDataSelpicksHelpID) );
	dataselfld_->attach( alignedBelow, polyfld_ );
	dataselfld_->attach( ensureBelow, sep );

	sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, dataselfld_ );

	objfld_->attach( alignedBelow, dataselfld_ );
	objfld_->attach( ensureBelow, sep );

	colorfld_ = new uiColorInput( this,
				 uiColorInput::Setup(OD::getRandStdDrawColor()).
				   lbltxt(uiStrings::sColor()) );
	colorfld_->attach( alignedBelow, objfld_ );
    }
    else
    {
	filefld_->attach( alignedBelow, objfld_ );
	if ( SI().hasProjection() )
	{
	    coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	    coordsysselfld_->attach(alignedBelow, filefld_);
	}
    }
}


uiImpExpPickSet::~uiImpExpPickSet()
{}


void uiImpExpPickSet::inputChgd( CallBacker* )
{
    storedid_.setUdf();
    FilePath fnmfp( filefld_->fileName() );
    objfld_->setInputText( fnmfp.baseName() );
}


void uiImpExpPickSet::formatSel( CallBacker* )
{
    const int zchoice = zfld_->box()->currentItem();
    const bool iszreq = zchoice == 0;
    constzfld_->display( zchoice == 1 );
    horinpfld_->display( zchoice == 2 );
    PickSetAscIO::updateDesc( fd_, iszreq );
    dataselfld_->updateSummary();
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiImpExpPickSet::doImport()
{
    const char* fname = filefld_->fileName();
    od_istream strm( fname );
    if ( !strm.isOK() )
	mErrRet( tr("Could not open input file") )

    const char* psnm = objfld_->getInput();
    RefMan<Pick::Set> ps = new Pick::Set( psnm );
    const int zchoice = zfld_->box()->currentItem();
    float constz = zchoice==1 ? constzfld_->getFValue() : 0;
    if ( SI().zIsTime() )
	constz /= 1000;

    ps->disp_.color_ = colorfld_->color();
    PickSetAscIO aio( fd_ );
    aio.get( strm, *ps, zchoice==0, constz );

    if ( zchoice == 2 )
	serv_->fillZValsFromHor( *ps, horinpfld_->box()->currentItem() );

    const IOObj* objfldioobj = objfld_->ioobj();
    if ( !objfldioobj )
	return false;

    PtrMan<IOObj> ioobj = objfldioobj->clone();
    const int seloption = polyfld_->getIntValue();
    if ( seloption==0 )
    {
	ps->disp_.connect_ = Pick::Set::Disp::None;
	ioobj->pars().set(sKey::Type(), PickSetTranslatorGroup::sKeyPickSet());
    }
    else
    {
	ps->disp_.connect_ = seloption==1 ? Pick::Set::Disp::Open
					  : Pick::Set::Disp::Close;
	ps->disp_.linestyle_.color_ = colorfld_->color();
	ioobj->pars().set( sKey::Type(), sKey::Polygon() );

	if ( seloption==2 )
	    ps->findStartIdxs();
    }

    IOM().commitChanges( *ioobj );
    uiString errmsg;
    if ( !PickSetTranslator::store(*ps,ioobj.ptr(),errmsg) )
	mErrRet( errmsg )

    storedid_ = ioobj->key();
    if ( saveButtonChecked() )
    {
	Pick::SetMgr& psmgr = Pick::Mgr();
	int setidx = psmgr.indexOf( storedid_ );
	if ( setidx < 0 )
	{
	    Pick::Set* newps = new Pick::Set( *ps );
	    psmgr.set( storedid_, newps );
	    setidx = psmgr.indexOf( storedid_ );
	    importReady.trigger();
	}
	else
	{
	    RefMan<Pick::Set> oldps = psmgr.get( setidx );
	    *oldps = *ps;
	    psmgr.reportChange( nullptr, *oldps );
	    psmgr.reportDispChange( nullptr, *oldps );
	}

	psmgr.setUnChanged( setidx, true );
    }

    return true;
}


bool uiImpExpPickSet::doExport()
{
    const IOObj* objfldioobj = objfld_->ioobj();
    if ( !objfldioobj )
	return false;

    PtrMan<IOObj> ioobj = objfldioobj->clone();
    uiString errmsg;
    RefMan<Pick::Set> ps = new Pick::Set;
    if ( !PickSetTranslator::retrieve(*ps,ioobj.ptr(),true,errmsg) )
	mErrRet( errmsg )

    const char* fname = filefld_->fileName();
    od_ostream strm( fname );
    if ( !strm.isOK() )
    {
	mErrRet(uiStrings::phrCannotOpen(uiStrings::phrOutput(
		uiStrings::sFile())))
    }

    strm.stdStream() << std::fixed;
    BufferString buf;
    for ( int locidx=0; locidx<ps->size(); locidx++ )
    {
	ps->get(locidx).toString( buf, true, coordsysselfld_ ?
			coordsysselfld_->getCoordSystem().ptr() : nullptr );
	strm << buf.buf() << '\n';
    }

    strm << od_endl;
    return true;
}


bool uiImpExpPickSet::acceptOK( CallBacker* )
{
    uiMsgMainWinSetter mws( this );

    if ( !checkInpFlds() ) return false;
    bool ret = import_ ? doImport() : doExport();
    if ( !ret ) return false;

    const bool ispolygon = polyfld_ && polyfld_->isChecked();
    uiString msg = tr("%1 successfully %2."
		      "\n\nDo you want to %3 more %4?")
		 .arg( import_ ? (ispolygon ? uiStrings::sPolygon()
					    : uiStrings::sPointSet())
			      : tr("Pointset/Polygon") )
		 .arg( import_ ? tr("imported") : tr("exported") )
		 .arg( import_ ? (uiStrings::sImport()).toLower()
			       : (uiStrings::sExport()).toLower() )
		 .arg( import_ ? (ispolygon ? (uiStrings::sPolygon(2)).toLower()
					   :(uiStrings::sPointSet(2)).toLower())
					    : tr("pointsets/polygons") );
    return !uiMSG().askGoOn( msg, uiStrings::sYes(), tr("No, close window") );
}


bool uiImpExpPickSet::checkInpFlds()
{
    BufferString filenm = filefld_->fileName();
    if ( import_ && !File::exists(filenm) )
	mErrRet( tr("Please select input file") )

    if ( !import_ && filenm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() )

    objfld_->reset();
    if ( !objfld_->ioobj() )
	return false;

    if ( import_ )
    {
	if ( !dataselfld_->commit() )
	    mErrRet( tr("Please specify data format") )

	const int zchoice = zfld_->box()->currentItem();
	if ( zchoice == 1 )
	{
	    float constz = constzfld_->getFValue();
	    if ( SI().zIsTime() ) constz /= 1000;

	    if ( !SI().zRange(false).includes( constz,false ) )
		mErrRet( tr("Please enter a valid Z value") )
	}
    }

    return true;
}

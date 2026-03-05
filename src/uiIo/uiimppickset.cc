/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiimppickset.h"

#include "uicolor.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
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
#include "tabledef.h"
#include "od_helpids.h"
#include "picklocation.h"

#include <math.h>

mImplFactory1Param( uiPickSetImportGroup, uiParent*,
		    uiPickSetImportGroup::factory );

uiPickSetImportGroup::uiPickSetImportGroup( uiParent* p )
    : uiGroup(p)
{
}


uiPickSetImportGroup::~uiPickSetImportGroup()
{
}


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


bool uiPickSetImportGroup::isPolygon() const
{
    return polyfld_ && polyfld_->isChecked();
}


static const char* zoptions[] =
{
    "Input file",
    "Constant Z",
    "Horizon",
    0
};



static uiString sPicksetPolygon() { return toUiString("Pointset/Polygon"); }

uiSinglePickSetImportGroup::uiSinglePickSetImportGroup( uiParent* p )
    : uiPickSetImportGroup(p)
    , fd_(*PickSetAscIO::getDesc(true))
{
    filefld_ = new uiASCIIFileInput( this, true );
    mAttachCB( filefld_->valueChanged, uiSinglePickSetImportGroup::inputChgd );
    setHAlignObj( filefld_ );

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.forread_ = false;

    zfld_ = new uiLabeledComboBox( this, zoptions, tr("Get Z values from") );
    zfld_->box()->selectionChanged.notify(
		mCB(this,uiSinglePickSetImportGroup,formatSel) );
    zfld_->attach( alignedBelow, filefld_ );

    uiString constzlbl = tr("Specify constant Z value %1")
				.arg( SI().getZUnitString() );
    constzfld_ = new uiGenInput( this, constzlbl, FloatInpSpec(0) );
    constzfld_->attach( rightTo, zfld_ );
    constzfld_->display( zfld_->box()->currentItem() == 1 );

    horinpfld_ = new uiLabeledComboBox( this,
			    uiStrings::phrSelect(uiStrings::sHorizon()) );
    horinpfld_->attach( rightTo, zfld_ );
    horinpfld_->display( zfld_->box()->currentItem() == 2 );
    horinpfld_->box()->setHSzPol( uiObject::MedVar );

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

    uiString label = uiStrings::phrOutput( sPicksetPolygon() );
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
}


void uiSinglePickSetImportGroup::inputChgd( CallBacker* )
{
    storedid_.setUdf();
    FilePath fnmfp( filefld_->fileName() );
    objfld_->setInputText( fnmfp.baseName() );
}


void uiSinglePickSetImportGroup::formatSel( CallBacker* )
{
    const int zchoice = zfld_->box()->currentItem();
    const bool iszreq = zchoice == 0;
    constzfld_->display( zchoice == 1 );
    horinpfld_->display( zchoice == 2 );
    PickSetAscIO::updateDesc( fd_, iszreq );
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
	mErrRet( tr("Could not open input file") )

    const char* psnm = objfld_->getInput();
    const int seloption = polyfld_->getIntValue();
    const bool ispoly = seloption == 1 || seloption == 2;
    RefMan<Pick::Set> ps = new Pick::Set( psnm, ispoly );
    const int zchoice = zfld_->box()->currentItem();
    float constz = zchoice==1 ? constzfld_->getFValue() : 0;
    if ( SI().zIsTime() )
	constz /= 1000;

    ps->disp3d().markerstyle_.color_ = colorfld_->color();
    PickSetAscIO aio( fd_ );
    aio.get( strm, *ps, zchoice==0, constz );

    if ( zchoice == 2 )
	serv_->fillZValsFromHor( *ps, horinpfld_->box()->currentItem() );

    const IOObj* objfldioobj = objfld_->ioobj();
    if ( !objfldioobj )
	return false;

    PtrMan<IOObj> ioobj = objfldioobj->clone();
    if ( seloption==0 )
	ioobj->pars().set(sKey::Type(), PickSetTranslatorGroup::sKeyPickSet());
    else
    {
	ps->disp3d().polyDisp()->connect_ = seloption==1
					  ? Pick::Set::Connection::Open
					  : Pick::Set::Connection::Close;
	ps->disp3d().polyDisp()->linestyle_.color_ = colorfld_->color();
	ioobj->pars().set( sKey::Type(), sKey::Polygon() );

	if ( seloption==2 )
	    ps->findStartIdxs();
    }

    IOM().commitChanges( *ioobj );
    uiString errmsg;
    if ( !PickSetTranslator::store(*ps,ioobj.ptr(),errmsg) )
	mErrRet( errmsg )

    storedid_ = ioobj->key();
    if ( display_on_import )
    {
	Pick::SetMgr& psmgr = Pick::Mgr();
	int setidx = psmgr.indexOf( storedid_ );
	if ( setidx < 0 )
	{
	    Pick::Set* newps = new Pick::Set( *ps );
	    psmgr.set( storedid_, newps );
	    setidx = psmgr.indexOf( storedid_ );
	    importready_ = true;
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


bool uiSinglePickSetImportGroup::checkInpFlds()
{
    BufferString filenm = filefld_->fileName();
    if ( !File::exists(filenm) )
	mErrRet( tr("Please select input file") )

    objfld_->reset();
    if ( !objfld_->ioobj() )
	return false;

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

    return true;
}


TypeSet<MultiID> uiSinglePickSetImportGroup::storedIDs() const
{
    TypeSet<MultiID> ids;
    ids.add( storedid_ );
    return ids;
}



uiImportPickSet::uiImportPickSet( uiParent* p, uiPickPartServer* pps )
    : uiDialog( p,
		Setup( uiStrings::phrImport(sPicksetPolygon()),
		       mODHelpKey(mImpPickSetHelpID) ).modal(false) )
    , importReady(this)
{
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );
    enableSaveButton( tr("Display after import") );

    const auto& fact = uiPickSetImportGroup::factory();
    const uiStringSet& options = fact.getUserNames();
    auto* lcc = new uiLabeledComboBox( this, tr("Import"), "import_options" );
    optionfld_ = lcc->box();
    optionfld_->addItems( options );

    const BufferStringSet& names = fact.getNames();
    for ( const auto* name : names )
    {
	auto* grp = fact.create( name->buf(), this );
	if ( !grp )
	    continue;

	grp->setPartServer( pps );
	grp->init();
	groups_.add( grp );
	grp->attach( alignedBelow, lcc );
    }

    mAttachCB( postFinalize(), uiImportPickSet::optionSelCB );
}


uiImportPickSet::~uiImportPickSet()
{
    detachAllNotifiers();
}


MultiID uiImportPickSet::getStoredID() const
{
    const int sel = optionfld_->currentItem();
    if ( !groups_.validIdx(sel) )
	return MultiID::udf();

    // TODO: Properly support multiple ids
    const TypeSet<MultiID> ids = groups_[sel]->storedIDs();
    return ids.isEmpty() ? MultiID::udf() : ids.first();
}


void uiImportPickSet::optionSelCB( CallBacker* )
{
    const int sel = optionfld_->currentItem();
    for ( int idx=0; idx<groups_.size(); idx++ )
	groups_[idx]->display( sel==idx );
}


bool uiImportPickSet::acceptOK( CallBacker* )
{
    uiMsgMainWinSetter mws( this );

    const int sel = optionfld_->currentItem();
    if ( !groups_.validIdx(sel) )
	return false;

    auto* selgroup = groups_[sel];
    const bool res = selgroup->doImport( saveButtonChecked() );
    if ( !res )
	return false;

    if ( selgroup->triggerImportReady() )
	importReady.trigger();

    const bool ispolygon = selgroup->isPolygon();
    uiString msg = tr("%1 successfully imported."
		      "\n\nDo you want to import more PointSets or Polygons?")
		 .arg( ispolygon ? uiStrings::sPolygon()
				 : uiStrings::sPointSet() );
    return !uiMSG().askGoOn( msg, uiStrings::sYes(), tr("No, close window") );
}



uiExportPickSet::uiExportPickSet( uiParent* p )
    : uiDialog( p,
		Setup( uiStrings::phrExport(sPicksetPolygon()),
		       mODHelpKey(mExpPickSetHelpID) ).modal(false) )
{
    setOkCancelText( uiStrings::sExport(), uiStrings::sClose() );

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.forread_ = true;

    uiString label = uiStrings::phrInput( sPicksetPolygon() );
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


bool uiExportPickSet::checkInpFlds()
{
    BufferString filenm = filefld_->fileName();
    if ( filenm.isEmpty() )
	mErrRet( uiStrings::sSelOutpFile() )

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

    uiString msg = tr("Data successfully exported."
		      "\n\nDo you want to export more PointSets or Polygons?");
    return !uiMSG().askGoOn( msg, uiStrings::sYes(), tr("No, close window") );
}

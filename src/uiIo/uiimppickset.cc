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

static const char* zoptions[] =
{
    "Input file",
    "Constant Z",
    "Horizon",
    0
};


uiImpExpPickSet::uiImpExpPickSet(uiParent* p, uiPickPartServer* pps, bool imp )
    : uiDialog(p,uiDialog::Setup(imp
				     ? uiStrings::phrImport(sPicksetPolygon())
				     : uiStrings::phrExport(sPicksetPolygon()),
				 mNoDlgTitle,
				 imp
					? mODHelpKey(mImpPickSetHelpID)
					: mODHelpKey(mExpPickSetHelpID) )
				.modal(false))
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
	constzfld_->attach( alignedBelow, zfld_ );
	constzfld_->display( zfld_->box()->currentItem() == 1 );

	horinpfld_ = new uiLabeledComboBox( this,
			    uiStrings::phrSelect( uiStrings::sHorizon() ) );
	serv_->fetchHors( false );
	const ObjectSet<SurfaceInfo> hinfos = serv_->horInfos();
	for ( int idx=0; idx<hinfos.size(); idx++ )
	    horinpfld_->box()->addItem( toUiString(hinfos[idx]->name) );
	horinpfld_->attach( alignedBelow, zfld_ );
	horinpfld_->display( zfld_->box()->currentItem() == 2 );

	uiSeparator* sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, constzfld_ );

	dataselfld_ = new uiTableImpDataSel( this, fd_,
		      mODHelpKey(mTableImpDataSelpicksHelpID) );
	dataselfld_->attach( alignedBelow, constzfld_ );
	dataselfld_->attach( ensureBelow, sep );

	sep = new uiSeparator( this, "H sep" );
	sep->attach( stretchedBelow, dataselfld_ );

	objfld_->attach( alignedBelow, constzfld_ );
	objfld_->attach( ensureBelow, sep );

	colorfld_ = new uiColorInput( this,
				 uiColorInput::Setup(OD::getRandStdDrawColor()).
				   lbltxt(uiStrings::sColor()) );
	colorfld_->attach( alignedBelow, objfld_ );

	uiStringSet impoptions;
	impoptions.add( uiStrings::sPointSet() ).add( uiStrings::sPolyLine() )
		  .add( uiStrings::sPolygon() );
	polyfld_ = new uiGenInput( this, tr("Import as"),
				   StringListInpSpec(impoptions) );
	polyfld_->attach( rightTo, colorfld_ );
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
    if ( SI().zIsTime() ) constz /= 1000;

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
    if ( seloption )
    {
	ps->disp_.connect_ = seloption==1 ? Pick::Set::Disp::Open
					 : Pick::Set::Disp::Close;
	ps->disp_.linestyle_.color_ = colorfld_->color();
	ioobj->pars().set( sKey::Type(), sKey::Polygon() );
    }
    else
    {
	ps->disp_.connect_ = Pick::Set::Disp::None;
	ioobj->pars().set(sKey::Type(), PickSetTranslatorGroup::sKeyPickSet());
    }

    IOM().commitChanges( *ioobj );
    BufferString errmsg;
    if ( !PickSetTranslator::store(*ps,ioobj,errmsg) )
	mErrRet( toUiString(errmsg) )

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
    if ( !objfldioobj ) return false;

    PtrMan<IOObj> ioobj = objfldioobj->clone();
    BufferString errmsg;
    RefMan<Pick::Set> ps = new Pick::Set;
    if ( !PickSetTranslator::retrieve(*ps,ioobj,true, errmsg) )
	mErrRet( toUiString(errmsg) )

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
	ps->get(locidx).toString( buf, true,
		coordsysselfld_ ? coordsysselfld_->getCoordSystem() : nullptr );
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

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uiimppickset.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uipickpartserv.h"
#include "uiseparator.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"

#include "ioobjctxt.h"
#include "file.h"
#include "filepath.h"
#include "ioobj.h"
#include "picksetmanager.h"
#include "picksettr.h"
#include "picksetascio.h"
#include "randcolor.h"
#include "od_istream.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "od_iostream.h"
#include "od_helpids.h"
#include "picklocation.h"

#include <math.h>

static uiStringSet zoptions()
{
    uiStringSet uistrset;
    uistrset.add(uiStrings::sInputFile());
    uistrset.add(od_static_tr("zoptions","Constant Z")),
    uistrset.add(uiStrings::sHorizon());
    uistrset.add(uiString::empty());
    return uistrset;
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
    , zfld_(0)
    , constzfld_(0)
    , dataselfld_(0)
    , importReady(this)
{
    setOkCancelText( import_ ? uiStrings::sImport() : uiStrings::sExport(),
		     uiStrings::sClose() );
    if ( import_ )
	enableSaveButton( tr("Display after import") );

    const uiString tp = uiStrings::sASCIIFile();
    uiString label = import_
	? uiStrings::sInputASCIIFile()
	: uiStrings::sOutputASCIIFile();

    uiFileSel::Setup fssu;
    fssu.setForWrite( !import_ );
    filefld_ = new uiFileSel( this, label, fssu );
    if ( import_ )
	filefld_->newSelection.notify( mCB(this,uiImpExpPickSet,inputChgd) );

    IOObjContext ctxt( mIOObjContext(PickSet) );
    ctxt.forread_ = !import_;
    label = import_
	? uiStrings::phrOutput( sPicksetPolygon() )
	: uiStrings::phrInput( sPicksetPolygon() );

    objfld_ = new uiIOObjSel( this, ctxt, label );

    if ( import_ )
    {
	zfld_ = new uiLabeledComboBox( this, tr("Get Z values from") );
	zfld_->box()->addItems( zoptions() );
	zfld_->box()->selectionChanged.notify( mCB(this,uiImpExpPickSet,
				formatSel) );
	zfld_->attach( alignedBelow, filefld_ );

	uiString constzlbl = tr("Constant Z value").withSurvZUnit();
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
				   uiColorInput::Setup(getRandStdDrawColor()).
				   lbltxt(uiStrings::sColor()) );
	colorfld_->attach( alignedBelow, objfld_ );

	polyfld_ = new uiCheckBox( this, tr("Import as Polygon") );
	polyfld_->attach( rightTo, colorfld_ );
    }
    else
    {
	filefld_->attach( alignedBelow, objfld_ );
	coordsysselfld_ = new Coords::uiCoordSystemSel( this );
	coordsysselfld_->attach(alignedBelow, filefld_);
    }
}


uiImpExpPickSet::~uiImpExpPickSet()
{}


void uiImpExpPickSet::inputChgd( CallBacker* )
{
    storedid_.setInvalid();
    File::Path fnmfp( filefld_->fileName() );
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
	mErrRet( uiStrings::phrCannotOpenInpFile() )

    const int zchoice = zfld_->box()->currentItem();
    const char* psnm = objfld_->getInput();
    float constz = zchoice==1 ? constzfld_->getFValue() : 0;
    if ( SI().zIsTime() )
	constz /= 1000;

    RefMan<Pick::Set> ps = new Pick::Set( psnm );
    PickSetAscIO aio( fd_ );
    aio.get( strm, *ps, zchoice==0, constz );
    if ( ps->isEmpty() )
	mErrRet( tr("No valid picks found") )

    const IOObj* ioobj = objfld_->ioobj();
    if ( !ioobj )
	return false;

    if ( zchoice == 2 )
	serv_->fillZValsFrmHor( ps, horinpfld_->box()->currentItem() );

    IOPar ioobjpars( ioobj->pars() );
    const bool ispolygon = polyfld_->isChecked();
    ps->setIsPolygon( ispolygon );
    ps->setDispColor( colorfld_->color() );
    ps->setConnection( ispolygon ? Pick::Set::Disp::Close
				 : Pick::Set::Disp::None );
    SilentTaskRunnerProvider trprov;
    uiString errmsg = Pick::SetMGR().store( *ps, ioobj->key(), trprov );
    if ( !errmsg.isEmpty() )
	mErrRet(errmsg);

    storedid_ = ioobj->key();
    if ( saveButtonChecked() )
	Pick::SetMGR().displayRequest( storedid_ );

    return true;
}


bool uiImpExpPickSet::doExport()
{
    const IOObj* ioobj = objfld_->ioobj();
    if ( !ioobj )
	return false;

    uiRetVal uirv;
    RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( ioobj->key(), uirv );
    if ( !ps )
	mErrRet(uirv)

    const char* fname = filefld_->fileName();
    od_ostream strm( fname );
    if ( !strm.isOK() )
	mErrRet(uiStrings::phrCannotOpenOutpFile())

    strm.stdStream() << std::fixed;
    BufferString buf;
    Pick::SetIter psiter( *ps );
    while ( psiter.next() )
    {
	psiter.get().toString( buf, true, coordsysselfld_->getCoordSystem() );
	strm << buf << od_newline;
    }

    strm << od_endl;
    return true;
}


bool uiImpExpPickSet::acceptOK()
{
    if ( !checkInpFlds() )
	return false;
    const bool ret = import_ ? doImport() : doExport();
    if ( !ret )
	return false;

    uiString msg = tr("Success.\n\nDo you want to %2 more PointSets?")
		 .arg(import_ ? uiStrings::sImport() : uiStrings::sExport());
    return !uiMSG().askGoOn( msg, uiStrings::sYes(), tr("No, close window") );
}


bool uiImpExpPickSet::checkInpFlds()
{
    BufferString filenm = filefld_->fileName();
    if ( import_ && !File::exists(filenm) )
	mErrRet( uiStrings::phrSelect(uiStrings::sInputFile().toLower()) );

    if ( !import_ && filenm.isEmpty() )
	mErrRet( uiStrings::phrSelOutpFile() );

    if ( !objfld_->commitInput() )
	return false;

    if ( import_ )
    {
	if ( !dataselfld_->commit() )
	    mErrRet( uiStrings::phrSpecify(tr("data format")) );

	const int zchoice = zfld_->box()->currentItem();
	if ( zchoice == 1 )
	{
	    float constz = constzfld_->getFValue();
	    if ( SI().zIsTime() ) constz /= 1000;

	    if ( !SI().zRange().includes( constz,false ) )
		mErrRet( uiStrings::phrEnter(tr("a valid Z value")) );
	}
    }

    return true;
}

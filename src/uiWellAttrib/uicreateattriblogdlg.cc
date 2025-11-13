/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicreateattriblogdlg.h"

#include "attribsel.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "survinfo.h"
#include "wellman.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "wellmarker.h"
#include "wellreader.h"
#include "wellwriter.h"

#include "uiattrsel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimnemonicsel.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uiseparator.h"
#include "uitaskrunner.h"


uiCreateAttribLogDlg::uiCreateAttribLogDlg( uiParent* p,
					    const BufferStringSet& wellnames,
					    const Attrib::DescSet* attrib ,
					    const NLAModel* mdl,
					    bool singlewell )
    : uiDialog(p,Setup(tr("Create Attribute Log"),
		       mODHelpKey(mCreateAttribLogDlgHelpID)))
    , wellnames_(wellnames)
    , singlewell_(singlewell)
    , datasetup_(AttribLogCreator::Setup(attrib,0))
{
    setCtrlStyle( RunAndClose );

    uiWellExtractParams::Setup wsu;
    wsu.withzstep_ = true; wsu.withzintime_ = false;
    wsu.defmeterstep_ = SI().depthsInFeet() ? 0.5f*mFromFeetFactorF : 0.15f;
    wsu.withextractintime_ = false;
    zrangeselfld_ = new uiWellExtractParams( this, wsu );

    datasetup_ = AttribLogCreator::Setup( attrib, &zrangeselfld_->params() );
    datasetup_.nlamodel_ = mdl;
    attribfld_ = datasetup_.attrib_ ?
			      new uiAttrSel( this, *datasetup_.attrib_ )
			    : new uiAttrSel( this, uiString::empty(),
						    uiAttrSelData(false) );
    attribfld_->setNLAModel( datasetup_.nlamodel_ );
    mAttachCB( attribfld_->selectionDone, uiCreateAttribLogDlg::selDone );

    auto* sep1 = new uiSeparator( this, "Attrib/Well Sep" );
    sep1->attach( stretchedBelow, attribfld_ );

    if ( !singlewell )
    {
	welllistfld_ = new uiListBox( this, "Wells", OD::ChooseAtLeastOne );
	welllistfld_->attach( ensureBelow, sep1 );
	welllistfld_->attach( alignedBelow, attribfld_ );
	welllistfld_->addItems( wellnames_ );
    }

    if ( singlewell )
    {
	zrangeselfld_->attach( ensureBelow, sep1 );
	zrangeselfld_->attach( alignedBelow, attribfld_ );
    }
    else
	zrangeselfld_->attach( alignedBelow, welllistfld_ );

    auto* sep2 = new uiSeparator( this, "Z Sel/Log Sep" );
    sep2->attach( stretchedBelow, zrangeselfld_ );

    const uiMnemonicsSel::Setup su( nullptr, tr("Output log mnemonic") );
    mnemfld_ = new uiMnemonicsSel( this, su );
    mnemfld_->attach( ensureBelow, sep2 );
    mnemfld_->attach( alignedBelow, zrangeselfld_);

    lognmfld_ = new uiGenInput( this, tr("Log name") );
    lognmfld_->setElemSzPol( uiObject::Wide );
    lognmfld_->attach( alignedBelow, mnemfld_);

    mAttachCB( postFinalize(), uiCreateAttribLogDlg::init  );
}


uiCreateAttribLogDlg::~uiCreateAttribLogDlg()
{
    detachAllNotifiers();
}


void uiCreateAttribLogDlg::init( CallBacker* )
{
    Well::MarkerSet mrkrs;
    Well::LoadReqs lreqs( Well::Mrkrs );
    RefObjectSet<const Well::Data> wds;
    for ( int idx=0; idx<wellnames_.size(); idx++ )
    {
	const IOObj* ioobj = Well::findIOObj( wellnames_.get(idx), nullptr );
	if ( !ioobj )
	    continue;

	ConstRefMan<Well::Data> wd = Well::MGR().get( ioobj->key(), lreqs );
	if ( wd )
	    wds.add( wd.ptr() );
    }

    BufferStringSet allmarkernames;
    Well::Man::getAllMarkerNames( allmarkernames, wds );
    zrangeselfld_->setMarkers( allmarkernames );
    const Mnemonic* defmnem = MNC().getByName( "SEIS" );
    if ( defmnem )
	mnemfld_->setMnemonic( *defmnem );
}


void uiCreateAttribLogDlg::selDone( CallBacker* )
{
    const char* inputstr = attribfld_->getInput();
    lognmfld_->setText( inputstr );
}


bool uiCreateAttribLogDlg::acceptOK( CallBacker* )
{
    if ( !attribfld_ )
	return true;

    const Attrib::DescID seldescid = attribfld_->attribID();
    const int outputnr = attribfld_->outputNr();
    if ( seldescid.asInt() < 0 && (datasetup_.nlamodel_ && outputnr<0) )
    {
	uiMSG().error( tr("No valid attribute selected") );
	return false;
    }

    Attrib::SelSpec selspec;
    datasetup_.selspec_ = &selspec;
    attribfld_->fillSelSpec( *datasetup_.selspec_ );

    BufferStringSet wellnms;
    if ( singlewell_ )
	wellnms.add( wellnames_.first()->buf() );
    else
    {
	if ( welllistfld_->nrChosen() < 1 )
	    return true;

	welllistfld_->getChosen( wellnms );
    }

    TypeSet<MultiID> wellids;
    uiRetVal uirv;
    for ( const auto* wellnm : wellnms )
    {
	PtrMan<IOObj> ioobj = Well::findIOObj( wellnm->buf(), nullptr );
	if ( !ioobj )
	{
	    uirv.add( tr("%1 '%2'")
		.arg( uiStrings::phrCannotFindDBEntry(uiStrings::sWell()))
		.arg( wellnm->buf() ) );
	    continue;
	}

	wellids += ioobj->key();
    }

    if ( wellids.isEmpty() )
    {
	uiMSG().error( uirv );
	return false;
    }
    else if ( uirv.isError() )
	uiMSG().warning( uirv );

    uiString errmsg;
    if ( !datasetup_.extractparams_->isOK(&errmsg) )
    {
	uiMSG().error( errmsg );
	return false;
    }

    datasetup_.lognm_ = lognmfld_->text();
    if ( datasetup_.lognm_.isEmpty() )
    {
	uiMSG().error( tr("Please provide an output log name") );
	return false;
    }

    const Well::LoadReqs lreq( Well::LogInfos );
    RefObjectSet<Well::Data> wds;
    MultiWellReader mwrdr( wellids, wds, lreq );
    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(mwrdr) )
    {
	uiMSG().error( mwrdr.uiMessage() );
	return false;
    }

    const char* lognm = datasetup_.lognm_.str();
    bool dooverwrite = false;
    uiString logpresentques = tr( "Following wells already have a '%1' log."
				  "Do you want to overwrite them?" )
			      .arg( lognm );
    uiStringSet ovwrwellnms;
    for ( int idx=0; idx<wds.size(); idx++ )
    {
	ConstRefMan<Well::Data> wd = wds.get( idx );
	if ( wd && wd->logs().isPresent(lognm) )
	    ovwrwellnms.add( tr(wd->name().buf()) );
    }

    if ( !ovwrwellnms.isEmpty() )
	dooverwrite = uiMSG().askGoOnWithDetails( logpresentques, ovwrwellnms,
					      tr("Overwrite"), tr("Ignore") );

    const Mnemonic* outmn = mnemfld_->mnemonic();
    if ( !outmn )
	outmn = &Mnemonic::undef();

    BulkAttribLogCreator balc( datasetup_, wellids, *outmn, dooverwrite );
    if ( !taskrunner.execute(balc) )
    {
	uiMSG().errorWithDetails( balc.details(), balc.uiMessage() );
	return false;
    }

    uiStringSet uiwellnms;
    wellnms.fill( uiwellnms );
    return !uiMSG().askGoOnWithDetails(
		    tr("Successfully created attribute log '%1'"
		       ".\nDo you want to create more?").arg(lognm), uiwellnms);
}

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
#include "welllog.h"
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

    uiMnemonicsSel::Setup su( nullptr, tr("Output log mnemonic") );
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

    BufferStringSet selwells;
    if ( !singlewell_ )
    {
	if ( welllistfld_->nrChosen() < 1 )
	    return true;

	welllistfld_->getChosen( selwells );
    }
    else
	selwells.add( wellnames_.get(0) );

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

    const char* lognm = datasetup_.lognm_.str();
    const Well::LoadReqs lreq( Well::Trck, Well::D2T, Well::LogInfos );
    RefObjectSet<Well::Data> wds;
    TypeSet<MultiID> keys;
    uiRetVal uirv;
    for ( int idx=0; idx<selwells.size(); idx++ )
    {
	const char* wellnm = selwells.get(idx).buf();
	PtrMan<IOObj> ioobj = Well::findIOObj( wellnm, nullptr );
	if ( !ioobj )
	{
	    uirv.add( tr("%1 '%2': %3")
		.arg( uiStrings::phrCannotFindDBEntry(uiStrings::sWell()))
		.arg( wellnm ).arg( Well::MGR().errMsg() ) );
	    continue;
	}

	keys += ioobj->key();
    }

    MultiWellReader mwr( keys, wds, lreq );
    uiTaskRunner mwrtr( this );
    if ( !mwrtr.execute(mwr) )
    {
	uiMSG().error( tr("Could not load any wells") );
	return false;
    }

    bool dooverwrite = false;
    uiString logpresentques = tr( "Following wells already have a '%1' log."
				  "Do you want to overwrite them?" )
			      .arg( lognm );
    uiStringSet wellnms;
    for ( int idx=0; idx<wds.size(); idx++ )
    {
	ConstRefMan<Well::Data> wd = wds.get( idx );
	if ( wd && wd->logs().isPresent(lognm) )
	    wellnms.add( tr(wd->name().buf()) );
    }

    if ( !wellnms.isEmpty() )
	dooverwrite = uiMSG().askGoOnWithDetails( logpresentques, wellnms,
						  tr("Overwrite"),
						  tr("Ignore") );

    const Mnemonic* outmn = mnemfld_->mnemonic();
    if ( !outmn )
	outmn = &Mnemonic::undef();

    uiTaskRunner uitr( this );
    BulkAttribLogCreator balc( datasetup_, wds, *outmn, uirv, dooverwrite );
    if ( !uitr.execute(balc) )
    {
	uiMSG().error( balc.uiMessage() );
	return false;
    }

    if ( uirv.isOK() )
	return true;

    if ( uirv.messages().size() == selwells.size() )
    {
	uiMSG().errorWithDetails( uirv, tr("Could not process a single well") );
	return false;
    }
    else
    {
	uiMSG().errorWithDetails( uirv, tr("Could not process all wells") );
	return false;
    }

    return true;
}

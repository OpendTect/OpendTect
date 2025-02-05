/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uicreateattriblogdlg.h"

#include "attribsel.h"
#include "ioman.h"
#include "ioobj.h"
#include "strmprov.h"
#include "survinfo.h"
#include "wellman.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "welllog.h"
#include "wellmarker.h"
#include "wellwriter.h"

#include "uiattrsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimultiwelllogsel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


uiCreateAttribLogDlg::uiCreateAttribLogDlg( uiParent* p,
					    const BufferStringSet& wellnames,
					    const Attrib::DescSet* attrib ,
					    const NLAModel* mdl,
					    bool singlewell )
    : uiDialog(p,uiDialog::Setup(tr("Create Attribute Log"),mNoDlgTitle,
				 mODHelpKey(mCreateAttribLogDlgHelpID)))
    , wellnames_(wellnames)
    , singlewell_(singlewell)
    , sellogidx_(-1)
    , attribfld_(nullptr)
    , datasetup_(AttribLogCreator::Setup( attrib, 0 ))
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
			    : new uiAttrSel( this, 0, uiAttrSelData(false) );
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

    lognmfld_ = new uiGenInput( this, tr("Log name") );
    lognmfld_->setElemSzPol( uiObject::Wide );
    lognmfld_->attach( ensureBelow, sep2 );
    lognmfld_->attach( alignedBelow, zrangeselfld_);

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

    uiTaskRunner taskrunner( this );
    datasetup_.tr_ = &taskrunner;

    const Well::LoadReqs lreq( Well::Trck, Well::D2T, Well::LogInfos );
    bool dontaskagain = false;
    uiRetVal uirv;
    for ( int idx=0; idx<selwells.size(); idx++ )
    {
	const char* wellnm = selwells.get(idx).buf();
	const char* lognm = datasetup_.lognm_.str();
	PtrMan<IOObj> ioobj = Well::findIOObj( selwells.get(idx), nullptr );
	if ( !ioobj )
	{
	    uirv.add( tr("%1 '%2': %3")
		    .arg( uiStrings::phrCannotFindDBEntry(uiStrings::sWell()))
		    .arg( wellnm ).arg( Well::MGR().errMsg() ) );
	    continue;
	}

	RefMan<Well::Data> wd = Well::MGR().get( ioobj->key(), lreq );
	if ( !wd )
	{
	    uirv.add( tr("%1 '%2': %3")
		    .arg( uiStrings::phrCannotRead(uiStrings::sWell()) )
		    .arg( wellnm ).arg( Well::MGR().errMsg() ) );
	    continue;
	}

	if ( SI().zIsTime() && !wd->d2TModel() )
	{
	    uirv.add( tr("No depth to time model defined for well '%1'")
			.arg( wellnm ) );
	    continue;
	}

	if ( wd->logs().isPresent(lognm) && !dontaskagain )
	{
	    const uiString msg = tr("Log: '%1' is already present.\n"
				    "Do you wish to overwrite this log?" )
					.arg( lognm );
	    if ( !uiMSG().askGoOn(msg,true,&dontaskagain) )
		continue;
	}

	int sellogidx = wd->logs().indexOf( lognm ); //not used
	AttribLogCreator attriblog( datasetup_, sellogidx );
	if ( !attriblog.doWork(*wd,errmsg) )
	{
	    uirv.add( errmsg );
	    continue;
	}

	sellogidx = wd->logs().indexOf( lognm );
	PtrMan<Well::Log> newlog = wd->logs().remove( sellogidx );
	if ( !newlog )
	{
	    pErrMsg("Should not happen");
	    continue;
	}

	const MultiID dbkey = ioobj->key();
	if ( !Well::MGR().writeAndRegister(dbkey,newlog) )
	{
	    uirv.add( toUiString(Well::MGR().errMsg()) );
	    continue;
	}
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


bool uiCreateAttribLogDlg::inputsOK( Well::Data& )
{
    return false;
}

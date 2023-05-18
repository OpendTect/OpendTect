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
#include "wellwriter.h"

#include "uiattrsel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uimultiwelllogsel.h"
#include "uiseparator.h"
#include "uitaskrunner.h"


uiCreateAttribLogDlg::uiCreateAttribLogDlg( uiParent* p,
					    const BufferStringSet& wellnames,
					    const Attrib::DescSet* attrib ,
					    const NLAModel* mdl,
					    bool singlewell )
    : uiDialog(p,uiDialog::Setup(tr("Create Attribute Log"),mNoDlgTitle,
				 mODHelpKey(mCreateAttribLogDlgHelpID)))
    , attribfld_(0)
    , wellnames_(wellnames)
    , sellogidx_(-1)
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
			    : new uiAttrSel( this, 0, uiAttrSelData(false) );
    attribfld_->setNLAModel( datasetup_.nlamodel_ );
    mAttachCB( attribfld_->selectionDone, uiCreateAttribLogDlg::selDone );

    uiSeparator* sep1 = new uiSeparator( this, "Attrib/Well Sep" );
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

    uiSeparator* sep2 = new uiSeparator( this, "Z Sel/Log Sep" );
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


#define mErrRet(msg) { uiMSG().error(msg); return false; }
bool uiCreateAttribLogDlg::acceptOK( CallBacker* )
{
    if ( !attribfld_ ) return true;

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
    if ( !datasetup_.extractparams_->isOK( &errmsg ) )
	mErrRet( errmsg );

    datasetup_.lognm_ = lognmfld_->text();
    Attrib::SelSpec selspec;
    datasetup_.selspec_ = &selspec;
    attribfld_->fillSelSpec( *datasetup_.selspec_ );

    Well::LoadReqs lreq( Well::Trck, Well::D2T, Well::LogInfos );
    for ( int idx=0; idx<selwells.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = Well::findIOObj( selwells.get(idx), nullptr );
	if ( !ioobj )
	    mErrRet(tr("Cannot find well in object manager"))

	RefMan<Well::Data> wd = Well::MGR().get( ioobj->key(), lreq );
	if ( !wd )
	    continue;

	if ( !inputsOK(*wd) )
	    return false;

	PtrMan<uiTaskRunner> taskrunner = new uiTaskRunner( this );
	datasetup_.tr_ = taskrunner;
	AttribLogCreator attriblog( datasetup_, sellogidx_ );
	if ( !attriblog.doWork( *wd, errmsg ) )
	    mErrRet( errmsg )

	Well::Writer wtr( *ioobj, *wd );

	const Well::Log& newlog = wd->logs().getLog(sellogidx_);
	if ( !wtr.putLog(newlog) )
	{
	    errmsg = tr("Cannot write log '%1'.\nCheck the permissions "
			"of the *.wll file").arg(newlog.name());
	    uiMSG().error( errmsg );
	    return false;
	}
	wd->logschanged.trigger( -1 );
    }
    return true;
}


bool uiCreateAttribLogDlg::inputsOK( Well::Data& wdin )
{
    RefMan<Well::Data> wd( &wdin );
    if ( SI().zIsTime() && !wd->d2TModel() )
	mErrRet( tr("No depth to time model defined") );

    const Attrib::DescID seldescid = attribfld_->attribID();
    const int outputnr = attribfld_->outputNr();
    if ( seldescid.asInt() < 0 && (datasetup_.nlamodel_ && outputnr<0) )
	mErrRet( tr("No valid attribute selected") );

    datasetup_.lognm_ = lognmfld_->text();
    if ( datasetup_.lognm_.isEmpty() )
	mErrRet( tr("Please provide logname") );

    sellogidx_ = wd->logs().indexOf( datasetup_.lognm_ );
    if ( sellogidx_ >= 0 )
    {
	uiString msg = tr("Log: '%1' is already present.\nDo you wish to "
			  "overwrite this log?").arg(datasetup_.lognm_);
	if ( !uiMSG().askOverwrite(msg) ) return false;
    }

    return true;
}

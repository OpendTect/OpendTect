/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          March 2008
_______________________________________________________________________

-*/

#include "uicreateattriblogdlg.h"

#include "attribsel.h"
#include "dbman.h"
#include "ioobj.h"
#include "strmprov.h"
#include "survinfo.h"
#include "wellmanager.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "wellmarker.h"

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
    , attribfld_(0)
    , datasetup_(AttribLogCreator::Setup( attrib, 0 ))
{
    uiWellExtractParams::Setup wsu;
    wsu.withzstep_ = true; wsu.withzintime_ = false;
    wsu.defmeterstep_ = 0.15;
    wsu.withextractintime_ = false;
    zrangeselfld_ = new uiWellExtractParams( this, wsu );

    datasetup_ = AttribLogCreator::Setup( attrib, &zrangeselfld_->params() );
    datasetup_.nlamodel_ = mdl;
    attribfld_ = datasetup_.attrib_ ?
			      new uiAttrSel( this, *datasetup_.attrib_ )
			    : new uiAttrSel( this, 0, uiAttrSelData(false) );
    attribfld_->setNLAModel( datasetup_.nlamodel_ );
    attribfld_->selectionDone.notify( mCB(this,uiCreateAttribLogDlg,selDone) );

    uiSeparator* sep1 = new uiSeparator( this, "Attrib/Well Sep" );
    sep1->attach( stretchedBelow, attribfld_ );

    if ( !singlewell )
    {
	welllistfld_ = new uiListBox( this, "Wells", OD::ChooseAtLeastOne );
	welllistfld_->attach( ensureBelow, sep1 );
	welllistfld_->attach( alignedBelow, attribfld_ );
	welllistfld_->addItems( wellnames );
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
    lognmfld_->attach( ensureBelow, sep2 );
    lognmfld_->attach( alignedBelow, zrangeselfld_);

    postFinalise().notify( mCB(this, uiCreateAttribLogDlg, init ) );
}


void uiCreateAttribLogDlg::init( CallBacker* )
{
    Well::MarkerSet mrkrs;
    for ( int idx=0; idx<wellnames_.size(); idx++ )
    {
	const DBKey dbky = Well::MGR().getIDByName( wellnames_.get(idx) );
	if ( dbky.isInvalid() )
	    continue;
	ConstRefMan<Well::Data> wd = Well::MGR().fetch( dbky );
	if ( wd )
	    mrkrs.append( wd->markers() );
    }

    //sort( mrkrs ); do later
    zrangeselfld_->setMarkers( mrkrs );
}


void uiCreateAttribLogDlg::selDone( CallBacker* )
{
    const char* inputstr = attribfld_->getInput();
    lognmfld_->setText( inputstr );
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }
bool uiCreateAttribLogDlg::acceptOK()
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

    for ( int idx=0; idx<selwells.size(); idx++ )
    {
	const DBKey dbky = Well::MGR().getIDByName( selwells.get(idx) );
	if ( dbky.isInvalid() )
	    continue;
	RefMan<Well::Data> wd = Well::MGR().fetchForEdit( dbky );
	if ( !wd )
	    continue;
	if ( !inputsOK(*wd) )
	    return false;

	PtrMan<uiTaskRunner> taskrunner = new uiTaskRunner( this );
	datasetup_.tr_ = taskrunner;
	AttribLogCreator attriblog( datasetup_, sellogidx_ );
	if ( !attriblog.doWork( *wd, errmsg ) )
	    mErrRet( errmsg )

	uiRetVal uirv = Well::MGR().save( dbky );
	if ( uirv.isError() )
	    { uiMSG().error( uirv ); return false; }
    }

    return true;
}


bool uiCreateAttribLogDlg::inputsOK( const Well::Data& wd )
{
    if ( SI().zIsTime() && !wd.d2TModelPtr() )
	mErrRet( tr("No depth to time model defined") );

    const Attrib::DescID seldescid = attribfld_->attribID();
    const int outputnr = attribfld_->outputNr();
    if ( seldescid.asInt() < 0 && (datasetup_.nlamodel_ && outputnr<0) )
	mErrRet( tr("No valid attribute selected") );

    datasetup_.lognm_ = lognmfld_->text();
    if ( datasetup_.lognm_.isEmpty() )
	mErrRet( tr("Please provide logname") );

    sellogidx_ = wd.logs().indexOf( datasetup_.lognm_ );
    if ( sellogidx_ >= 0 )
    {
	uiString msg = tr("Log: '%1' is already present.\nDo you wish to "
			  "overwrite this log?").arg(datasetup_.lognm_);
	if ( !uiMSG().askOverwrite(msg) ) return false;
    }

    return true;
}

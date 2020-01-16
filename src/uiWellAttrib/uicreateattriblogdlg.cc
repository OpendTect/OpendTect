/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          March 2008
_______________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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


static int getWellIndex( const char* nm )
{
    const FixedString wellnm = nm;
    for ( int idx=0; idx<Well::MGR().wells().size(); idx++ )
    {
	if ( wellnm == Well::MGR().wells()[idx]->name() )
	    return idx;
    }
    return -1;
}


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
    wsu.defmeterstep_ = SI().depthsInFeet() ? 0.5f*mFromFeetFactorF : 0.15f;
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
    lognmfld_->setElemSzPol( uiObject::Wide );
    lognmfld_->attach( ensureBelow, sep2 );
    lognmfld_->attach( alignedBelow, zrangeselfld_);

    postFinalise().notify( mCB(this, uiCreateAttribLogDlg, init ) );
}


void uiCreateAttribLogDlg::init( CallBacker* )
{
    Well::MarkerSet mrkrs;
    for ( int idx=0; idx<wellnames_.size(); idx++ )
    {
	const int wdidx = getWellIndex( wellnames_.get(idx) );
	if ( !Well::MGR().wells().validIdx(wdidx) )
	    continue;

	Well::Data* wdtmp = Well::MGR().wells()[wdidx];
	if ( wdtmp )
	    mrkrs.append( wdtmp->markers() );
    }

    sort( mrkrs ); zrangeselfld_->setMarkers( mrkrs );
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

    for ( int idx=0; idx<selwells.size(); idx++ )
    {
	const int wellidx = getWellIndex( selwells.get(idx) );
	if ( wellidx<0 ) continue;

	if ( !inputsOK(wellidx) )
	    return false;

	PtrMan<uiTaskRunner> taskrunner = new uiTaskRunner( this );
	datasetup_.tr_ = taskrunner;
	AttribLogCreator attriblog( datasetup_, sellogidx_ );
	Well::Data* wd = Well::MGR().wells()[ wellidx ];
	if ( !wd )
	    continue;
	if ( !attriblog.doWork( *wd, errmsg ) )
	    mErrRet( errmsg )

	PtrMan<IOObj> ioobj = IOM().get( wd->multiID() );
	if ( !ioobj ) mErrRet(tr("Cannot find well in object manager"))

	Well::Writer wtr( *ioobj, *wd );

	const Well::Log& newlog = wd->logs().getLog(sellogidx_);
	if ( !wtr.putLog(newlog) )
	{
	    errmsg = tr("Cannot write log '%1'.\nCheck the permissions "
			"of the *.wll file").arg(newlog.name());
	    uiMSG().error( errmsg );
	    return false;
	}
    }

    return true;
}


bool uiCreateAttribLogDlg::inputsOK( int wellno )
{
    Well::Data* wd = Well::MGR().wells()[ wellno ];
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

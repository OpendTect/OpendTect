/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          March 2008
_______________________________________________________________________

-*/
static const char* rcsID = "$Id: uicreateattriblogdlg.cc,v 1.34 2012/05/11 14:17:06 cvsbruno Exp $";

#include "uicreateattriblogdlg.h"

#include "attribsel.h"
#include "survinfo.h"
#include "wellman.h"
#include "welldata.h"
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


static int getWellIndex( const char* wellnm )
{
    for ( int idx=0; idx<Well::MGR().wells().size(); idx++ )
    {
	if ( !strcmp(Well::MGR().wells()[idx]->name(),wellnm) )
	    return idx;
    }
    return -1;
}


uiCreateAttribLogDlg::uiCreateAttribLogDlg( uiParent* p,
					    const BufferStringSet& wellnames,
					    const Attrib::DescSet* attrib , 
					    const NLAModel* mdl,
					    bool singlewell )
    : uiDialog(p,uiDialog::Setup("Create Attribute Log",
				 "Specify parameters for the new attribute log",
				 "107.3.0") )
    , wellnames_(wellnames)
    , singlewell_(singlewell)
    , sellogidx_(-1)
    , attribfld_(0)
    , datasetup_(AttribLogCreator::Setup( attrib, 0 ))
{
    zrangeselfld_ = new uiWellExtractParams(this,
	    uiWellExtractParams::Setup().withzstep(true).withzintime(false));

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
	welllistfld_ = new uiListBox( this );
	welllistfld_->attach( ensureBelow, sep1 );
	welllistfld_->attach( alignedBelow, attribfld_ );
	welllistfld_->setMultiSelect();
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

    lognmfld_ = new uiGenInput( this, "Log name" );
    lognmfld_->attach( ensureBelow, sep2 );
    lognmfld_->attach( alignedBelow, zrangeselfld_);

    postFinalise().notify( mCB(this,uiCreateAttribLogDlg,init) );
}


void uiCreateAttribLogDlg::init( CallBacker* )
{
    for ( int idx=0; idx<wellnames_.size(); idx++ )
    {
	int wdidx = getWellIndex( wellnames_.get(idx) );
	Well::Data* wdtmp = Well::MGR().wells()[wdidx];
	if ( wdtmp )
	    zrangeselfld_->addMarkers( wdtmp->markers() );
    }
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
	if ( welllistfld_->nrSelected() < 1 )
	    mErrRet( "Select at least one well" );
	welllistfld_->getSelectedItems( selwells );
    }
    else
	selwells.add( wellnames_.get(0) );


    BufferString errmsg;
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

	uiTaskRunner* tr = new uiTaskRunner( this );
	datasetup_.tr_ = tr;
	AttribLogCreator attriblog( datasetup_, sellogidx_ );
	Well::Data* wd = Well::MGR().wells()[ wellidx ];
	if ( !wd ) 
	    continue;
	if ( !attriblog.doWork( *wd, errmsg ) )
	    { delete tr; mErrRet( errmsg ) }
	delete tr;
    }
    return true;
}


bool uiCreateAttribLogDlg::inputsOK( int wellno )
{
    Well::Data* wd = Well::MGR().wells()[ wellno ];
    if ( SI().zIsTime() && !wd->d2TModel() )
	mErrRet( "No depth to time model defined" );

    const Attrib::DescID seldescid = attribfld_->attribID();
    const int outputnr = attribfld_->outputNr();
    if ( seldescid.asInt() < 0 && (datasetup_.nlamodel_ && outputnr<0) )
	mErrRet( "No valid attribute selected" );

    datasetup_.lognm_ = lognmfld_->text();
    if ( datasetup_.lognm_.isEmpty() )
	mErrRet( "Please provide logname" );

    sellogidx_ = wd->logs().indexOf( datasetup_.lognm_ );
    if ( sellogidx_ >= 0 )
    {
	BufferString msg( "Log: '" ); msg += datasetup_.lognm_;
	msg += "' is already present.\nDo you wish to overwrite this log?";
	if ( !uiMSG().askOverwrite(msg) ) return false;
    }
    return true;
}

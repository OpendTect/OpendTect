/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki
 Date:          March 2008
_______________________________________________________________________

-*/
static const char* rcsID = "$Id: uicreateattriblogdlg.cc,v 1.28 2011-01-20 10:23:22 cvsbruno Exp $";

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
#include "uimsg.h"
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
    , datasetup_(AttribLogCreator::Setup(attrib))
    , wellnames_(wellnames)
    , singlewell_(singlewell)
    , sellogidx_(-1)
    , attribfld_(0)
{
    datasetup_.nlamodel_ = mdl;
    int nrmarkers = -1; int wellidx = -1;
    for ( int idx=0; idx<wellnames_.size(); idx++ )
    {
	int wdidx = getWellIndex( wellnames_.get(idx) );
	Well::Data* wdtmp = Well::MGR().wells()[wdidx];
	if ( wdtmp->markers().size() > nrmarkers )
	    { nrmarkers = wdtmp->markers().size(); wellidx = wdidx; }
    }

    Well::Data* wd = wellidx<0 ? 0 : Well::MGR().wells()[wellidx];
    if ( !wd )
	{ uiMSG().error( "First well not valid" ); return; }

    attribfld_ = datasetup_.attrib_ ? 
			      new uiAttrSel( this, *datasetup_.attrib_ )
			    : new uiAttrSel( this, 0, uiAttrSelData(false) );
    attribfld_->setNLAModel( datasetup_.nlamodel_ );
    attribfld_->selectionDone.notify( mCB(this,uiCreateAttribLogDlg,selDone) );

    if ( !singlewell )
    {
	welllistfld_ = new uiListBox( this );
	welllistfld_->attach( alignedBelow, attribfld_ );
	welllistfld_->setMultiSelect();
	welllistfld_->addItems( wellnames );
    }

    BufferStringSet markernames;
    markernames.add( Well::TrackSampler::sKeyDataStart() );
    if ( wd->haveMarkers() )
    {
	for ( int idx=0; idx<wd->markers().size(); idx++ )
	    markernames.add( wd->markers()[idx]->name() );
    }
    markernames.add( Well::TrackSampler::sKeyDataEnd() );

    StringListInpSpec slis( markernames );
    topmrkfld_ = new uiGenInput( this, "Extract between",
	    					slis.setName("Top Marker") );

    if ( singlewell )
	topmrkfld_->attach( alignedBelow, attribfld_ );
    else
	topmrkfld_->attach( alignedBelow, welllistfld_ );
    topmrkfld_->setValue( (int)0 );
    topmrkfld_->setElemSzPol( uiObject::Medium );
    botmrkfld_ = new uiGenInput( this, "", slis.setName("Bottom Marker") );
    botmrkfld_->attach( rightOf, topmrkfld_ );
    botmrkfld_->setValue( markernames.size()-1 );
    botmrkfld_->setElemSzPol( uiObject::Medium );

    const bool zinft = SI().depthsInFeetByDefault();
    const float defstep = zinft ? 0.5 : 0.15;
    BufferString lbl = "Step "; lbl += zinft ? "(ft)" : "(m)";
    stepfld_ = new uiGenInput( this, lbl, FloatInpSpec(defstep) );
    stepfld_->attach( rightOf, botmrkfld_ );

    lognmfld_ = new uiGenInput( this, "Log name" );
    lognmfld_->attach( alignedBelow, topmrkfld_ );
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

    if ( !strcmp(topmrkfld_->text(),botmrkfld_->text()) )
	mErrRet( "Please select different markers" )

    datasetup_.topmrknm_ = topmrkfld_->text();
    datasetup_.botmrknm_ = botmrkfld_->text();
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
	BufferString errmsg;
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

    datasetup_.extractstep_ = stepfld_->getfValue();
    if( datasetup_.extractstep_<0 || datasetup_.extractstep_>100 )
	mErrRet( "Please Enter a valid step value" );
    
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

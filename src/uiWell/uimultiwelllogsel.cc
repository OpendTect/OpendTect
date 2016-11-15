/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:		Bruno
Date:		Jan 2011
________________________________________________________________________

-*/


#include "uimultiwelllogsel.h"

#include "dbman.h"
#include "ioobj.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "welldata.h"
#include "wellextractdata.h"
#include "welllogset.h"
#include "wellmanager.h"
#include "wellmarker.h"
#include "wellwriter.h"

#include "uicombobox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uilistboxchoiceio.h"
#include "uistrings.h"
#include "uitaskrunner.h"
#include "uiwellmarkersel.h"
#include "uiwellextractparams.h"


uiMultiWellLogSel::uiMultiWellLogSel( uiParent* p, const bool singlelog, 
				      const uiWellExtractParams::Setup* s,
				      const BufferStringSet* wellnms,
				      const BufferStringSet* lognms )
    : uiGroup( p, "Select Multi-Well Logs" )
    , singlewid_(0)
    , singlelog_(singlelog)
    , setup_(s)
    , wellextractparamsfld_(0)
{
    bool isextractparamsreq = s ? true : false;
    init(isextractparamsreq);
    update();
    if ( wellsfld_ && wellnms && wellnms->size() )
	wellsfld_->setChosen( *wellnms );

    if ( lognms && lognms->size() )
	logsfld_->setChosen( *lognms );
}


uiMultiWellLogSel::uiMultiWellLogSel( uiParent* p, const bool singlelog, 
				      const DBKey& singlewid,
				      const uiWellExtractParams::Setup* s )
    : uiGroup( p, "Select Multi-Well Logs" )
    , singlewid_(&singlewid)
    , singlelog_(singlelog)
    , setup_(s)
    , wellextractparamsfld_(0)
{
    bool isextractparamsreq = s ? true : false;
    init(isextractparamsreq);
    update();
}

void uiMultiWellLogSel::init(const bool isextractparamsreq )
{
    const uiObject::SzPolicy hpol = uiObject::MedMax;
    const uiObject::SzPolicy vpol = uiObject::WideMax;
    const OD::ChoiceMode chmode =
	singlelog_ ? OD::ChooseOnlyOne : OD::ChooseAtLeastOne;
    uiListBox::Setup su( chmode, singlelog_ ? uiStrings::sLog() :
							    uiStrings::sLogs(),
			 singlewid_ ? uiListBox::LeftTop : uiListBox::AboveMid);
    logsfld_ = new uiListBox( this, su );
    logsfld_->setHSzPol( hpol );
    logsfld_->setVSzPol( vpol );

    wellsfld_ = 0; wellschoiceio_ = 0;

    if ( isextractparamsreq )
    {
      wellextractparamsfld_ = new  uiWellExtractParams( this, *setup_ );
      wellextractparamsfld_->attach( ensureBelow, logsfld_ );
    } 

    if ( !singlewid_ )
    {
	uiListBox::Setup suw( OD::ChooseAtLeastOne, uiStrings::sWells(),
			      uiListBox::AboveMid );
	wellsfld_ = new uiListBox( this, suw );
	wellsfld_->setHSzPol( hpol );
	wellsfld_->setVSzPol( vpol );
	mAttachCB( wellsfld_->selectionChanged,
		   uiMultiWellLogSel::updateLogsFldCB);
	mAttachCB( wellsfld_->itemChosen, uiMultiWellLogSel::updateLogsFldCB);
	logsfld_->attach( rightTo, wellsfld_ );

	wellschoiceio_ = new uiListBoxChoiceIO( *wellsfld_, "Well" );
	mAttachCB( wellschoiceio_->readDone,
		   uiMultiWellLogSel::readWellChoiceDone );
	mAttachCB( wellschoiceio_->storeRequested,
		   uiMultiWellLogSel::writeWellChoiceReq );
    }

    if ( isextractparamsreq )
	wellextractparamsfld_->attach( ensureBelow, wellsfld_ ?
							wellsfld_ : logsfld_ );

}


void uiMultiWellLogSel::selectOnlyWritableWells()
{
    NotifyStopper ns( wellsfld_->selectionChanged );
    for ( int idx=0; idx<wellobjs_.size(); idx++ )
    {
	IOObj* ioobj = wellobjs_[idx];
	if ( !Well::Writer::isFunctional(*ioobj) )
	{
	    wellsfld_->removeItem( idx );
	    wellobjs_.removeSingle(idx);
	    delete ioobj;
	    idx--;
	}
    }

    updateLogsFldCB( 0 );
}


uiMultiWellLogSel::~uiMultiWellLogSel()
{
    detachAllNotifiers();
    deepErase( wellobjs_ );
}


void uiMultiWellLogSel::update()
{
    clear();
    if ( wellsfld_ )
    {
	NotifyStopper ns( wellsfld_->selectionChanged );
	wellsfld_->setEmpty();
    }

    logsfld_->setEmpty();

    deepErase( wellobjs_ );

    Well::InfoCollector wic( false, false, false );
    if ( !wic.execute() ) return;

    for ( int iid=0; iid<wic.ids().size(); iid++ )
    {
	const DBKey dbky = wic.ids()[iid];
	IOObj* ioobj = DBM().get( dbky );
	if ( !ioobj || ( singlewid_ && dbky != *singlewid_ ) )
	    continue;

	wellobjs_ += ioobj;

	if ( wellsfld_ )
	    wellsfld_->addItem( ioobj->uiName() );
    }

    updateLogsFldCB( 0 );
}


void uiMultiWellLogSel::updateLogsFldCB( CallBacker* )
{
    logsfld_->setEmpty();
    DBKeySet mids;
    getSelWellIDs( mids );
    if ( mids.isEmpty() )
	return;

    BufferStringSet availablelognms;
    BufferStringSet availablemrkrs;
    for ( int midx=0; midx<mids.size(); midx++ )
    {
	const DBKey dbky = mids[midx];
	ConstRefMan<Well::Data> wd = Well::MGR().fetch( dbky );
	if ( !wd )
	    continue;

	BufferStringSet lognms;
	wd->logs().getNames( lognms );

	BufferStringSet mrkrnms;
	wd->markers().getNames( mrkrnms );
	if ( midx == 0 )
	{
	    availablelognms = lognms;
	    availablemrkrs = mrkrnms;
	}
	else
	{
	    for ( int lidx=availablelognms.size()-1; lidx>=0; lidx-- )
	    {
		if (!lognms.isPresent(availablelognms.get(lidx)) )
		    availablelognms.removeSingle( lidx );
	    }

	    for ( int mrkidx=availablemrkrs.size()-1; mrkidx>=0; mrkidx-- )
	    {
		if (!mrkrnms.isPresent(availablemrkrs.get(mrkidx)) )
		    availablemrkrs.removeSingle( mrkidx );
	    }
	}
    }

    logsfld_->addItems( availablelognms );
    if ( wellextractparamsfld_ )
	wellextractparamsfld_->setMarkers( availablemrkrs );
}


void uiMultiWellLogSel::getSelWellIDs( DBKeySet& mids ) const
{
    if ( singlewid_ && !wellobjs_.isEmpty() )
    {
	mids.add( wellobjs_[0]->key() );
    }
    else if ( wellsfld_ )
    {
	for ( int idx=0; idx<wellsfld_->size(); idx++ )
	{
	    if ( wellsfld_->isChosen(idx) )
		mids.add( wellobjs_[idx]->key() );
	}
    }
}


void uiMultiWellLogSel::getSelWellIDs( BufferStringSet& wids ) const
{
    DBKeySet mids;
    getSelWellIDs( mids );
    for ( int idx=0; idx<mids.size(); idx++ )
	wids.add( mids[idx].toString() );
}


void uiMultiWellLogSel::setSelWellIDs( const BufferStringSet& idstrs )
{
    DBKeySet mids;
    for ( int idx=0; idx<idstrs.size(); idx++ )
	mids += DBKey::getFromString( idstrs.get(idx) );

    setSelWellIDs( mids );
}


void uiMultiWellLogSel::setSelWellIDs( const DBKeySet& ids )
{
    BufferStringSet wellnms;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = DBM().get( ids[idx] );
	if ( ioobj ) wellnms.add( ioobj->name() );
    }

    setSelWellNames( wellnms );
}


void uiMultiWellLogSel::getSelWellNames( BufferStringSet& wellnms ) const
{
    if ( singlewid_ && !wellobjs_.isEmpty() )
	wellnms.add( wellobjs_[0]->name() );
    else
	wellsfld_->getChosen( wellnms );
}


void uiMultiWellLogSel::setSelWellNames( const BufferStringSet& nms )
{ if ( wellsfld_ ) wellsfld_->setChosen( nms ); }

void uiMultiWellLogSel::getSelLogNames( BufferStringSet& nms ) const
{ logsfld_->getChosen( nms ); }

void uiMultiWellLogSel::setSelLogNames( const BufferStringSet& nms )
{ logsfld_->setChosen( nms ); }


void uiMultiWellLogSel::readWellChoiceDone( CallBacker* )
{
    if ( !wellschoiceio_ ) return;

    DBKeySet mids;
    for ( int idx=0; idx<wellschoiceio_->chosenKeys().size(); idx++ )
	mids += DBKey::getFromString( wellschoiceio_->chosenKeys().get(idx) );
    setSelWellIDs( mids );
}


void uiMultiWellLogSel::writeWellChoiceReq( CallBacker* )
{
    if ( !wellschoiceio_ ) return;

    wellschoiceio_->keys().setEmpty();
    for ( int idx=0; idx<wellobjs_.size(); idx++ )
	wellschoiceio_->keys().add( wellobjs_[idx]->key().toString() );
}


void uiMultiWellLogSel::setWellExtractParams( const Well::ExtractParams& sel )
{
    if ( wellextractparamsfld_ )
	wellextractparamsfld_->setRangeSel( sel ); 
    
    return;
}


Well::ExtractParams* uiMultiWellLogSel::getWellExtractParams()
{ 
    return &wellextractparamsfld_->params(); 
}


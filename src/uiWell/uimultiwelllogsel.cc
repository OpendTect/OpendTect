/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:		Bruno
Date:		Jan 2011
________________________________________________________________________

-*/


#include "uimultiwelllogsel.h"

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

    uiTaskRunner uitr( this );
    Well::InfoCollector wic( false, false, false );
    if ( !uitr.execute(wic) )
	return;

    BufferStringSet wellnms;
    for ( int iid=0; iid<wic.ids().size(); iid++ )
    {
	const DBKey dbky = wic.ids()[iid];
	IOObj* ioobj = dbky.getIOObj();
	if ( !ioobj || (singlewid_ && dbky != *singlewid_) )
	    { delete ioobj; continue; }

	wellobjs_ += ioobj;
	wellnms.add( ioobj->name() );
    }

    if ( wellsfld_ )
	wellsfld_->addItems( wellnms );

    updateLogsFldCB( 0 );
}


void uiMultiWellLogSel::updateLogsFldCB( CallBacker* )
{
    logsfld_->setEmpty();
    DBKeySet dbkys;
    getSelWellIDs( dbkys );
    if ( dbkys.isEmpty() )
	return;

    BufferStringSet availablelognms;
    BufferStringSet availablemrkrs;
    TypeSet<Color> mrkcolors;
    for ( int midx=0; midx<dbkys.size(); midx++ )
    {
	const DBKey dbky = dbkys[midx];
	ConstRefMan<Well::Data> wd = Well::MGR().fetch( dbky,
				    Well::LoadReqs(Well::Trck,Well::Logs) );
	if ( !wd )
	    continue;

	BufferStringSet lognms;
	wd->logs().getNames( lognms );

	BufferStringSet mrkrnms; TypeSet<Color> colors;
	wd->markers().getNames( mrkrnms );
	wd->markers().getColors( colors );
	if ( midx == 0 )
	{
	    availablelognms = lognms;
	    availablemrkrs = mrkrnms;
	    mrkcolors = colors;
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
		if ( !mrkrnms.isPresent(availablemrkrs.get(mrkidx)) )
		{
		    availablemrkrs.removeSingle( mrkidx );
		    mrkcolors.removeSingle( mrkidx );
		}
	    }
	}
    }

    logsfld_->addItems( availablelognms );
    if ( wellextractparamsfld_ )
	wellextractparamsfld_->setMarkers( availablemrkrs, mrkcolors );
}


void uiMultiWellLogSel::getSelWellIDs( DBKeySet& dbkys ) const
{
    if ( singlewid_ && !wellobjs_.isEmpty() )
    {
	dbkys.add( wellobjs_[0]->key() );
    }
    else if ( wellsfld_ )
    {
	for ( int idx=0; idx<wellsfld_->size(); idx++ )
	{
	    if ( wellsfld_->isChosen(idx) )
		dbkys.add( wellobjs_[idx]->key() );
	}
    }
}


void uiMultiWellLogSel::getSelWellIDs( BufferStringSet& wids ) const
{
    DBKeySet dbkys;
    getSelWellIDs( dbkys );
    for ( int idx=0; idx<dbkys.size(); idx++ )
	wids.add( dbkys[idx].toString() );
}


void uiMultiWellLogSel::setSelWellIDs( const BufferStringSet& idstrs )
{
    DBKeySet dbkys;
    for ( int idx=0; idx<idstrs.size(); idx++ )
	dbkys += DBKey( idstrs.get(idx) );

    setSelWellIDs( dbkys );
}


void uiMultiWellLogSel::setSelWellIDs( const DBKeySet& ids )
{
    BufferStringSet wellnms;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = ids[idx].getIOObj();
	if ( ioobj )
	    wellnms.add( ioobj->name() );
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

    DBKeySet dbkys;
    for ( int idx=0; idx<wellschoiceio_->chosenKeys().size(); idx++ )
	dbkys += DBKey( wellschoiceio_->chosenKeys().get(idx) );
    setSelWellIDs( dbkys );
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


/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra / Bert
 Date:		January 2012 / Nov 2016
________________________________________________________________________

-*/


#include "uiwellsel.h"

#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "wellmanager.h"
#include "welltransl.h"

#include "uicompoundparsel.h"
#include "uiioobjselgrp.h"
#include "uiioobjseldlg.h"
#include "uimsg.h"
#include "uistrings.h"

#define mSelTxt seltxt && *seltxt ? seltxt \
				  : ( forread ? "Input Well" : "Output Well" )

ConstRefMan<Well::Data> uiWellSel::getWellData() const
{
    const IOObj* wellioobj = ioobj();
    if ( !wellioobj )
	return 0;

    uiRetVal uirv;
    ConstRefMan<Well::Data> wd = Well::MGR().fetch(
				wellioobj->key(), Well::LoadReqs(), uirv );
    if ( !wd && uirv.isEmpty() )
	uirv.set( tr("No well data found") );

    uiMSG().handleErrors( uirv );
    return wd;
}


RefMan<Well::Data> uiWellSel::getWellDataForEdit() const
{
    const IOObj* wellioobj = ioobj();
    if ( !wellioobj )
	return 0;

    uiRetVal uirv;
    RefMan<Well::Data> wd = Well::MGR().fetchForEdit(
				wellioobj->key(), Well::LoadReqs(), uirv );
    if ( !wd && uirv.isEmpty() )
	uirv.set( tr("No well data found") );

    uiMSG().handleErrors( uirv );
    return wd;
}


uiIOObjSel::Setup uiWellSel::getSetup( bool forread, const uiString& seltxt,
					bool withinserters ) const
{
    uiString st = seltxt;
    if ( st.isEmpty() )
	st = forread ? tr("Input Well") : tr("Output Well");
    uiIOObjSel::Setup su( st );
    su.withinserters( withinserters );
    return su;
}


IOObjContext uiWellSel::getContext( bool forread, bool withinserters ) const
{
    IOObjContext ret( mRWIOObjContext(Well,forread) );
    if ( !withinserters )
	ret.fixTranslator( "dGB" );
    return ret;
}


uiWellSel::uiWellSel( uiParent* p, bool forread, const uiString& seltxt,
			bool withinserters )
    : uiIOObjSel(p,getContext(forread,withinserters),
		 getSetup(forread,seltxt,withinserters))
{
}


uiWellSel::uiWellSel( uiParent* p, bool forread, const uiIOObjSel::Setup& su )
    : uiIOObjSel(p,getContext(forread,su.withinserters_),su)
{
}


class uiWellSingLineMultiSel : public uiCompoundParSel
{ mODTextTranslationClass(uiWellSingLineMultiSel);
public:

uiWellSingLineMultiSel( uiParent* p )
    : uiCompoundParSel(p,uiStrings::sWell(mPlural))
    , selDone(this)
{
    butPush.notify( mCB(this,uiWellSingLineMultiSel,doDlg) );
}

void setSelected( const DBKeySet& ids )
{
    selids_ = ids;
    updSummary(0);
}

void fillPar( IOPar& iop ) const
{
    iop.mergeComp( iopar_, sKey::Well() );
}

    int			nrSelected() const	{ return selids_.size(); }
    void		getSelected( DBKeySet& ids ) const { ids = selids_; }
    bool		usePar(const IOPar&);

    Notifier<uiWellSingLineMultiSel>	selDone;

    void		doDlg(CallBacker*);
    uiString		getSummary() const;

    DBKeySet		selids_;
    IOPar		iopar_;

};


void uiWellSingLineMultiSel::doDlg( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    uiIOObjSelDlg::Setup sdsu( tr("Select Wells") ); sdsu.multisel( true );
    uiIOObjSelDlg dlg( this, sdsu, *ctio );
    uiIOObjSelGrp* selgrp = dlg.selGrp();
    selgrp->usePar( iopar_ );
    if ( !dlg.go() )
	return;

    selids_.erase();
    selgrp->getChosen( selids_ );
    iopar_.setEmpty();
    selgrp->fillPar( iopar_ );

    selDone.trigger();
}


bool uiWellSingLineMultiSel::usePar( const IOPar& iop )
{
    selids_.erase();
    iopar_.setEmpty();
    PtrMan<IOPar> subsel = iop.subselect( sKey::Well() );
    if ( !subsel )
	return false;

    iopar_ = *subsel;

    int nrids;
    iopar_.get( sKey::Size(), nrids );
    for ( int idx=0; idx<nrids; idx++ )
    {
	DBKey dbky;
	if ( iopar_.get(IOPar::compKey(sKey::ID(),idx),dbky) )
	    selids_ += dbky;
    }

    updSummary(0);
    return true;
}


uiString uiWellSingLineMultiSel::getSummary() const
{
    uiStringSet names;
    for ( int idx=0; idx<selids_.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = selids_[idx].getIOObj();
	if ( !ioobj )
	    continue;

	names.add( toUiString( ioobj->name() ) );
    }

    return names.createOptionString();
}



uiMultiWellSel::uiMultiWellSel( uiParent* p, bool singleline,
		const uiIOObjSelGrp::Setup* su )
    : uiGroup(p,"Multi-well selector")
    , singlnfld_(0)
    , multilnfld_(0)
    , newSelection(0)
    , newCurrent(0)
{
    if ( singleline )
	singlnfld_ = new uiWellSingLineMultiSel( this );
    else
    {
	const uiIOObjSelGrp::Setup plainsu( OD::ChooseAtLeastOne );
	if ( !su )
	    su = &plainsu;
	multilnfld_ = new uiIOObjSelGrp( this, mIOObjContext(Well), *su );
    }

    if ( singlnfld_ )
    {
	setHAlignObj( singlnfld_ );
	mAttachCB( singlnfld_->selDone, uiMultiWellSel::newSelectionCB );
    }
    else
    {
	setHAlignObj( multilnfld_ );
	mAttachCB( multilnfld_->itemChosen,
		   uiMultiWellSel::newSelectionCB );
	mAttachCB( multilnfld_->selectionChanged,
		   uiMultiWellSel::newCurrentCB );
    }
}


int uiMultiWellSel::nrWells() const
{
    return multilnfld_ ? multilnfld_->size() : 1;
}


int uiMultiWellSel::nrSelected() const
{
    return singlnfld_ ? singlnfld_->nrSelected() : multilnfld_->nrChosen();
}


void uiMultiWellSel::setSelected( const DBKeySet& ids )
{
    if ( singlnfld_ )
	singlnfld_->setSelected( ids );
    else
	multilnfld_->setChosen( ids );
}


void uiMultiWellSel::getSelected( DBKeySet& ids ) const
{
    if ( singlnfld_ )
	singlnfld_->getSelected( ids );
    else
	multilnfld_->getChosen( ids );
}


DBKey uiMultiWellSel::currentID() const
{
    if ( multilnfld_ )
	return multilnfld_->currentID();

    return !singlnfld_->selids_.isEmpty() ? singlnfld_->selids_[0] : DBKey();
}


void uiMultiWellSel::fillPar( IOPar& iop ) const
{
    if ( singlnfld_ )
	singlnfld_->fillPar( iop );
    else
    {
	IOPar fldiop;
	multilnfld_->fillPar( fldiop );
	iop.mergeComp( fldiop, sKey::Well() );
    }
}


bool uiMultiWellSel::usePar( const IOPar& iop )
{
    if ( singlnfld_ )
	return singlnfld_->usePar( iop );

    PtrMan<IOPar> subsel = iop.subselect( sKey::Well() );
    if ( !subsel )
	return false;

    multilnfld_->usePar( *subsel );
    return true;
}

/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellsel.h"

#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "ptrman.h"
#include "welltransl.h"

#include "uilistbox.h"
#include "uiioobjmanip.h"
#include "uiioobjselgrp.h"
#include "uiioobjseldlg.h"

#define mSelTxt seltxt && *seltxt ? seltxt \
				  : ( forread ? "Input Well" : "Output Well" )

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



uiWellParSel::uiWellParSel( uiParent* p )
    : uiCompoundParSel(p,uiStrings::sWell(mPlural))
    , iopar_(*new IOPar)
    , selDone(this)
{
    butPush.notify( mCB(this,uiWellParSel,doDlg) );
}


uiWellParSel::~uiWellParSel()
{
    delete &iopar_;
}


void uiWellParSel::setSelected( const TypeSet<MultiID>& ids )
{
    selids_ = ids;
    updSummary(0);
}


void uiWellParSel::getSelected( TypeSet<MultiID>& ids ) const
{ ids = selids_; }


void uiWellParSel::doDlg( CallBacker* )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(Well);
    uiIOObjSelDlg::Setup sdsu( tr("Select Wells") ); sdsu.multisel( true );
    uiIOObjSelDlg dlg( this, sdsu, *ctio );
    uiIOObjSelGrp* selgrp = dlg.selGrp();
    selgrp->usePar( iopar_ );
    if ( !dlg.go() ) return;

    selids_.erase();
    selgrp->getChosen( selids_ );
    iopar_.setEmpty();
    selgrp->fillPar( iopar_ );

    selDone.trigger();
}


void uiWellParSel::fillPar( IOPar& iop ) const
{ iop.mergeComp( iopar_, sKey::Well() ); }

bool uiWellParSel::usePar( const IOPar& iop )
{
    selids_.erase();
    iopar_.setEmpty();
    PtrMan<IOPar> subsel = iop.subselect( sKey::Well() );
    if ( !subsel ) return false;

    iopar_ = *subsel;

    int nrids;
    iopar_.get( sKey::Size(), nrids );
    for ( int idx=0; idx<nrids; idx++ )
    {
	MultiID mid;
	if ( iopar_.get(IOPar::compKey(sKey::ID(),idx),mid) )
	    selids_ += mid;
    }

    updSummary(0);
    return true;
}


BufferString uiWellParSel::getSummary() const
{
    BufferStringSet names;
    for ( int idx=0; idx<selids_.size(); idx++ )
    {
	PtrMan<IOObj> ioobj = IOM().get( selids_[idx] );
	if ( !ioobj ) continue;

	names.add( ioobj->name() );
    }

    return names.getDispString( -1, false );
}


// uiMultiWellSel
uiMultiWellSel::uiMultiWellSel( uiParent* p, bool singleline,
		const uiIOObjSelGrp::Setup* su )
    : uiGroup(p,"Multi-well selector")
    , singlnfld_(0)
    , multilnfld_(0)
    , newSelection(0)
    , newCurrent(0)
{
    if ( singleline )
	singlnfld_ = new uiWellParSel( this );
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


void uiMultiWellSel::allowIOObjManip( bool yn )
{
    if ( multilnfld_ && multilnfld_->getManipGroup() )
	multilnfld_->getManipGroup()->display( yn );
}


int uiMultiWellSel::nrSelected() const
{
    return singlnfld_ ? singlnfld_->nrSelected() : multilnfld_->nrChosen();
}


void uiMultiWellSel::setSelected( const TypeSet<MultiID>& ids )
{
    if ( singlnfld_ )
	singlnfld_->setSelected( ids );
    else
	multilnfld_->setChosen( ids );
}


void uiMultiWellSel::getSelected( TypeSet<MultiID>& ids ) const
{
    if ( singlnfld_ )
	singlnfld_->getSelected( ids );
    else
	multilnfld_->getChosen( ids );
}


MultiID uiMultiWellSel::currentID() const
{
    if ( multilnfld_ )
	return multilnfld_->currentID();

    TypeSet<MultiID> selids;
    singlnfld_->getSelected( selids );
    return !selids.isEmpty() ? selids[0] : MultiID::udf();
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

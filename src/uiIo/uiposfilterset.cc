/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposfilterset.cc,v 1.3 2008-02-25 15:05:04 cvsbert Exp $";

#include "uiposfilterset.h"
#include "posfilterset.h"
#include "uiposprovgroup.h"
#include "uimainwin.h"
#include "uilistbox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uidialog.h"
#include "keystrs.h"


uiPosFilterSet::uiPosFilterSet( uiParent* p, const uiPosFilterSet::Setup& su )
	: uiGroup(p,"uiPosFilterSet")
	, setup_(su)
	, selfld_(0)
{
    BufferStringSet nms;

    const BufferStringSet& filtnms( setup_.is2d_
	    		? Pos::Filter2D::factory().getNames()
			: Pos::Filter3D::factory().getNames() );
    for ( int idx=0; idx<filtnms.size(); idx++ )
    {
	const BufferString& nm( filtnms.get(idx) );
	uiPosFiltGroup* grp = uiPosFiltGroup::factory()
	    			.create(nm,this,setup_,true);
	if ( grp )
	{
	    nms.add( nm );
	    grp->setName( nm );
	    grps_ += grp;
	    issel_ += false;
	    isprov_ += false;
	}
    }

    if ( setup_.incprovs_ )
    {
	const BufferStringSet& provnms( setup_.is2d_
			    ? Pos::Filter2D::factory().getNames()
			    : Pos::Filter3D::factory().getNames() );
	for ( int idx=0; idx<provnms.size(); idx++ )
	{
	    const BufferString& nm( provnms.get(idx) );
	    uiPosProvGroup::Setup ppgsu( setup_.is2d_, true );
	    uiPosProvGroup* grp = uiPosProvGroup::factory()
				    .create(nm,this,ppgsu,true);
	    if ( grp )
	    {
		nms.add( nm );
		grp->setName( nm );
		grps_ += grp;
		issel_ += false;
		isprov_ += true;
	    }
	}
    }

    if ( nms.isEmpty() )
    {
	new uiLabel( this, "No position filters available" );
	return;
    }

    const CallBack selcb( mCB(this,uiPosFilterSet,selChg) );
    uiObject* attobj = 0;

    if ( nms.size() == 1 )
    {
	uiLabel* lbl2 = new uiLabel( this, nms.get(0) );
	uiLabel* lbl1 = new uiLabel( this, "Filter: ", lbl2 );
	attobj = lbl2;
    }
    else
    {
	selfld_ = new uiListBox( this, "Filter selection" );
	selfld_->addItems( nms );
	selfld_->selectionChanged.notify( selcb );
	attobj = selfld_;
    }

    ynfld_ = new uiGenInput( this, "Use", BoolInpSpec(false) );
    ynfld_->attach( alignedBelow, attobj );
    ynfld_->valuechanged.notify( mCB(this,uiPosFilterSet,ynChg) );

    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->attach( alignedBelow, ynfld_ );

    setHAlignObj( grps_[0] );
    mainwin()->finaliseDone.notify( selcb );
}


void uiPosFilterSet::ynChg( CallBacker* )
{
    issel_[selIdx()] = ynfld_->getBoolValue();
    selChg( 0 );
}


void uiPosFilterSet::selChg( CallBacker* cb )
{
    if ( grps_.isEmpty() ) return;

    const int selidx = selIdx();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx == selidx && issel_[idx] );

    if ( cb )
	ynfld_->setValue( issel_[selidx] );
}


int uiPosFilterSet::selIdx() const
{
    return selfld_ ? selfld_->currentItem() : 0;
}


void uiPosFilterSet::usePar( const IOPar& iop )
{
    for ( int igrp=0; igrp<grps_.size(); igrp++ )
	issel_[igrp] = false;

    for ( int ipar=0; ; ipar++ )
    {
	PtrMan<IOPar> subiop = iop.subselect(IOPar::compKey(sKey::Filter,ipar));
	if ( !subiop || !subiop->size() ) break;
	const char* typ = subiop->find( sKey::Type );
	if ( !typ ) continue;

	for ( int igrp=0; igrp<grps_.size(); igrp++ )
	{
	    if ( grps_[igrp]->name() == typ )
	    {
		mDynamicCastGet(uiPosFiltGroup*,pfgrp,grps_[igrp])
		mDynamicCastGet(uiPosProvGroup*,ppgrp,grps_[igrp])
		if ( pfgrp ) pfgrp->usePar( *subiop );
		if ( ppgrp ) ppgrp->usePar( *subiop );
		issel_[igrp] = true;
		break;
	    }
	}
    }
    selChg( this );
}


bool uiPosFilterSet::fillPar( IOPar& iop ) const
{
    iop.removeWithKey( IOPar::compKey(sKey::Filter,"*") );
    if ( grps_.isEmpty() ) return true;

    iop.set( sKey::Type, Pos::FilterSet::typeStr() );
    int ipar = 0;
    for ( int igrp=0; igrp<grps_.size(); igrp++ )
    {
	if ( !issel_[igrp] ) continue;

	const BufferString keybase( IOPar::compKey(sKey::Filter,ipar) );
	IOPar subiop;
	subiop.set( sKey::Type, grps_[igrp]->name() );
	mDynamicCastGet(const uiPosFiltGroup*,pfgrp,grps_[igrp])
	mDynamicCastGet(const uiPosProvGroup*,ppgrp,grps_[igrp])
	if ( pfgrp ) pfgrp->fillPar( subiop );
	if ( ppgrp ) ppgrp->fillPar( subiop );
	iop.mergeComp( subiop, keybase );
    }

    return true;
}


uiPosFilterSetSel::uiPosFilterSetSel( uiParent* p,
				      const uiPosFilterSetSel::Setup& su )
    : uiCompoundParSel(p,su.seltxt_)
    , setup_(su)
{
    butPush.notify( mCB(this,uiPosFilterSetSel,doDlg) );
}


BufferString uiPosFilterSetSel::getSummary() const
{
    BufferString txt;
    if ( setup_.is2d_ )
	{ Pos::FilterSet2D pfs; pfs.usePar( iop_ ); pfs.getSummary( txt ); }
    else
	{ Pos::FilterSet3D pfs; pfs.usePar( iop_ ); pfs.getSummary( txt ); }
    return txt;
}


void uiPosFilterSetSel::doDlg( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup("Filters","Specify Filters",
					"0.0.0" ) );
    uiPosFilterSet* pfs = new uiPosFilterSet( &dlg, setup_ );
    pfs->usePar( iop_ );
    if ( dlg.go() )
	pfs->fillPar( iop_ );
}

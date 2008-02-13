/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposprovider.cc,v 1.5 2008-02-13 13:28:48 cvsbert Exp $";

#include "uiposprovgroup.h"
#include "uigeninput.h"
#include "uimainwin.h"
#include "uilabel.h"
#include "keystrs.h"
#include "posprovider.h"
#include "iopar.h"
#include "cubesampling.h"


uiPosProvider::uiPosProvider( uiParent* p, const uiPosProvider::Setup& su )
	: uiGroup(p,"uiPosProvider")
	, setup_(su)
	, selfld_(0)
{
    const BufferStringSet& factnms( setup_.is2d_
	    ? Pos::Provider2D::factory().getNames()
	    : Pos::Provider3D::factory().getNames() );
    BufferStringSet nms;
    BufferStringSet reqnms;
    if ( setup_.choicetype_ != Setup::All )
    {
	reqnms.add( sKey::Range );
	if ( setup_.choicetype_ == Setup::OnlySeisTypes )
	{
	    reqnms.add( sKey::Table );
	    reqnms.add( sKey::Polygon );
	}
    }

    for ( int idx=0; idx<factnms.size(); idx++ )
    {
	const BufferString& nm( factnms.get(idx) );
	if ( !reqnms.isEmpty() && reqnms.indexOf(nm) < 0 )
	    continue;

	uiPosProvGroup* grp = uiPosProvGroup::factory()
	    			.create(nm,this,setup_,true);
	if ( !grp ) continue;

	nms.add( nm );
	grp->setName( nm );
	grps_ += grp;
    }
    if ( setup_.allownone_ )
	nms.add( "All" );

    const CallBack selcb( mCB(this,uiPosProvider,selChg) );
    if ( grps_.size() == 0 )
    {
	new uiLabel( this, "No position providers available" );
	return;
    }

    if ( nms.size() > 1 )
    {
	selfld_ = new uiGenInput( this, setup_.seltxt_, StringListInpSpec(nms));
	for ( int idx=0; idx<grps_.size(); idx++ )
	    grps_[idx]->attach( alignedBelow, selfld_ );
	selfld_->valuechanged.notify( selcb );
    }

    setHAlignObj( grps_[0] );
    mainwin()->finaliseDone.notify( selcb );
}


void uiPosProvider::selChg( CallBacker* )
{
    if ( !selfld_ ) return;
    const int selidx = selfld_->getIntValue();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx == selidx );
}


uiPosProvGroup* uiPosProvider::curGrp() const
{
    if ( grps_.size() < 1 ) return 0;
    const int selidx = selfld_ ? selfld_->getIntValue() : 0;
    return const_cast<uiPosProvGroup*>(
	    selidx < grps_.size() ? grps_[selidx] : 0);
}


void uiPosProvider::usePar( const IOPar& iop )
{
    BufferString typ;
    iop.get( sKey::Type, typ );
    for ( int idx=0; idx<grps_.size(); idx++ )
    {
	if ( typ == grps_[idx]->name() )
	{
	    grps_[idx]->usePar( iop );
	    if ( selfld_ )
		selfld_->setText( grps_[idx]->name() );
	    return;
	}
    }

    if ( selfld_ )
	selfld_->setValue( ((int)0) );
}


bool uiPosProvider::fillPar( IOPar& iop ) const
{
    if ( grps_.size() < 1 )
	return false;

    if ( isAll() )
	iop.set( sKey::Type, sKey::None );

    const uiPosProvGroup* curgrp = curGrp();
    return curgrp ? curgrp->fillPar(iop) : true;
}


Pos::Provider* uiPosProvider::createProvider() const
{
    IOPar iop;
    if ( !fillPar(iop) )
	return 0;

    if ( setup_.is2d_ )
	return Pos::Provider2D::make( iop );
    else
	return Pos::Provider3D::make( iop );
}

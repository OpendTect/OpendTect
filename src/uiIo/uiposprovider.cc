/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        R. K. Singh
 Date:          Aug 2007
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiposprovider.cc,v 1.1 2008-02-06 16:04:32 cvsbert Exp $";

#include "uiposprovider.h"
#include "uigeninput.h"
#include "uimainwin.h"
#include "uilabel.h"
#include "keystrs.h"
#include "posprovider.h"
#include "iopar.h"

mImplFactory1Param(uiPosProvGroup,uiParent*,uiPosProvGroup::factory);


uiPosProvider::uiPosProvider( uiParent* p, const uiPosProvider::Setup& su )
	: uiGroup(p,"uiPosProvider")
	, setup_(su)
	, selfld_(0)
{
    Factory<Pos::Provider*>* fact;
    if ( setup_.is2d_ )
	fact = (Factory<Pos::Provider*>*)(&Pos::Provider2D::factory());
    else
	fact = (Factory<Pos::Provider*>*)(&Pos::Provider3D::factory());

    const BufferStringSet& factnms( fact->getNames() );
    BufferStringSet nms;
    BufferStringSet reqnms;
    if ( setup_.choicetype_ != Setup::All )
    {
	reqnms.add( setup_.is2d_ ? "Segment" : "Volume" );
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

	uiPosProvGroup* grp = uiPosProvGroup::factory().create(nm,this,true);
	if ( !grp ) continue;

	nms.add( nm );
	grp->setName( nm );
	grps_ += grp;
    }

    const CallBack selcb( mCB(this,uiPosProvider,selChg) );
    if ( grps_.size() == 0 )
	new uiLabel( this, "No position providers available" );
    if ( grps_.size() > 1 )
    {
	selfld_ = new uiGenInput( this, setup_.seltxt_, StringListInpSpec(nms));
	for ( int idx=0; idx<grps_.size(); idx++ )
	    grps_[idx]->attach( alignedBelow, selfld_ );
	selfld_->valuechanged.notify( selcb );
    }

    mainwin()->finaliseDone.notify( selcb );
}


void uiPosProvider::selChg( CallBacker* )
{
    if ( !selfld_ ) return;
    const int selidx = selfld_->getIntValue();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx == selidx );
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
}


bool uiPosProvider::fillPar( IOPar& iop ) const
{
    if ( grps_.size() < 1 ) return false;

    const int selidx = selfld_ ? selfld_->getIntValue() : 0;
    iop.set( sKey::Type, grps_[selidx]->name() );
    return grps_[selidx]->fillPar(iop);
}

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uiposfilterset.cc,v 1.17 2012-07-27 09:46:03 cvsbert Exp $";

#include "uiposfilterset.h"
#include "posfilterset.h"
#include "posprovider.h"
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
    , ynfld_(0)
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
	}
    }

    if ( setup_.incprovs_ )
    {
	const BufferStringSet& provnms( setup_.is2d_
			    ? Pos::Provider2D::factory().getNames()
			    : Pos::Provider3D::factory().getNames() );
	for ( int idx=0; idx<provnms.size(); idx++ )
	{
	    const BufferString& nm( provnms.get(idx) );
	    uiPosProvGroup::Setup ppgsu( setup_.is2d_, true, true );
	    uiPosProvGroup* grp = uiPosProvGroup::factory()
				    .create(nm,this,ppgsu,true);
	    if ( grp )
	    {
		nms.add( nm );
		grp->setName( nm );
		grps_ += grp;
		issel_ += false;
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
	uiLabel* nmlbl = new uiLabel( this, nms.get(0) );
	new uiLabel( this, "Filter: ", nmlbl );
	ynfld_ = new uiGenInput( this, "Use", BoolInpSpec(false) );
	ynfld_->attach( alignedBelow, nmlbl );
	attobj = ynfld_->attachObj();
    }
    else
    {
	selfld_ = new uiListBox( this, "Filter selection" );
	selfld_->addItems( nms );
	selfld_->selectionChanged.notify( selcb );
	int ph = nms.size()-1; if ( ph > 9 ) ph = 9;
	selfld_->setPrefHeightInChar( ph );
	attobj = selfld_;
	for ( int idx=0; idx<nms.size(); idx++ )
	    selfld_->setItemCheckable( idx, true );
    }

    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->attach( alignedBelow, attobj );

    setHAlignObj( grps_[0] );
    postFinalise().notify( selcb );
}


void uiPosFilterSet::selChg( CallBacker* cb )
{
    if ( grps_.isEmpty() ) return;

    const int selidx = selIdx();
    for ( int idx=0; idx<grps_.size(); idx++ )
	grps_[idx]->display( idx == selidx );
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
	PtrMan<IOPar> subiop = iop.subselect(IOPar::compKey(sKey::Filter(),ipar));
	if ( !subiop || !subiop->size() ) break;
	const char* typ = subiop->find( sKey::Type() );
	if ( !typ ) continue;

	for ( int igrp=0; igrp<grps_.size(); igrp++ )
	{
	    if ( grps_[igrp]->name() == typ )
	    {
		grps_[igrp]->usePar( *subiop );
		issel_[igrp] = true;
		break;
	    }
	}
    }

    if ( ynfld_ ) ynfld_->setValue( issel_[0] );
    else
    {
	for ( int idx=0; idx<issel_.size(); idx++ )
	    selfld_->setItemChecked( idx, issel_[idx] );
    }
    selChg( this );
}


bool uiPosFilterSet::fillPar( IOPar& iop ) const
{
    iop.removeWithKey( IOPar::compKey(sKey::Filter(),"*") );
    if ( grps_.isEmpty() ) return true;

    iop.set( sKey::Type(), Pos::FilterSet::typeStr() );
    int ipar = 0;
    for ( int igrp=0; igrp<grps_.size(); igrp++ )
    {
	if ( (ynfld_ && !ynfld_->getBoolValue()) || 
	     (selfld_ && !selfld_->isItemChecked(igrp) ) )
	    continue;

	const BufferString keybase( IOPar::compKey(sKey::Filter(),ipar) );
	IOPar subiop;
	subiop.set( sKey::Type(), grps_[igrp]->name() );
	grps_[igrp]->fillPar( subiop );
	iop.mergeComp( subiop, keybase );

	ipar++;
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
					"111.1.2" ) );
    uiPosFilterSet* pfs = new uiPosFilterSet( &dlg, setup_ );
    pfs->usePar( iop_ );
    if ( dlg.go() )
	pfs->fillPar( iop_ );
}

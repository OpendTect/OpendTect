/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		March 2019
________________________________________________________________________

-*/


#include "uiseisstorer.h"

#include "keystrs.h"
#include "scaler.h"
#include "seisgeomtypeprovider.h"
#include "seisioobjinfo.h"
#include "seisstorer.h"

#include "uiscaler.h"
#include "uiseissel.h"
#include "uistrings.h"



uiSeisStorer::uiSeisStorer( uiParent* p, GeomType gt, const Setup& su )
    : uiGroup(p,"Seismic Storer Selector (single GT)")
    , setup_(su)
    , selectionChanged(this)
    , gtprov_(*new Seis::SingleGeomTypeProvider(gt))
    , gtprovmine_(true)
{
    createFlds();
}


uiSeisStorer::uiSeisStorer( uiParent* p, const GeomTypeProvider& gtp,
			    const Setup& su )
    : uiGroup(p,"Seismic Storer Selector (GTProv)")
    , setup_(su)
    , selectionChanged(this)
    , gtprov_(gtp)
{
    createFlds();
}


uiSeisStorer::~uiSeisStorer()
{
    if ( gtprovmine_ )
	delete &gtprov_;
}


void uiSeisStorer::createFlds()
{
    if ( setup_.allowscale_ )
	scalefld_ = new uiScaler( this );

    const auto gts( gtprov_.availableTypes() );
    for ( auto gt : gts )
    {
	uiSeisSel::Setup sssu( gt );
	sssu.optional( setup_.optional_ ).seltxt( setup_.seltxt_ );
	IOObjContext ctxt( uiSeisSel::ioContext(gt,false) );
	ctxt.toselect_.allowtransls_.set( setup_.allowtransls_ );
	auto* sel = new uiSeisSel( this, ctxt, sssu );
	seissels_.add( sel );
	if ( scalefld_ )
	    sel->attach( alignedBelow, scalefld_ );
    }
    setHAlignObj( seissels_.first() );
}


void uiSeisStorer::initGrp( CallBacker* )
{
    updUi();

    for ( auto ss : seissels_ )
    {
	mAttachCB( ss->selectionDone, uiSeisStorer::selChgCB );
	mAttachCB( ss->optionalChecked, uiSeisStorer::optChgCB );
    }
}


int uiSeisStorer::curSelIdx() const
{
    const auto gt = gtprov_.geomType();
    return gtprov_.availableTypes().indexOf( gt );
}


void uiSeisStorer::updUi()
{
    const auto curidx = curSelIdx();
    const bool isactive = seissels_[curidx]->isChecked();
    for ( int idx=0; idx<seissels_.size(); idx++ )
	seissels_[idx]->display( isactive && idx == curidx );
}


void uiSeisStorer::optChgCB( CallBacker* )
{
    updUi();
}


void uiSeisStorer::selChgCB( CallBacker* )
{
    selectionChanged.trigger();
}


void uiSeisStorer::set( const DBKey& dbky )
{
    SeisIOObjInfo objinf( dbky );
    const auto curidx = curSelIdx();
    const auto newidx = gtprov_.availableTypes().indexOf( objinf.geomType() );
    if ( newidx < 0 )
	{ pErrMsg( "GeomType not supported" ); }
    else if ( newidx != curidx )
	{ pErrMsg( "Set uiProvider's GeomType first" ); }
    else
	seissels_[curidx]->setInput( dbky );
}


DBKey uiSeisStorer::key() const
{
    const auto curidx = curSelIdx();
    return seissels_[curidx]->key( true );
}


bool uiSeisStorer::isOK( bool showerr ) const
{
    const auto curidx = curSelIdx();
    return seissels_[curidx]->ioobj( !showerr );
}


Seis::Storer* uiSeisStorer::get( bool showerr ) const
{
    const auto curidx = curSelIdx();
    const IOObj* ioobj = seissels_[curidx]->ioobj( !showerr );
    if ( !ioobj )
	return 0;

    Seis::Storer* ret = new Seis::Storer( *ioobj );
    if ( scalefld_ )
    {
	Scaler* sclr = scalefld_->getScaler();
	if ( sclr && sclr->isEmpty() )
	    delete sclr;
	else if ( sclr )
	    ret->setScaler( sclr );
    }

    return ret;
}


void uiSeisStorer::fillPar( IOPar& iop ) const
{
    const auto curidx = curSelIdx();
    const DBKey dbky = seissels_[curidx]->key( true );
    if ( !dbky.isValid() )
	iop.removeWithKey( sKey::ID() );
    else
    {
	iop.set( sKey::ID(), dbky );
	if ( scalefld_ )
	    scalefld_->fillPar( iop );
    }
}


void uiSeisStorer::usePar( const IOPar& iop )
{
    const auto curidx = curSelIdx();
    const DBKey curdbky = seissels_[curidx]->key( true );
    DBKey dbky = curdbky;
    iop.get( sKey::ID(), dbky );
    if ( dbky.isValid() && dbky != curdbky )
	set( dbky );

    if ( scalefld_ )
	scalefld_->usePar( iop );
}

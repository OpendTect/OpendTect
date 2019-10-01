/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2016
________________________________________________________________________

-*/

#include "uipicksettools.h"
#include "picksetmanager.h"
#include "picksettr.h"
#include "keystrs.h"
#include "uiioobjselgrp.h"
#include "uimsg.h"


template <class T>
static ODPolygon<T>* gtPolygon( const uiPickSetIOObjSel* objsel )
{
    ConstRefMan<Pick::Set> ps = objsel->getPickSet( false );
    if ( !ps )
	return 0;

    ODPolygon<T>* ret = new ODPolygon<T>;
    ps->getPolygon( *ret );
    return ret;
}


IOObjContext uiPickSetIOObjSel::getCtxt( Type typ, bool forread,
					 const char* cat, const char* transl )
{
    IOObjContext ret( mIOObjContext(PickSet) );
    updateCtxt( ret, typ, forread, cat, transl );
    return ret;
}

void uiPickSetIOObjSel::updateCtxt( IOObjContext& ctxt, Type typ, bool forread,
				    const char* cat, const char* transl )
{
    ctxt.forread_ = forread;

    ctxt.toselect_.require_.removeWithKey( sKey::Type() );
    ctxt.toselect_.dontallow_.removeWithKey( sKey::Type() );
    if ( typ == uiPickSetIOObjSel::PolygonOnly )
	ctxt.toselect_.require_.set( sKey::Type(), sKey::Polygon() );
    else if ( typ == uiPickSetIOObjSel::NoPolygon )
	ctxt.toselect_.dontallow_.set( sKey::Type(), sKey::Polygon() );

    ctxt.toselect_.require_.removeWithKey( sKey::Category() );
    ctxt.toselect_.require_.update( sKey::Category(), cat );
    if ( transl && *transl )
	ctxt.fixTranslator( transl );
}


#define mSetLabel() \
    if ( typ == PolygonOnly ) \
	setLabelText( uiStrings::sPolygon() ); \
    else \
	setLabelText( uiStrings::sPointSet() )


uiPickSetIOObjSel::uiPickSetIOObjSel( uiParent* p, bool forread, Type typ,
				      const char* cat )
    : uiIOObjSel(p,getCtxt(typ,forread,cat))
{
    mSetLabel();
}


uiPickSetIOObjSel::uiPickSetIOObjSel( uiParent* p, const Setup& su,
				      bool forread, Type typ, const char* cat )
    : uiIOObjSel(p,getCtxt(typ,forread,cat),su)
{
    mSetLabel();
}


ConstRefMan<Pick::Set> uiPickSetIOObjSel::getPickSet( bool emptyok ) const
{
    const IOObj* psioobj = ioobj();
    if ( !psioobj )
	return 0;

    uiRetVal uirv;
    ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( psioobj->key(), uirv );
    if ( ps && !emptyok && ps->isEmpty() )
	{ uirv.set( tr("No locations found") ); ps = 0; }

    uiMSG().handleErrors( uirv );
    return ps;
}


RefMan<Pick::Set> uiPickSetIOObjSel::getPickSetForEdit( bool emptyok ) const
{
    const IOObj* psioobj = ioobj();
    if ( !psioobj )
	return 0;

    uiRetVal uirv;
    RefMan<Pick::Set> ps = Pick::SetMGR().fetchForEdit( psioobj->key(), uirv );
    if ( ps && !emptyok && ps->isEmpty() )
	{ uirv = tr("No locations found"); ps = 0; }

    uiMSG().handleErrors( uirv );
    return ps;
}


ODPolygon<float>* uiPickSetIOObjSel::getSelectionPolygon() const
{
    return gtPolygon<float>( this );
}


ODPolygon<double>* uiPickSetIOObjSel::getCoordPolygon() const
{
    return gtPolygon<double>( this );
}



uiMergePickSets::uiMergePickSets( uiParent* p, DBKey& setid )
    : uiDialog(p,uiDialog::Setup(uiStrings::phrMerge(uiStrings::sPointSet()),
				 tr("Specify sets to merge"),
				 mODHelpKey(mMergePickSetsHelpID) ))
    , setid_(setid)
{
    IOObjContext ctxt( PickSetTranslatorGroup::ioContext() );
    selfld_ = new uiIOObjSelGrp( this, ctxt, uiStrings::phrSelect(
				uiStrings::phrInput(uiStrings::sPointSet(2))),
				uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );
    selfld_->setCurrent( setid );

    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			     uiStrings::phrOutput( tr("Merged Set")) );
    outfld_->attach( alignedBelow, selfld_ );
}


bool uiMergePickSets::acceptOK()
{
    if ( selfld_->nrChosen() < 2 )
    {
	uiMSG().error( uiStrings::phrSelect(tr("at least two sets to merge")) );
	return false;
    }
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    IOPar ioobjpars;
    RefMan<Pick::Set> ps = getMerged( ioobjpars );
    if ( !ps )
	return false;

    const DBKey setid( outioobj->key() );
    SilentTaskRunnerProvider trprov;
    uiRetVal uirv = ::Pick::SetMGR().store( *ps, setid, trprov, &ioobjpars );
    if ( uirv.isError() )
	{ uiMSG().error( uirv ); return false; }

    setid_ = setid;
    return true;
}


RefMan<Pick::Set> uiMergePickSets::getMerged( IOPar& ioobjpars ) const
{
    DBKeySet inpids;
    selfld_->getChosen( inpids );

    RefMan<Pick::Set> outps = new Pick::Set;
    for ( int idx=0; idx<inpids.size(); idx++ )
    {
	const DBKey id = inpids[idx];
	uiRetVal uirv;
	ConstRefMan<Pick::Set> ps = Pick::SetMGR().fetch( id, uirv );
	if ( !ps )
	    { uiMSG().error( uirv ); return 0; }

	if ( idx == 0 )
	{
	    outps->setDisp( ps->getDisp() );
	    ioobjpars = Pick::SetMGR().getIOObjPars( id );
	    ioobjpars.removeWithKey( sKey::Type() );
	    outps->setConnection( Pick::Set::Disp::None );
	}
	outps->append( *ps );
    }
    if ( outps->isEmpty() )
	{ uiMSG().error( tr("No valid locations found") ); return 0; }

    return outps;
}

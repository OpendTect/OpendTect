/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          May 2001
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uipicksetmgr.h"
#include "uiimppickset.h"
#include "uiioobj.h"
#include "uiioobjsel.h"
#include "uimsg.h"

#include "ctxtioobj.h"
#include "keystrs.h"
#include "ioman.h"
#include "ioobj.h"
#include "pickset.h"
#include "picksettr.h"
#include "ptrman.h"


uiPickSetMgr::uiPickSetMgr( Pick::SetMgr& m )
    	: setmgr_(m)
{
}


bool uiPickSetMgr::storeNewSet( Pick::Set*& ps ) const
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->setName( ps->name() );
    if ( uiIOObj::fillCtio(*ctio,true) )
    {
	PtrMan<IOObj> ioobj = ctio->ioobj;
	if ( ps->disp_.connect_ == Pick::Set::Disp::None )
	    ioobj->pars().set( sKey::Type(),
			       PickSetTranslatorGroup::sKeyPickSet() );
	else
	    ioobj->pars().set( sKey::Type(), sKey::Polygon() );

	if ( !doStore( *ps, *ioobj ) )
	    { delete ps; ps = 0; return false; }

	setmgr_.set( ioobj->key(), ps );
	return true;
    }

    delete ps; ps = 0;
    return false;
}


IOObj* uiPickSetMgr::getSetIOObj( const Pick::Set& ps ) const
{
    int setidx = setmgr_.indexOf( ps );
    if ( setidx < 0 ) return 0;

    IOObj* ioobj = IOM().get( setmgr_.id(setidx) );
    if ( !ioobj )
    {
	BufferString msg( "The PickSet '" );
	msg += ps.name();
	msg += "' no longer has an entry in the data store.\n"
	       "Please use 'Save As' to store this set.";
	uiMSG().warning( msg );
    }
    return ioobj;
}


bool uiPickSetMgr::storeSet( const Pick::Set& ps )
{
    PtrMan<IOObj> ioobj = getSetIOObj( ps );
    if ( !ioobj || !doStore(ps,*ioobj) )
	return false;

    setmgr_.setUnChanged( setmgr_.indexOf(ps) );
    return true;
}


bool uiPickSetMgr::storeSets()
{
    for ( int idx=0; idx<setmgr_.size(); idx++ )
    {
	if ( !setmgr_.isChanged(idx) )
	    continue;

	storeSet( setmgr_.get(idx) );
    }
    return true;
}


bool uiPickSetMgr::pickSetsStored() const
{
    for ( int idx=0; idx<setmgr_.size(); idx++ )
    {
	if ( setmgr_.isChanged(idx) )
	    return false;
    }
    return true;
}


bool uiPickSetMgr::storeSetAs( const Pick::Set& ps )
{
    const bool ispoly = ps.disp_.connect_ != Pick::Set::Disp::None;
    const BufferString oldname = ps.name();
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj( PickSet );
    ctio->ctxt.forread = false;

    if ( ispoly )
	ctio->ctxt.toselect.require_.set( sKey::Type(), sKey::Polygon() );
    ctio->setName( oldname );
    uiIOObjSelDlg dlg( parent(), *ctio );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    if ( !doStore( ps, *dlg.ioObj() ) )
	return false;

    const_cast<Pick::Set&>(ps).setName( dlg.ioObj()->name() );
    const int psidx = setmgr_.indexOf( ps );
    if ( psidx >= 0 )
	setmgr_.setID( psidx, dlg.ioObj()->key() );
    setmgr_.reportChange( this, ps );
    return true;
}


bool uiPickSetMgr::doStore( const Pick::Set& ps, const IOObj& ioobj ) const
{
    IOM().commitChanges( ioobj );
    BufferString bs;
    if ( !PickSetTranslator::store( ps, &ioobj, bs ) )
	{ uiMSG().error(bs); return false; }

    return true;
}


class uiMergePickSets : public uiDialog
{
public:

uiMergePickSets( uiParent* p, MultiID& mid )
    : uiDialog(p,uiDialog::Setup("Merge Pick Sets","Specify sets to merge",
				 "105.0.4"))
    , ctioin_( PickSetTranslatorGroup::ioContext() )
    , ctioout_( PickSetTranslatorGroup::ioContext() )
    , mid_(mid)
{
    ctioin_.setObj( IOM().get(mid_) );
    selfld = new uiIOObjSelGrp( this, ctioin_, "Select Pick Sets to merge",
	    			true );
    ctioout_.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctioout_, "Output merged set" );
    outfld->attach( alignedBelow, selfld );
}


bool acceptOK( CallBacker* )
{
    nrsel = selfld->nrSel();
    if ( nrsel < 2 )
    {
	uiMSG().error( "Please select at least two sets" );
	return false;
    }
    else if ( !outfld->commitInput() )
    {
	uiMSG().error( "Cannot create the output set" );
	return false;
    }

    if ( ctioout_.ioobj )
	mid_ = ctioout_.ioobj->key();
    return true;
}

    uiIOObjSelGrp*	selfld;
    uiIOObjSel*		outfld;
    CtxtIOObj		ctioin_;
    CtxtIOObj		ctioout_;
    MultiID&		mid_;

    int			nrsel;

};


void uiPickSetMgr::mergeSets( MultiID& mid )
{
    CtxtIOObj ctio( PickSetTranslatorGroup::ioContext() );
    uiMergePickSets dlg( parent(), mid );
    if ( !dlg.go() ) return;

    ObjectSet<const Pick::Set> pss;
    ObjectSet<Pick::Set> pssread;
    for ( int idx=0; idx<dlg.nrsel; idx++ )
    {
	const MultiID& ky = dlg.selfld->selected( idx );
	int setidx = setmgr_.indexOf( ky );
	if ( setidx >= 0 )
	    pss += &setmgr_.get( setidx );
	else
	{
	    Pick::Set* newset = new Pick::Set;
	    IOObj* ioobj = IOM().get( ky );
	    BufferString msg;
	    if ( PickSetTranslator::retrieve(*newset,ioobj,true, msg) )
		{ pss += newset; pssread += newset; }
	    else
		uiMSG().warning( msg );
	    delete ioobj;
	}
    }

    if ( pss.size() < 2 )
    {
	uiMSG().error( "Not enough valid sets selected for merge" );
	deepErase( pssread ); return;
    }

    Pick::Set resset( *pss[0] );
    resset.setName( dlg.ctioout_.ioobj->name() );
    for ( int idx=1; idx<pss.size(); idx ++ )
	resset.append( *pss[idx] );

    BufferString msg;
    if ( !PickSetTranslator::store(resset,dlg.ctioout_.ioobj,msg) )
	uiMSG().error( msg );

    dlg.ctioout_.ioobj->pars().set( sKey::Type(),
	    PickSetTranslatorGroup::sKeyPickSet() );
    IOM().commitChanges( *dlg.ctioout_.ioobj );
    deepErase( pssread );
}

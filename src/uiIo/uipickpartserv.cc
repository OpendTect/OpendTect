/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          May 2001
 RCS:           $Id: uipickpartserv.cc,v 1.37 2006-06-28 15:58:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "uipickpartserv.h"
#include "uifetchpicks.h"
#include "uiimppickset.h"
#include "uiioobj.h"
#include "uimsg.h"
#include "uiioobjsel.h"
#include "uicursor.h"
#include "pickset.h"
#include "picksettr.h"
#include "surfaceinfo.h"
#include "ctxtioobj.h"
#include "color.h"
#include "ioobj.h"
#include "ioman.h"
#include "iopar.h"
#include "survinfo.h"
#include "stats.h"
#include "ptrman.h"

const int uiPickPartServer::evGetHorInfo = 0;
const int uiPickPartServer::evGetHorDef = 1;


uiPickPartServer::uiPickPartServer( uiApplService& a )
	: uiApplPartServer(a)
    	, gendef(2,true)
	, setmgr(Pick::Mgr())
{
}


uiPickPartServer::~uiPickPartServer()
{
    deepErase( selhorids );
}


void uiPickPartServer::impexpSet( bool import )
{
    uiImpExpPickSet dlg( appserv().parent(), import );
    dlg.go();
}


bool uiPickPartServer::storeNewSet( Pick::Set*& ps ) const
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->setName( ps->name() );
    if ( uiIOObj::fillCtio(*ctio,true) )
    {
	PtrMan<IOObj> ioobj = ctio->ioobj;
	if ( !doStore( *ps, *ioobj ) )
	    { delete ps; ps = 0; return false; }

	setmgr.set( ioobj->key(), ps );
	return true;
    }

    delete ps; ps = 0;
    return false;
}


bool uiPickPartServer::fetchSets()
{
    deepErase( hinfos );
    sendEvent( evGetHorInfo );
    BufferStringSet hornms;
    for ( int idx=0; idx<hinfos.size(); idx++ )
	hornms.add( hinfos[idx]->name );

    uiFetchPicks dlg( appserv().parent(), setmgr, hornms );
    if ( !dlg.go() )
	{ deepErase( hinfos ); return false; }

    bool rv = false;
    if ( dlg.mkNew() )
    {
	Pick::Set* newps = new Pick::Set( dlg.getName() );
	newps->disp_.color_ = dlg.getPickColor();
	if ( dlg.genRand() )
	{
	    if ( !mkRandLocs(*newps,dlg.randPars()) )
		{ delete newps; newps = 0; }
	}
	if ( newps )
	    rv = storeNewSet( newps );
    }
    else
    {
	for ( int idx=0; dlg.ioobj(idx); idx++ )
	{
	    const IOObj* ioobj = dlg.ioobj( idx );
	    if ( setmgr.indexOf(ioobj->name()) >= 0 )
		continue;

	    Pick::Set* newps = new Pick::Set;
	    BufferString bs;
	    if ( PickSetTranslator::retrieve(*newps,ioobj,bs) )
	    {
		rv = true;
		setmgr.set( ioobj->key(), newps );
	    }
	    else
	    {
		delete newps;
		if ( idx == 0 )
		{
		    uiMSG().error( bs );
		    return false;
		}
		else
		{
		    BufferString msg( dlg.ioobj(idx)->name() );
		    msg += ": "; msg += bs;
		    uiMSG().warning( msg );
		}
	    }
	}
    }

    return rv;
}


bool uiPickPartServer::mkRandLocs( Pick::Set& ps, const RandLocGenPars& rp )
{
    uiCursorChanger cursorlock( uiCursor::Wait );

    Stat_initRandom(0);
    selbr = &rp.bidrg;
    const bool do2hors = !rp.iscube && rp.horidx2 >= 0 && 
			 rp.horidx2 != rp.horidx;
    gendef.empty();
    deepErase( selhorids );
    if ( !rp.iscube )
    {
	selhorids += new MultiID( hinfos[rp.horidx]->multiid );
	if ( do2hors )
	    selhorids += new MultiID( hinfos[rp.horidx2]->multiid );
	sendEvent( evGetHorDef );
    }
    else
    {
	const BinID stp = BinID( SI().inlStep(), SI().crlStep() );
	BinID bid;
	for ( bid.inl=selbr->start.inl; bid.inl<=selbr->stop.inl;
		bid.inl +=stp.inl )
	{
	    for ( bid.crl=selbr->start.crl; bid.crl<=selbr->stop.crl;
		    	bid.crl += stp.crl )
		gendef.add( bid, rp.zrg.start, rp.zrg.stop );
	}
    }

    const int nrpts = gendef.totalSize();
    if ( !nrpts ) return true;

    BinID bid; Interval<float> zrg;
    for ( int ipt=0; ipt<rp.nr; ipt++ )
    {
	const int ptidx = Stat_getIndex( nrpts );
	BinIDValueSet::Pos pos = gendef.getPos( ptidx );
	gendef.get( pos, bid, zrg.start, zrg.stop );
	float val = zrg.start + Stat_getRandom() * zrg.width();

	ps += Pick::Location( SI().transform(bid), val );
    }

    gendef.empty();
    return true;
}


IOObj* uiPickPartServer::getSetIOObj( const Pick::Set& ps ) const
{
    int setidx = setmgr.indexOf( ps );
    if ( setidx < 0 ) return 0;

    IOObj* ioobj = IOM().get( setmgr.id(setidx) );
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


bool uiPickPartServer::storeSet( const Pick::Set& ps )
{
    PtrMan<IOObj> ioobj = getSetIOObj( ps );
    if ( !ioobj || !doStore(ps,*ioobj) )
	return false;

    setmgr.setUnChanged( setmgr.indexOf(ps) );
    return true;
}


bool uiPickPartServer::storeSets()
{
    for ( int idx=0; idx<setmgr.size(); idx++ )
    {
	if ( !setmgr.isChanged(idx) )
	    continue;

	storeSet( setmgr.get(idx) );
    }
    return true;
}


bool uiPickPartServer::pickSetsStored() const
{
    for ( int idx=0; idx<setmgr.size(); idx++ )
    {
	if ( setmgr.isChanged(idx) )
	    return false;
    }
    return true;
}


bool uiPickPartServer::storeSetAs( const Pick::Set& ps )
{
    const BufferString oldname = ps.name();
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(PickSet);
    ctio->ctxt.forread = false;
    ctio->ctxt.maychdir = false;
    ctio->setName( oldname );
    uiIOObjSelDlg dlg( appserv().parent(), *ctio );
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    if ( !doStore( ps, *dlg.ioObj() ) )
	return false;

    const_cast<Pick::Set&>(ps).setName( dlg.ioObj()->name() );
    setmgr.reportChange( this, ps );
    return true;
}


bool uiPickPartServer::doStore( const Pick::Set& ps, const IOObj& ioobj ) const
{
    BufferString bs;
    if ( !PickSetTranslator::store( ps, &ioobj, bs ) )
	{ uiMSG().error(bs); return false; }

    return true;
}


void uiPickPartServer::setMisclassSet( const BinIDValueSet& bivs )
{
    static const char* sKeyMisClass = "Misclassified [NN]";
    int setidx = setmgr.indexOf( sKeyMisClass );
    const bool isnew = setidx < 0;
    Pick::Set* ps = isnew ? 0 : &setmgr.get( setidx );
    if ( ps )
	ps->erase();
    else
    {
	ps = new Pick::Set( sKeyMisClass );
	ps->disp_.color_.set( 240, 0, 0 );
    }

    BinIDValueSet::Pos pos; BinIDValue biv;
    while ( bivs.next(pos,false) )
    {
	bivs.get( pos, biv );
	Coord pos = SI().transform( biv.binid );
	*ps += Pick::Location( pos.x, pos.y, biv.value );
    }

    if ( isnew )
	storeNewSet( ps );
    else
    {
	PtrMan<IOObj> ioobj = getSetIOObj( *ps );
	if ( ioobj )
	    doStore( *ps, *ioobj );
    }
}


class uiMergePickSets : public uiDialog
{
public:

uiMergePickSets( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Merge Pick Sets","Specify sets to merge"))
    , ctioin( PickSetTranslatorGroup::ioContext() )
    , ctioout( PickSetTranslatorGroup::ioContext() )
{
    selfld = new uiIOObjSelGrp( this, ctioin, "Select Pick Sets to merge",
	    			true );
    ctioout.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctioout, "Output merged set" );
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

    if ( !outfld->commitInput(true) )
    {
	uiMSG().error( "Cannot create the output set" );
	return false;
    }
    return true;
}

    uiIOObjSelGrp*	selfld;
    uiIOObjSel*		outfld;
    CtxtIOObj		ctioin;
    CtxtIOObj		ctioout;

    int			nrsel;

};


void uiPickPartServer::mergeSets()
{
    CtxtIOObj ctio( PickSetTranslatorGroup::ioContext() );
    uiMergePickSets dlg( appserv().parent() );
    if ( !dlg.go() ) return;

    ObjectSet<const Pick::Set> pss;
    ObjectSet<Pick::Set> pssread;
    for ( int idx=0; idx<dlg.nrsel; idx++ )
    {
	const MultiID& ky = dlg.selfld->selected( idx );
	int setidx = Pick::Mgr().indexOf( ky );
	if ( setidx >= 0 )
	    pss += &Pick::Mgr().get( setidx );
	else
	{
	    Pick::Set* newset = new Pick::Set;
	    IOObj* ioobj = IOM().get( ky );
	    BufferString msg;
	    if ( PickSetTranslator::retrieve(*newset,ioobj,msg) )
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
    resset.setName( dlg.ctioout.ioobj->name() );
    for ( int idx=1; idx<pss.size(); idx ++ )
	resset.append( *pss[idx] );

    BufferString msg;
    if ( !PickSetTranslator::store(resset,dlg.ctioout.ioobj,msg) )
	uiMSG().error( msg );

    deepErase( pssread );
}

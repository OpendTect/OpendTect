/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2001
________________________________________________________________________

-*/

#include "uipickpartserv.h"

#include "uicreatepicks.h"
#include "uiimppickset.h"
#include "uipicksettools.h"
#include "uiioobjseldlg.h"
#include "uipicksetman.h"
#include "uitaskrunner.h"
#include "uimsg.h"

#include "binnedvalueset.h"
#include "color.h"
#include "datapointset.h"
#include "executor.h"
#include "dbman.h"
#include "ioobj.h"
#include "picksetmanager.h"
#include "picksettr.h"
#include "picksetio.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "statrand.h"
#include "surfaceinfo.h"
#include "survgeom2d.h"
#include "survinfo.h"

int uiPickPartServer::evGetHorInfo2D()		{ return 0; }
int uiPickPartServer::evGetHorInfo3D()		{ return 1; }
int uiPickPartServer::evGetHorDef3D()		{ return 2; }
int uiPickPartServer::evGetHorDef2D()		{ return 3; }
int uiPickPartServer::evFillPickSet()		{ return 4; }
int uiPickPartServer::evDisplayPickSet()	{ return 7; }


uiPickPartServer::uiPickPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , gendef_(*new BinnedValueSet(2,true))
    , selhs_(true)
    , ps_(0)
    , imppsdlg_(0)
    , exppsdlg_(0)
    , manpicksetsdlg_(0)
{
    mAttachCB( DBM().surveyChanged, uiPickPartServer::survChangedCB );
}


uiPickPartServer::~uiPickPartServer()
{
    detachAllNotifiers();
    delete &gendef_;
    delete manpicksetsdlg_;
}


void uiPickPartServer::survChangedCB( CallBacker* )
{
    delete imppsdlg_; imppsdlg_ = 0;
    delete exppsdlg_; exppsdlg_ = 0;
    delete manpicksetsdlg_; manpicksetsdlg_ = 0;
}


void uiPickPartServer::managePickSets()
{
    delete manpicksetsdlg_;
    manpicksetsdlg_ = new uiPickSetMan( parent(), dgbPickSetTranslator::translKey() );
    manpicksetsdlg_->go();
}


void uiPickPartServer::importSet()
{
    if ( !imppsdlg_ )
    {
	imppsdlg_ = new uiImpExpPickSet( parent(), this, true );
	imppsdlg_->importReady.notify(
		mCB(this,uiPickPartServer,importReadyCB) );
    }

    imppsdlg_->show();
    imppsdlg_->raise();
}


void uiPickPartServer::importReadyCB( CallBacker* cb )
{
    if ( imppsdlg_ && cb==imppsdlg_ )
    {
	picksetid_ = imppsdlg_->getStoredID();
	sendEvent( evDisplayPickSet() );
    }
}


void uiPickPartServer::exportSet()
{
    if ( !exppsdlg_ )
	exppsdlg_ = new uiImpExpPickSet( parent(), this, false );

    exppsdlg_->show();
    exppsdlg_->raise();
}


bool uiPickPartServer::storePickSets( int polyopt, const char* cat )
{
    // Store all sets that have changed
    DBKeySet setids;
    MonitorLock ml( Pick::SetMGR() );
    for ( int idx=0; idx<Pick::SetMGR().size(); idx++ )
    {
	const DBKey setid = Pick::SetMGR().getIDByIndex( idx );
	if ( !Pick::SetMGR().hasCategory(setid,cat) )
	    continue;
	if ( polyopt )
	{
	    const bool ispoly = Pick::SetMGR().isPolygon( setid );
	    if ( (ispoly && polyopt<0) || (!ispoly && polyopt>0) )
		continue;
	}

	if ( Pick::SetMGR().needsSave(setid) )
	    setids += setid;
    }
    ml.unlockNow();
    if ( setids.isEmpty() )
	return true;

    uiStringSet errmsgs;
    for ( int idx=0; idx<setids.size(); idx++ )
    {
	const Pick::SetManager::ObjID setid( setids[idx] );
	if ( !Pick::SetMGR().canSave(setid) )
	    doSaveAs( setid, 0 );
	else
	{
	    SilentTaskRunnerProvider trprov;
	    uiRetVal uirv = Pick::SetMGR().save( setids[idx], trprov );
	    if ( uirv.isError() )
		errmsgs.add( uirv );
	}
    }

    if ( !errmsgs.isEmpty() )
    {
	errmsgs.insert( 0, tr("Could not save (all) changes.") );
	uimsg().errorWithDetails( errmsgs );
	return false;
    }

    return true;
}


bool uiPickPartServer::storePickSet( const Pick::Set& ps )
{
    Pick::SetManager::ObjID setid = Pick::SetMGR().getID( ps );
    if ( setid.isInvalid() )
    {
	uimsg().error( tr("Internal: Request to store an unmanaged PointSet") );
	return false;
    }
    else if ( !Pick::SetMGR().canSave(setid) )
	return doSaveAs( setid, &ps );

    SilentTaskRunnerProvider trprov;
    uiRetVal uirv = Pick::SetMGR().save( setid, trprov );
    if ( uirv.isError() )
	{ uimsg().error( uirv ); return false; }

    return true;
}


#define mObjSelType(ispoly) \
	ispoly ? uiPickSetIOObjSel::PolygonOnly : uiPickSetIOObjSel::NoPolygon

bool uiPickPartServer::storePickSetAs( const Pick::Set& ps )
{
    Pick::SetManager::ObjID setid = Pick::SetMGR().getID( ps );
    if ( setid.isInvalid() )
    {
	uimsg().error(tr("Internal: Request to Save-As an unmanaged PointSet"));
	return false;
    }

    return doSaveAs( setid, &ps );
}



bool uiPickPartServer::doSaveAs( const DBKey& setid, const Pick::Set* ps )
{
    ConstRefMan<Pick::Set> psref;
    if ( !ps )
    {
	psref = Pick::SetMGR().fetch( setid );
	ps = psref;
    }
    IOObjContext ctxt( uiPickSetIOObjSel::getCtxt( mObjSelType(ps->isPolygon()),
						    false, ps->category() ) );
    uiIOObjSelDlg::Setup sdsu( uiStrings::phrSaveAs(ps->name()) );
    uiIOObjSelDlg dlg( parent(), sdsu, ctxt );
    dlg.showAlwaysOnTop();
    if ( !dlg.go() || !dlg.ioObj() )
	return false;

    SilentTaskRunnerProvider trprov;
    uiString errmsg = Pick::SetMGR().saveAs( setid, dlg.ioObj()->key(), trprov);
    if ( !errmsg.isEmpty() )
	{ uimsg().error( errmsg ); return false; }

    return true;
}


void uiPickPartServer::mergePickSets( DBKey& mid )
{
    uiMergePickSets dlg( parent(), mid );
    dlg.go();
}


void uiPickPartServer::fetchHors( bool is2d )
{
    hinfos_.setEmpty();
    sendEvent( is2d ? evGetHorInfo2D() : evGetHorInfo3D() );
}


RefMan<Pick::Set> uiPickPartServer::loadSet( const DBKey& dbky )
{
    DBKeySet psids( dbky );
    return doLoadSets(psids) ? Pick::SetMGR().fetchForEdit(dbky) : 0;
}


bool uiPickPartServer::loadSets( DBKeySet& psids, bool poly,
				 const char* cat, const char* transl )
{
    psids.setEmpty();

    IOObjContext ctxt( uiPickSetIOObjSel::getCtxt( mObjSelType(poly),
						   true, cat, transl ) );
    uiIOObjSelDlg::Setup sdsu; sdsu.multisel( true );
    uiIOObjSelDlg dlg( parent(), sdsu, ctxt );
    dlg.showAlwaysOnTop();
    if ( !dlg.go() )
	return false;

    DBKeySet chosenids;
    dlg.getChosen( chosenids );
    if ( chosenids.isEmpty() )
	return true;

    if ( !doLoadSets(chosenids) && chosenids.isEmpty() )
	return false;

    psids = chosenids;
    return true;
}


bool uiPickPartServer::doLoadSets( DBKeySet& psids )
{
    Pick::SetLoader psloader( psids );
    uiTaskRunner taskrunner( parent() );
    Executor* ldrexec = psloader.getLoader();
    taskrunner.execute( *ldrexec );
    delete ldrexec;
    psids = psloader.available();
    const uiRetVal uirv = psloader.result();
    if ( !uirv.isOK() )
	{ uimsg().error( uirv ); return false; }

    return true;
}


RefMan<Pick::Set> uiPickPartServer::createEmptySet( bool aspolygon )
{
    uiNewPickSetDlg dlg( parent(), aspolygon );
    return dlg.go() ? dlg.getSet() : 0;
}


RefMan<Pick::Set> uiPickPartServer::create3DGenSet()
{
    uiGenPosPicksDlg dlg( parent() );
    return dlg.go() ? dlg.getSet() : 0;
}


RefMan<Pick::Set> uiPickPartServer::createRandom2DSet()
{
    fetchHors( true );
    BufferStringSet hornms;
    for ( int idx=0; idx<hinfos_.size(); idx++ )
	hornms.add( hinfos_[idx]->name );

    BufferStringSet linenames;
    GeomIDSet geomids;
    SurvGeom::list2D( geomids, &linenames );
    if ( linenames.isEmpty() )
    {
	uimsg().warning( tr("No 2D lines are available in this survey") );
	return 0;
    }

    uiGenRandPicks2DDlg dlg( parent(), hornms, linenames );
    mAttachCB( dlg.fillLocs, uiPickPartServer::mkRandLocs2D );
    return dlg.go() ? dlg.getSet() : 0;
}


void uiPickPartServer::mkRandLocs2D( CallBacker* cb )
{
    mDynamicCastGet(uiGenRandPicks2DDlg*,dlg,cb)
    RefMan<Pick::Set> psref = dlg->getSet();
    const RandLocGenPars& rp = dlg->randPars();
    Pick::Set& ps = *psref;

    uiUserShowWait usw( parent(), uiStrings::sGenerating() );

    selectlines_ = rp.linenms_;
    selhorids_.setEmpty();
    coords2d_.erase(); geomids2d_.erase();
    if ( rp.needhor_ )
    {
	selhorids_ += hinfos_[rp.horidx_]->dbkey;
	if ( rp.horidx2_ >= 0 )
	    selhorids_ += hinfos_[rp.horidx2_]->dbkey;

	hor2dzrgs_.erase();
	sendEvent( evGetHorDef2D() );
    }
    else
    {
	for ( int iln=0; iln<selectlines_.size(); iln++ )
	{
	    const auto& geom2d = SurvGeom::get2D(selectlines_.get(iln));
	    if ( geom2d.isEmpty() )
		continue;

	    const Pos::GeomID geomid( geom2d.geomID() );
	    const auto& posns = geom2d.data().positions();
	    for ( int ipos=0; ipos<posns.size(); ipos++ )
	    {
		coords2d_ += posns[ipos].coord_;
		geomids2d_ += geomid;
	    }
	}
    }

    const int nrpos = coords2d_.size();
    if ( nrpos < 1 )
	return;

    const bool needsubsel = nrpos > rp.nr_;
    TypeSet<int> locsleft;
    if ( needsubsel )
	for ( int idx=0; idx<nrpos; idx++ )
	    locsleft += idx;

    for ( int ipt=0; ipt<rp.nr_; ipt++ )
    {
	int posidx = ipt;
	if ( needsubsel )
	{
	    const int llidx = Stats::randGen().getIndex( locsleft.size() );
	    posidx = locsleft[llidx];
	    locsleft.removeSingle( llidx );
	}

	Interval<float> zrg = rp.needhor_ ? hor2dzrgs_[posidx] : rp.zrg_;
	float val = (float) ( zrg.start +
				  Stats::randGen().get() * zrg.width(false) );
	Pick::Location pl( coords2d_[posidx], val );
	pl.setGeomID( geomids2d_[posidx] );
	ps.add( pl );
    }
}


void uiPickPartServer::setMisclassSet( const DataPointSet& dps )
{
    const char* sKeyMisClass = "Misclassified [NN]";
    Pick::SetManager::ObjID id = Pick::SetMGR().getIDByName( sKeyMisClass );
    RefMan<Pick::Set> ps = id.isInvalid() ? 0
			 : Pick::SetMGR().fetchForEdit( id );
    if ( ps )
	ps->setEmpty();
    else
    {
	ps = new Pick::Set( sKeyMisClass );
	ps->setDispColor( Color(240,0,0) );
    }

    for ( int idx=0; idx<dps.size(); idx++ )
    {
	Coord crd = dps.coord( idx );
	ps->add( Pick::Location(crd.x_,crd.y_,dps.z(idx)) );
    }

    SilentTaskRunnerProvider trprov;
    Pick::SetMGR().store( *ps, id, trprov );
}


void uiPickPartServer::fillZValsFrmHor( Pick::Set* ps, int horidx )
{
    ps_ = ps;
    horid_ = hinfos_[horidx]->dbkey;
    sendEvent( evFillPickSet() );
}

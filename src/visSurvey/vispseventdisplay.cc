/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : July 2010
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "vispseventdisplay.h"

#include "binidvalset.h"
#include "prestackevents.h"
#include "survinfo.h"
#include "valseries.h"
#include "velocitycalc.h"
#include "viscoord.h"
#include "visdatagroup.h"
#include "visdrawstyle.h"
#include "vismarker.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visprestackdisplay.h"
#include "vistransform.h"

 
mCreateFactoryEntry( visSurvey::PSEventDisplay );

namespace visSurvey
{

DefineEnumNames( PSEventDisplay, MarkerColor, 0, "Marker Color" )
{ "Single", "Quality", "Velocity", "Velocity fit", 0 };

DefineEnumNames( PSEventDisplay, DisplayMode, 0, "Display Mode" )
{ "None","Zero offset", "Sticks from sections", "Zero offset on sections", 
  "Sticks to gathers", 0 };

PSEventDisplay::PSEventDisplay()
    : VisualObjectImpl( false )
    , displaymode_( ZeroOffsetOnSections )
    , eventman_( 0 )
    , qualityrange_( 0, 1 )
    , displaytransform_( 0 )
    , linestyle_( visBase::DrawStyle::create() )
    , horid_( -1 )
    , offsetscale_( 1 )
    , markercolor_( Single )
    , eventseeds_(visBase::DataObjectGroup::create())
{
    setLockable();
    linestyle_->ref();
    addChild( linestyle_->getInventorNode() );
    eventseeds_->ref();
    addChild( eventseeds_->getInventorNode() );
    ctabmapper_.setup_.type( ColTab::MapperSetup::Auto );
}


PSEventDisplay::~PSEventDisplay()
{
    clearAll();

    setDisplayTransformation( 0 );
    setEventManager( 0 );
    linestyle_->unRef();

    removeChild( eventseeds_->getInventorNode() );
    eventseeds_->unRef();
}


void PSEventDisplay::clearAll()
{
    for ( int idx=parentattached_.size()-1; idx>=0; idx-- )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	removeChild( pao->separator_->getInventorNode() );
    }

    deepErase( parentattached_ );
}


Color PSEventDisplay::getColor() const
{ 
    return getMaterial()->getColor();
}


const char** PSEventDisplay::markerColorNames() const
{
    return PSEventDisplay::MarkerColorNames();
}


const char** PSEventDisplay::displayModeNames() const
{
    return PSEventDisplay::DisplayModeNames();
}


void PSEventDisplay::setEventManager( PreStack::EventManager* em )
{
    if ( eventman_==em )
	return;

    clearAll();

    if ( eventman_ )
    {
	eventman_->change.remove( mCB(this,PSEventDisplay,eventChangeCB) );
	eventman_->forceReload.remove(
		mCB(this,PSEventDisplay,eventForceReloadCB) );
	eventman_->unRef();
    }

    eventman_ = em;

    if ( eventman_ )
    {
	eventman_->ref();
	eventman_->change.notify( mCB(this,PSEventDisplay,eventChangeCB) );
	eventman_->forceReload.notify(
		mCB(this,PSEventDisplay,eventForceReloadCB) );
	getMaterial()->setColor( eventman_->getColor() );
    }
    
    
    updateDisplay();
}


void PSEventDisplay::setHorizonID( int horid )
{
    if ( horid_==horid )
	return;

    horid_ = horid;

    updateDisplay();
}


void PSEventDisplay::setMarkerColor( MarkerColor n, bool update )
{
    if ( markercolor_==n )
	return;

    markercolor_ = n;

    if ( update )
	updateDisplay();
}


PSEventDisplay::MarkerColor PSEventDisplay::getMarkerColor() const
{ return markercolor_; }


void PSEventDisplay::setColTabMapper( const ColTab::MapperSetup& n,
				      bool update )
{
    if ( ctabmapper_.setup_==n )
	return;

    ctabmapper_.setup_ = n;

    if ( update )
	updateDisplay();
}


const ColTab::MapperSetup& PSEventDisplay::getColTabMapper() const
{ return ctabmapper_.setup_; }

const ColTab::MapperSetup* PSEventDisplay::getColTabMapperSetup(
						int visid, int attr ) const
{ return &ctabmapper_.setup_; }

void PSEventDisplay::setColTabSequence( int ch, const ColTab::Sequence& n,
					TaskRunner* tr )
{ setColTabSequence( n, true ); }

void PSEventDisplay::setColTabSequence( const ColTab::Sequence& n, bool update )
{
    if ( ctabsequence_==n )
	return;

    ctabsequence_ = n;

    if ( update )
	updateDisplay();
}


const ColTab::Sequence* PSEventDisplay::getColTabSequence( int ) const
{ return &ctabsequence_; }


void PSEventDisplay::setDisplayMode( DisplayMode dm )
{
    if ( dm==displaymode_ )
	return;

    displaymode_ = dm;

    updateDisplay();
}


PSEventDisplay::DisplayMode PSEventDisplay::getDisplayMode() const
{ return displaymode_; }


void PSEventDisplay::setLineStyle( const LineStyle& ls )
{
    linestyle_->setLineStyle( ls );
}


LineStyle PSEventDisplay::getLineStyle() const
{ 
    return linestyle_->lineStyle();
} 


void PSEventDisplay::setMarkerStyle( const MarkerStyle3D& st, bool update )
{
    if ( markerstyle_==st )
	return;

    markerstyle_ = st;

    if ( update )
    {
	for ( int idx=0; idx<parentattached_.size(); idx++ )
	{
	    ParentAttachedObject* pao = parentattached_[idx];
	    for ( int idy=0; idy<pao->markers_.size(); idy++ )
		pao->markers_[idy]->setMarkerStyle( st );
	}
    }
}


//bool PSEventDisplay::filterBinID( const BinID& bid ) const
//{
    //for ( int idx=0; idx<sectionranges_.size(); idx++ )
    //{
	//if ( sectionranges_[idx].includes( bid ) )
	    //return false;
    //}
//
    //return true;
//}


void PSEventDisplay::updateDisplay()
{
    if ( !tryWriteLock() )
	return;

    if ( displaymode_==ZeroOffset )
    {
	eventChangeCB( 0 );
	updateDisplay( 0 );
	writeUnLock();
	return;
    }

    for ( int idx=0; idx<parentattached_.size(); idx++ )
	updateDisplay( parentattached_[idx] );

    writeUnLock();
}


#define mRemoveParAttached( obj ) \
    removeChild( obj->separator_->getInventorNode() ); \
    delete obj; \
    parentattached_ -= obj


void PSEventDisplay::otherObjectsMoved(
	const ObjectSet<const SurveyObject>& objs, int whichid )
{
    TypeSet<int> newparentsid;
    for ( int idx=objs.size()-1; idx>=0; idx-- )
    {
	int objid = -1;
	    
	mDynamicCastGet(const visSurvey::PreStackDisplay*,gather,objs[idx]);
	if ( gather )
	    objid = gather->id();
	else
	{
	    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,objs[idx]);
	    if ( pdd && pdd->getOrientation() != 
		    visSurvey::PlaneDataDisplay::Zslice )
		objid = pdd->id();
	}

	if ( objid==-1 )
	    continue;

	if ( whichid!=-1 )
	{
	    if ( objid==whichid )
	    {
		newparentsid += objid;
		break;
	    }
	}
	else
	{
	    newparentsid += objid;
	}
    }

    ObjectSet<ParentAttachedObject> toremove;
    if ( whichid==-1 )
       toremove = parentattached_;

    for ( int idx=0; idx<newparentsid.size(); idx++ )
    {
	ParentAttachedObject* pao = 0;
	for ( int idy=0; idy<toremove.size(); idy++ )
	{
	    if ( toremove[idy]->parentid_!=newparentsid[idx] )
		continue;

	    pao = toremove[idy];

	    toremove.removeSingle( idy );
	    break;
	}

	if ( !pao )
	{
	    for ( int idy=0; idy<parentattached_.size(); idy++ )
	    {
		if ( parentattached_[idy]->parentid_ != newparentsid[idx] )
		    continue;
	
		mRemoveParAttached( parentattached_[idy] );
	    }

	    pao = new ParentAttachedObject( newparentsid[idx] );
	    addChild( pao->separator_->getInventorNode() );
	    parentattached_ += pao;
	    pao->separator_->setDisplayTransformation( displaytransform_ );
	}
	
	if ( displaymode_==FullOnSections || displaymode_==FullOnGathers ||
       	     displaymode_==ZeroOffsetOnSections )
	    updateDisplay( pao );
    }

    for ( int idx=0; idx<toremove.size(); idx++ )
    {
	mRemoveParAttached( toremove[idx] );
    }
}


float PSEventDisplay::getMoveoutComp( const TypeSet<float>& offsets,
				    const TypeSet<float>& picks ) const
{
    float variables[] = { picks[0], 0, 3000 };
    PtrMan<MoveoutComputer> moveoutcomp = new RMOComputer;
    const float error = moveoutcomp->findBestVariable(
		    variables, 1, ctabmapper_.setup_.range_,
		    offsets.size(), offsets.arr(), picks.arr() );
    return markercolor_==Velocity ? variables[1] : error;
}


void PSEventDisplay::updateDisplay( ParentAttachedObject* pao )
{
    if ( !eventman_ )
	return;

    if ( displaymode_==None )
    {
	for ( int idx=0; idx<parentattached_.size(); idx++ )
	    clearDisplay( parentattached_[idx] );
	
	return;
    }
    else if ( displaymode_==ZeroOffset )
    {
	for ( int idx=0; idx<parentattached_.size(); idx++ )
    	    clearDisplay( parentattached_[idx] );

	BinIDValueSet locations( 0, false );
	eventman_->getLocations( locations );
	TypeSet<float> vals;
	for ( int lidx=0; lidx<locations.totalSize(); lidx++ )
	{
	    const BinID bid = locations.getBinID( locations.getPos(lidx) );
	    ConstRefMan<PreStack::EventSet> eventset
		    = eventman_->getEvents(bid, true );
	    if ( !eventset )
		return clearAll();

	    const int size = eventset->events_.size();
	    
	    for ( int idx=0; idx<size; idx++ )
	    {
		const PreStack::Event* psevent = eventset->events_[idx];
		if ( !psevent->sz_ )
		    continue;

		visBase::Marker* marker = visBase::Marker::create();
		eventseeds_->addObject( marker );
		marker->setMarkerStyle( markerstyle_ );
		Coord3 pos( bid.inl, bid.crl, psevent->pick_[0] );
		marker->setCenterPos( pos );
	    	marker->setMaterial( getMaterial() );
		TypeSet<float> offsets;
		TypeSet<float> picks;
		for ( int idy=0; idy<psevent->sz_; idy++ )
		{
		    offsets += psevent->offsetazimuth_[idy].offset();
		    picks += psevent->pick_[idy];
		}
		sort_coupled( offsets.arr(), picks.arr(), picks.size() );

		vals += (markercolor_==Quality ? psevent->quality_
					       : getMoveoutComp(offsets,picks));
	    }
	}

	if (  markercolor_ == Single )
	    getMaterial()->setColor( eventman_->getColor() );
	else
	{
	    const ArrayValueSeries<float,float> vs(vals.arr(),0,vals.size());
	    ctabmapper_.setData( &vs, vals.size() );
	    for ( int idx=0; idx<eventseeds_->size(); idx++ )
	    {
		const Color col = ctabsequence_.color(
		    ctabmapper_.position( vals[idx]) );
		RefMan<visBase::Material> mat = visBase::Material::create();
		mDynamicCastGet(
		    visBase::Marker*,marker,eventseeds_->getObject(idx))
		mat->setColor( col );
		marker->setMaterial( mat );
	    }
	}
	return;
    }
    
    clearDisplay( pao );
    CubeSampling cs( false );
    bool fullevent;
    Coord dir;
    if ( displaymode_==FullOnGathers )
    {
	fullevent = true;
	mDynamicCastGet(const visSurvey::PreStackDisplay*,gather,
		visBase::DM().getObject(pao->parentid_) );
	if ( !gather )
	{
	    for ( int idx=pao->markers_.size()-1; idx>=0; idx-- )
	    {
		pao->separator_->removeObject(
			pao->separator_->getFirstIdx( pao->markers_[idx] ) );
		pao->markers_.removeSingle( idx );
	    }
	    if ( pao->lines_ )
		pao->lines_->removeCoordIndexAfter( -1 );
	    
	    return;
	}

	const BinID bid = gather->is3DSeis() ? gather->getPosition()
					     : BinID(-1,-1);
	cs.hrg.setInlRange( Interval<int>(bid.inl, bid.inl) );
	cs.hrg.setCrlRange( Interval<int>(bid.crl, bid.crl) );

	const bool isinl = gather->isOrientationInline();
	dir.x = (isinl ? offsetscale_ : 0) / SI().inlDistance();
	dir.y = (isinl ? 0 : offsetscale_ ) / SI().crlDistance();
    }
    else
    {
	mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,
			visBase::DM().getObject( pao->parentid_ ) );
	if ( !pdd )
	    return;

	cs = pdd->getCubeSampling();
	const bool isinl =
	    pdd->getOrientation()==visSurvey::PlaneDataDisplay::Inline;

	fullevent = displaymode_==FullOnSections;

	dir.x = (isinl ? offsetscale_ : 0) / SI().inlDistance();
	dir.y = (isinl ? 0 : offsetscale_ ) / SI().crlDistance();
    }

    HorSamplingIterator iter( cs.hrg );

    BinID bid = cs.hrg.start;
    int cii = 0;
    int ci = 0;
    int lastmarker = 0;

    PtrMan<MoveoutComputer> moveoutcomp = new RMOComputer;

    ObjectSet<PreStack::EventSet> eventsetstounref = pao->eventsets_;
    pao->eventsets_.erase();
    pao->hrg_ = cs.hrg;

    if ( fullevent && !pao->lines_ )
    {
	pao->lines_ = visBase::IndexedPolyLine::create();
	pao->separator_->addObject( pao->lines_ );
    }

    TypeSet<float> values;
    ObjectSet<visBase::Marker> markers;

    do
    {
	RefMan<PreStack::EventSet> eventset = eventman_ ?
	    eventman_->getEvents( bid, true, false ) : 0;
	if ( !eventset )
	    continue;

	Interval<int> eventrg( 0, eventset->events_.size()-1 );
	if ( horid_!=-1 )
	{
	    const int eventidx = eventset->indexOf( horid_ );
	    if ( eventidx==-1 )
		continue;

	    eventrg.start = eventrg.stop = eventidx;
	}

	pao->eventsets_ += eventset;
	eventset->ref();
	for ( int idx=eventrg.start; idx<=eventrg.stop; idx++ )
	{
	    const PreStack::Event* event = eventset->events_[idx];
	    if ( !event->sz_ )
		continue;

	    TypeSet<float> offsets( event->sz_, 0 );
	    TypeSet<float> picks( event->sz_, 0 );
	    for ( int idy=0; idy<event->sz_; idy++ )
	    {
		offsets[idy] = event->offsetazimuth_[idy].offset();
		picks[idy] = event->pick_[idy];
	    }
	    sort_coupled( offsets.arr(), picks.arr(), picks.size() );

	    float value = mUdf(float);
	    if ( markercolor_==Quality )
		value = event->quality_;
	    else if ( markercolor_!=Single && event->sz_>1 )
		value = getMoveoutComp( offsets, picks );  

	    Interval<int> pickrg( 0, fullevent ? event->sz_-1 : 0 );
	    const bool doline = pickrg.start!=pickrg.stop;
	    for ( int idy=pickrg.start; idy<=pickrg.stop; idy++ )
	    {
		if ( lastmarker>=pao->markers_.size() )
		{
		    visBase::Marker* marker = visBase::Marker::create();
		    pao->separator_->addObject( marker );
		    marker->setMarkerStyle( markerstyle_ );
		    pao->markers_ += marker;
		}

		Coord3 pos( bid.inl, bid.crl,  picks[idy] );
		if ( fullevent )
		{
		    const Coord offset = dir*offsets[idy];
		    pos.x += offset.x;
		    pos.y += offset.y;
		}

		pao->markers_[lastmarker]->setCenterPos( pos );
		if ( markercolor_==Single )
		    pao->markers_[lastmarker]->setMaterial( 0 );
		else
		{
		    if ( !pao->markers_[lastmarker]->getMaterial() )
			pao->markers_[lastmarker]->setMaterial( 
			       visBase::Material::create() );

		    values += value;
		    markers += pao->markers_[lastmarker];
		}

		lastmarker++;
		if ( doline )
		{
		    pao->lines_->getCoordinates()->setPos( ci, pos );
		    pao->lines_->setCoordIndex( cii++, ci++ );
		}
	    }

	    if ( doline && cii && pao->lines_->getCoordIndex( cii-1 )!=-1 ) 
		pao->lines_->setCoordIndex( cii++, -1 );
	}

    } while ( iter.next( bid ) );

    if ( ctabmapper_.setup_.type_!=ColTab::MapperSetup::Fixed )
    {
	const ArrayValueSeries<float,float> vs(values.arr(),0,values.size());
	ctabmapper_.setData( &vs, values.size() );
    }

    for ( int idx=0; idx<markers.size(); idx++ )
    {
	markers[idx]->getMaterial()->setColor( 
		ctabsequence_.color( ctabmapper_.position(values[idx])) );
    }

    for ( int idx=pao->markers_.size()-1; idx>=lastmarker; idx-- )
    {
	pao->separator_->removeObject(
		pao->separator_->getFirstIdx( pao->markers_[idx] ) );
	pao->markers_.removeSingle( idx );
    }

    if ( pao->lines_ )
	pao->lines_->removeCoordIndexAfter( cii-1 );

    deepUnRef( eventsetstounref );
    
}


void PSEventDisplay::clearDisplay( ParentAttachedObject* pao )
{
    if ( eventseeds_ )
	eventseeds_->removeAll();

    if ( !pao )
	return;
    
    if ( pao->separator_ )
    	pao->separator_->removeAll();

    pao->markers_.erase();
    pao->lines_ = 0;
}


void PSEventDisplay::setDisplayTransformation(const mVisTrans* nt)
{ 
    for ( int idx=0; idx<parentattached_.size(); idx++ )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	pao->separator_->setDisplayTransformation( nt );
    }

    if ( displaytransform_ )
	displaytransform_->unRef();

    displaytransform_ = nt;

    if ( displaytransform_ )
	displaytransform_->ref();
}


const mVisTrans* PSEventDisplay::getDisplayTransformation() const
{ return displaytransform_; }


void PSEventDisplay::eventChangeCB(CallBacker*)
{
    const BinID bid = eventman_ ? eventman_->changeBid() : BinID(-1,-1);
    if ( bid.inl<0 || bid.crl<0 )
    {
	if ( eventman_ )
 	    getMaterial()->setColor( eventman_->getColor() );
    }

    if ( !parentattached_.size() )
    {
	retriveParents();
	return;
    }

    for ( int idx=parentattached_.size()-1; idx>=0; idx-- )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	if ( !pao->hrg_.includes(bid) )
	    continue;

	updateDisplay(pao);
    }
}


void PSEventDisplay::eventForceReloadCB(CallBacker*)
{
    for ( int idx=parentattached_.size()-1; idx>=0; idx-- )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	deepUnRef( pao->eventsets_ );
	HorSamplingIterator iter( pao->hrg_ );

	BinID bid = pao->hrg_.start;
	do
	{
	    eventman_->addReloadPosition( bid );
	}
	while ( iter.next( bid ) );
    }

    deepErase( parentattached_ );

}


bool PSEventDisplay::hasParents() const
{
    return !parentattached_.isEmpty();
}


void PSEventDisplay::retriveParents()
{
    if ( !scene_ )
	return;
    
    TypeSet<int> visids;
    for ( int idx=0; idx<scene_->size(); idx++ )
	visids += scene_->getObject( idx )->id();

    for ( int idx=0; idx<visids.size(); idx++ )
    {
	mDynamicCastGet( visSurvey::SurveyObject*, so,
		scene_->getObject(visids[idx])  );
	if ( !so ) continue;
	
	mDynamicCastGet(const visSurvey::PreStackDisplay*,gather,so);
	mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,so);
	if ( gather || (pdd && pdd->isOn() &&
	     pdd->getOrientation()!=visSurvey::PlaneDataDisplay::Zslice) )
	{
	    ParentAttachedObject* pao = new ParentAttachedObject( visids[idx] );
	    addChild( pao->separator_->getInventorNode() );
	    parentattached_ += pao;
	    pao->separator_->setDisplayTransformation( displaytransform_ );
	    updateDisplay( pao );
	}
    }
}


PSEventDisplay::ParentAttachedObject::ParentAttachedObject( int parent )
    : parentid_( parent )
    , separator_( visBase::DataObjectGroup::create() )
    , lines_( 0 )
{
    separator_->ref();
    separator_->setSeparate();
}


PSEventDisplay::ParentAttachedObject::~ParentAttachedObject()
{
    separator_->unRef();
    deepUnRef( eventsets_ );
}

} // namespace

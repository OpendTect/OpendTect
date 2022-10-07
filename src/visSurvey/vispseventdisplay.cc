/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vispseventdisplay.h"

#include "binidvalset.h"
#include "color.h"
#include "prestackevents.h"
#include "survinfo.h"

#include "velocitycalc.h"
#include "viscoord.h"
#include "visdatagroup.h"
#include "visdrawstyle.h"
#include "vismarkerset.h"
#include "vismaterial.h"
#include "visplanedatadisplay.h"
#include "vispolyline.h"
#include "visprestackdisplay.h"
#include "vistransform.h"


namespace visSurvey
{

mDefineEnumUtils( PSEventDisplay, MarkerColor, "Marker Color" )
{ "Single", "Quality", "Velocity", "Velocity fit", 0 };

mDefineEnumUtils( PSEventDisplay, DisplayMode, "Display Mode" )
{ "Zero offset", "Sticks from sections", "Zero offset on sections",
  "Sticks to gathers", 0 };

PSEventDisplay::PSEventDisplay()
    : VisualObjectImpl(false)
    , displaymode_(ZeroOffsetOnSections)
    , eventman_(nullptr)
    , qualityrange_(0,1)
    , displaytransform_(nullptr)
    , linestyle_(new visBase::DrawStyle)
    , horid_(-1)
    , offsetscale_(1)
    , markercolor_(Single)
    , eventmarkerset_(visBase::MarkerSet::create())
{
    setLockable();
    linestyle_->ref();
    addNodeState( linestyle_ );
    eventmarkerset_->ref();
    eventmarkerset_->setMarkerStyle( markerstyle_ );
    eventmarkerset_->setMaterial( getMaterial() );

    addChild( eventmarkerset_->osgNode() );
    ctabmapper_.setup_.type( ColTab::MapperSetup::Auto );

}


PSEventDisplay::~PSEventDisplay()
{
    clearAll();

    setDisplayTransformation( 0 );
    setEventManager( 0 );
    linestyle_->unRef();

    removeChild( eventmarkerset_->osgNode() );
    eventmarkerset_->unRef();
}


void PSEventDisplay::clearAll()
{
    for ( int idx=parentattached_.size()-1; idx>=0; idx-- )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	removeChild( pao->objectgroup_->osgNode() );
    }

    deepErase( parentattached_ );
}


OD::Color PSEventDisplay::getColor() const
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
	updateDisplay();
    }
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
						int attr, int version ) const
{ return &ctabmapper_.setup_; }


void PSEventDisplay::setColTabSequence( int ch, const ColTab::Sequence& n,
					TaskRunner* )
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


void PSEventDisplay::setLineStyle( const OD::LineStyle& ls )
{
    linestyle_->setLineStyle( ls );
}


OD::LineStyle PSEventDisplay::getLineStyle() const
{
    return linestyle_->lineStyle();
}


void PSEventDisplay::setMarkerStyle( const MarkerStyle3D& st )
{
    if ( markerstyle_==st )
	return;

    markerstyle_ = st;

    for ( int idx=0; idx<parentattached_.size(); idx++ )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	    pao->markerset_->setMarkerStyle( st );
    }
}


const MarkerStyle3D* PSEventDisplay::markerStyle() const
{
    return &markerstyle_;
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
    Threads::Locker locker( lock_ );

    if ( displaymode_==ZeroOffset )
    {
	eventChangeCB( 0 );
	updateDisplay( 0 );
	return;
    }

    for ( int idx=0; idx<parentattached_.size(); idx++ )
	updateDisplay( parentattached_[idx] );
}


#define mRemoveParAttached( obj ) \
    removeChild( obj->objectgroup_->osgNode() ); \
    delete obj; \
    parentattached_ -= obj


void PSEventDisplay::otherObjectsMoved(
	const ObjectSet<const SurveyObject>& objs, VisID whichid )
{
    TypeSet<VisID> newparentsid;
    for ( int idx=objs.size()-1; idx>=0; idx-- )
    {
	VisID objid;
	mDynamicCastGet(const visSurvey::PreStackDisplay*,gather,objs[idx]);
	if ( gather )
	    objid = gather->id();
	else
	{
	    mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,objs[idx]);
	    if ( pdd && pdd->getOrientation() != OD::ZSlice )
		objid = pdd->id();
	}

	if ( !objid.isValid() )
	    continue;

	if ( whichid.isValid() )
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
    if ( !whichid.isValid() )
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
	    addChild( pao->objectgroup_->osgNode() );
	    parentattached_ += pao;
	    pao->objectgroup_->setDisplayTransformation( displaytransform_ );
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
		    variables, 1, qualityrange_,
		    offsets.size(), offsets.arr(), picks.arr() );
    return markercolor_==Velocity ? variables[1] : error;
}


void PSEventDisplay::updateDisplay( ParentAttachedObject* pao )
{
    if ( !eventman_ )
	return;

    BinIDValueSet locations( 0, false );
    eventman_->getLocations( locations );

    TrcKeySampling evntrg( false );
    evntrg.survid_ = OD::Geom3D;
    evntrg.setInlRange( locations.inlRange() );
    evntrg.setCrlRange( locations.crlRange() );


    if ( displaymode_==ZeroOffset )
    {
	for ( int idx=0; idx<parentattached_.size(); idx++ )
	    clearDisplay( parentattached_[idx] );


	eventmarkerset_->clearMarkers();
	TypeSet<float> vals;
	for ( int lidx=0; lidx<locations.totalSize(); lidx++ )
	{
	    const BinID bid = locations.getBinID( locations.getPos(lidx) );
	    const PreStack::EventSet* eventset
		    = eventman_->getEvents(bid, true );
	    if ( !eventset )
		continue;

	    const int size = eventset->events_.size();
	    for ( int idx=0; idx<size; idx++ )
	    {
		const PreStack::Event* psevent = eventset->events_[idx];
		if ( !psevent->sz_ )
		    continue;
		Coord3 pos( bid.inl(), bid.crl(), psevent->pick_[0] );
		eventmarkerset_->addPos( pos, false );

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
	{
	    eventmarkerset_->setMarkersSingleColor( eventman_->getColor() );
	    getMaterial()->setColor( eventman_->getColor() );
	}
	else
	{
	    ctabmapper_.setData( vals.arr(), vals.size() );
	    for (int idx=0;idx<eventmarkerset_->getCoordinates()->size();idx++)
	    {
		const OD::Color col = ctabsequence_.color(
		    ctabmapper_.position( vals[idx]) );
		 eventmarkerset_->getMaterial()->setColor(col,idx) ;
	    }
	}

	eventmarkerset_->turnAllMarkersOn( true );
	eventmarkerset_->forceRedraw( true );
	return;
    }

    clearDisplay( pao );
    TrcKeyZSampling cs( false );
    bool fullevent;
    Coord dir;
    if ( displaymode_==FullOnGathers )
    {
	fullevent = true;
	mDynamicCastGet(const visSurvey::PreStackDisplay*,gather,
		visBase::DM().getObject(pao->parentid_) );
	if ( !gather )
	{
	    pao->objectgroup_->removeObject(
		pao->objectgroup_->getFirstIdx( pao->markerset_ ) );

	    pao->markerset_->clearMarkers();

	    if ( pao->lines_ )
	    {
		pao->lines_->removeAllPrimitiveSets();
		pao->lines_->getCoordinates()->setEmpty();
	    }

	    return;
	}

	const BinID bid = gather->is3DSeis() ? gather->getPosition()
					     : BinID(-1,-1);
	cs.hsamp_.setInlRange( Interval<int>(bid.inl(), bid.inl()) );
	cs.hsamp_.setCrlRange( Interval<int>(bid.crl(), bid.crl()) );

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

	cs = pdd->getTrcKeyZSampling();
	const bool isinl =
	    pdd->getOrientation()==OD::InlineSlice;

	fullevent = displaymode_==FullOnSections;

	dir.x = (isinl ? offsetscale_ : 0) / SI().inlDistance();
	dir.y = (isinl ? 0 : offsetscale_ ) / SI().crlDistance();
    }

    if ( !cs.hsamp_.includes(evntrg) )
	return;

    pao->eventsets_.erase();
    pao->tks_ = cs.hsamp_;
    pao->objectgroup_->addObject( pao->markerset_ );
    pao->markerset_->setMarkerStyle( markerstyle_ );

    if ( fullevent && !pao->lines_ )
    {
	pao->lines_ = visBase::PolyLine3D::create();
	pao->lines_->ref();
	pao->lines_->setLineStyle( linestyle_->lineStyle() );
	pao->objectgroup_->addObject( pao->lines_ );
	pao->lines_->setDisplayTransformation( displaytransform_ );

    }

    int cii = 0;
    int lastmarker = 0;
    TypeSet<float> values;
    TrcKeySamplingIterator iter( cs.hsamp_ );
    BinID bid;
    do
    {
	PreStack::EventSet* eventset = eventman_ ?
	    eventman_->getEvents( bid, true, false) : 0;
	if ( !eventset )
	    continue;

	eventset->ref();
	Interval<int> eventrg( 0, eventset->events_.size()-1 );
	if ( horid_!=-1 )
	{
	    const int eventidx = eventset->indexOf( horid_ );
	    if ( eventidx==-1 )
		continue;

	    eventrg.start = eventrg.stop = eventidx;
	}

	pao->eventsets_ += eventset;


	if ( markercolor_==Single )
	    pao->markerset_->setMarkersSingleColor( eventman_->getColor() );
	else
	{
	    if ( !pao->markerset_->getMaterial() )
		pao->markerset_->setMaterial(
		new visBase::Material );
	}

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
		Coord3 pos( bid.inl(), bid.crl(),  picks[idy] );
		if ( fullevent )
		{
		    const Coord offset = dir*offsets[idy];
		    pos.x += offset.x;
		    pos.y += offset.y;
		}

		pao->markerset_->addPos( pos, false );

		if ( markercolor_ != Single )
		    values += value;

		lastmarker++;
		if ( doline )
		    pao->lines_->getCoordinates()->addPos( pos );
	    }

	    pao->markerset_->turnAllMarkersOn( true );
	    pao->markerset_->forceRedraw( true );

	    const int size = pao->lines_ ? pao->lines_->getCoordinates()->size()
					 : 0;
	    if ( doline && size > 0 )
	    {
		Geometry::RangePrimitiveSet* ps =
		    Geometry::RangePrimitiveSet::create();
		Interval<int> range( cii,size-1 );
		ps->setRange( range );
		ps->ref();
		pao->lines_->addPrimitiveSet( ps );
		cii = pao->lines_->getCoordinates()->size();
	    }
	}

    } while ( iter.next(bid) );

    if ( markercolor_ != Single )
    {
	if ( ctabmapper_.setup_.type_ != ColTab::MapperSetup::Fixed )
	    ctabmapper_.setData( values.arr(), values.size() );

	for ( int idx=0; idx<lastmarker; idx++ )
	{
	    OD::Color color = ctabsequence_.color(
			      ctabmapper_.position(values[idx]) );
	    pao->markerset_->getMaterial()->setColor(color, idx );
	}

    }

    for ( int idx=pao->markerset_->getCoordinates()->size()-1;
	idx>=lastmarker; idx-- )
	pao->markerset_->removeMarker( idx );
}


void PSEventDisplay::clearDisplay( ParentAttachedObject* pao )
{

    if( eventmarkerset_ )
	eventmarkerset_->clearMarkers();

    if ( !pao )
	return;

    pao->markerset_->clearMarkers();
    pao->lines_ = 0;

    if ( pao->objectgroup_ )
	pao->objectgroup_->removeAll();

}


void PSEventDisplay::setDisplayTransformation(const mVisTrans* nt)
{
    for ( int idx=0; idx<parentattached_.size(); idx++ )
    {
	ParentAttachedObject* pao = parentattached_[idx];
	pao->objectgroup_->setDisplayTransformation( nt );
    }

    eventmarkerset_->setDisplayTransformation( nt );

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
    if ( bid.inl()<0 || bid.crl()<0 )
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
	if ( !pao->tks_.includes(bid) )
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
	TrcKeySamplingIterator iter( pao->tks_ );

	BinID bid;
	while( iter.next( bid ))
	{
	    eventman_->addReloadPosition( bid );
	}
    }

    deepErase( parentattached_ );

}


bool PSEventDisplay::hasParents() const
{
    return !parentattached_.isEmpty();
}


bool PSEventDisplay::supportsDisplay() const
{
    if ( !hasParents() || !eventman_ )
	return false;

    BinIDValueSet locations( 0, false );
    eventman_->getLocations( locations );
    TrcKeySampling eventrg( false );
    eventrg.setInlRange( locations.inlRange() );
    eventrg.setCrlRange( locations.crlRange() );
    bool issuported = false;
    for ( int idx=0; idx<parentattached_.size(); idx++ )
    {
	const ParentAttachedObject* pao = parentattached_[idx];
	mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,
			visBase::DM().getObject(pao->parentid_))
	if ( !pdd )
	    continue;

	const TrcKeyZSampling pddrg = pdd->getTrcKeyZSampling();
	issuported = pddrg.hsamp_.includes(eventrg);
    }

    return issuported;
}


void PSEventDisplay::retriveParents()
{
    if ( !scene_ )
	return;

#define mCreatPao \
    ParentAttachedObject* pao = \
	new ParentAttachedObject( parentid ); \
    addChild( pao->objectgroup_->osgNode() ); \
    parentattached_ += pao; \
    pao->objectgroup_->setDisplayTransformation( displaytransform_); \
    updateDisplay( pao );

    for ( int idx=0; idx<scene_->size(); idx++ )
    {
	mDynamicCastGet( visSurvey::SurveyObject*, so, scene_->getObject(idx));
	if ( !so ) continue;

	mDynamicCastGet(const visSurvey::PreStackDisplay*,gather,so);
	mDynamicCastGet(const visSurvey::PlaneDataDisplay*,pdd,so);
	if ( gather || (pdd && pdd->isOn() &&
	     pdd->getOrientation()!=OD::ZSlice) )
	{
	    const VisID parentid = scene_->getObject(idx)->id();
	    if ( parentattached_.isEmpty() )
	    {
		mCreatPao
	    }
	    else
	    {
		for ( int idy=0; idy<parentattached_.size(); idy++ )
		{
		    if ( parentattached_[idy]->parentid_ == parentid )
			continue;

		    mCreatPao
		}
	    }
	}
    }
}


PSEventDisplay::ParentAttachedObject::ParentAttachedObject( VisID parent )
    : parentid_( parent )
    , objectgroup_( visBase::DataObjectGroup::create() )
    , lines_( 0 )
    , markerset_( visBase::MarkerSet::create() )
{
    objectgroup_->ref();
    markerset_->ref();
    objectgroup_->addObject( markerset_ );
}


PSEventDisplay::ParentAttachedObject::~ParentAttachedObject()
{
    unRefPtr( objectgroup_ );
    unRefPtr( markerset_ );
    unRefPtr( lines_ );
    deepUnRef( eventsets_ );
}


void PSEventDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( eventmarkerset_ )
        eventmarkerset_->setPixelDensity( dpi );

    for ( int idx=0; idx<parentattached_.size(); idx++ )
	parentattached_[idx]->objectgroup_->setPixelDensity( dpi );

}

} // namespace visSurvey

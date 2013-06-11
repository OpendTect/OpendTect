/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#include "emhorizonpainter.h"

#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emobject.h"
#include "flatview.h"


namespace EM
{


HorizonPainter::HorizonPainter( FlatView::Viewer& fv )
    : viewer_(fv)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , horrg_(0,1)
    , loadinghorcount_(0)
    , markerstyle_( MarkerStyle2D::Square, 4, Color::White() )
    , is2d_(false) 
    , horidtoberepainted_(-1)
    , isupdating_(false)
    , linenm_(0)
    , horizonAdded(this)
    , horizonRemoved(this)
{
    hormarkerlines_.allowNull();
    horsmarkerseeds_.allowNull();
    cs_.setEmpty();
    EM::EMM().addRemove.notify( mCB(this,HorizonPainter,nrHorChangeCB) );
}


HorizonPainter::~HorizonPainter()
{
    while ( hormarkerlines_.size() )
    {
	EM::EMM().getObject( horizoninfos_[0]->id_ )->change.remove(
		mCB(this,HorizonPainter,horChangeCB) );
	removeHorizon( 0 );
    }

    EM::EMM().addRemove.remove( mCB(this,HorizonPainter,nrHorChangeCB) );
    deepErase( horizoninfos_ );
}


void HorizonPainter::addHorizon( const MultiID& mid )
{
    const EM::ObjectID oid = EM::EMM().getObjectID( mid );

    addHorizon( oid );
}


void HorizonPainter::addHorizon( const EM::ObjectID& oid )
{
    for ( int idx=0; idx<horizoninfos_.size(); idx++ )
	if ( horizoninfos_[idx]->id_ == oid )
	    return;

    HorizonInfo* horinfo = new HorizonInfo;
    horinfo->id_ = oid;
    horinfo->lineenabled_ = true;
    horinfo->seedenabled_ = true;
    horinfo->name_ = EM::EMM().getObject( oid )->name();

    horizoninfos_ += horinfo;

    if ( !addPolyLine(oid) )
    {
	delete horizoninfos_[horizoninfos_.size() - 1];
	horizoninfos_.removeSingle( horizoninfos_.size() - 1 );
	return;
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );

    EMObjPainterCallbackData cbdata;
    cbdata.objid_ = horinfo->id_;
    cbdata.name_ = horinfo->name_;
    cbdata.enabled_ = true;
    horizonAdded.trigger( cbdata );
}


HorizonPainter::HorizonInfo* HorizonPainter::getHorizonInfo( 
							const EM::ObjectID& oid)
{
    for ( int idx=0; idx<horizoninfos_.size(); idx++ )
	if ( horizoninfos_[idx]->id_ == oid )
	    return horizoninfos_[idx];

    return 0;
}


void HorizonPainter::setHorizonIDs( const ObjectSet<MultiID>* mids )
{
    if ( !mids ) return;

    deepErase( horizoninfos_ );
    for ( int idx=0; idx<mids->size(); idx++ )
    {
	HorizonInfo* horinfo = new HorizonInfo;
	horinfo->id_ = EM::EMM().getObjectID( *(*mids)[idx] );
	horinfo->lineenabled_ = true;
	horinfo->seedenabled_ = true;
	horinfo->name_ = EM::EMM().getObject( horinfo->id_ )->name();

	horizoninfos_ += horinfo;
    }

    updateDisplay();
}


void HorizonPainter::setCubeSampling( const CubeSampling& cs, bool update )
{
    cs_ = cs;
    if ( update )
	updateDisplay();
}


bool HorizonPainter::addPolyLine( const EM::ObjectID& oid )
{
    EM::EMObject* emobj = EM::EMM().getObject( oid );
    if ( !emobj ) return false;

    mDynamicCastGet(EM::Horizon*,hor,emobj)
	if ( !hor ) return false;

    mDynamicCastGet(EM::Horizon2D*,hor2d,emobj)
	if ( is2d_ && !hor2d )
	    return false;

    if ( !loadinghorcount_ && !isupdating_ )
	hor->change.notify( mCB(this,HorizonPainter,horChangeCB) );

    int horidx = -1;
    for ( horidx = 0; horidx<horizoninfos_.size(); horidx++ )
	if ( horizoninfos_[horidx]->id_ == oid )
	    break;

     FlatView::AuxData* seedsauxdata = viewer_.createAuxData( 0 );
     seedsauxdata->enabled_ = horizoninfos_[horidx]->seedenabled_;

    if ( isupdating_ )
	horsmarkerseeds_.replace( horidx, seedsauxdata );
    else
	horsmarkerseeds_ += seedsauxdata;

    seedsauxdata->poly_.erase();
    seedsauxdata->markerstyles_ += markerstyle_;
    viewer_.addAuxData( seedsauxdata );

    ObjectSet<SectionMarkerLine>* sectionmarkerlines =
					new ObjectSet<SectionMarkerLine>;

    if ( isupdating_ )
	hormarkerlines_.replace( horidx, sectionmarkerlines );
    else
	hormarkerlines_ += sectionmarkerlines;

    for ( int ids=0; ids<hor->nrSections(); ids++ )
    {
	SectionMarkerLine* markerlines = new SectionMarkerLine;
	(*sectionmarkerlines) += markerlines;
	
	bool newmarker = true;
	bool coorddefined = true;
	int markerlinecount = 0;
	FlatView::AuxData* auxdata = 0;

	EM::SectionID sid = hor->sectionID( ids );
	HorSamplingIterator iter( cs_.hrg );
	BinID bid;
	while( iter.next(bid) )
	{
	    int inlfromcs = bid.inl;
	    if ( is2d_ )
	    {
		if ( hor2d->geometry().lineIndex( linenm_ ) < 0 )
		    continue;
		else
		{
		    bid.inl = hor2d->geometry().lineIndex( linenm_ );
		}
	    }

	    const Coord3 crd = hor->getPos( sid, bid.toInt64() );
	    EM::PosID posid( hor->id(), sid, bid.toInt64() );

	    if ( !crd.isDefined() )
	    {
		coorddefined = false;
		bid.inl = inlfromcs;
		continue;
	    }
	    else if ( !coorddefined )
	    {
		coorddefined = true;
		newmarker = true;
	    }

	    bool isaseed = hor->isPosAttrib( posid,EM::EMObject::sSeedNode());

	    if ( newmarker )
	    {
		auxdata = viewer_.createAuxData( 0 );
		(*markerlines) += auxdata;
		viewer_.addAuxData( auxdata );
		auxdata->poly_.erase();
		auxdata->linestyle_ = markerlinestyle_;
		Color prefcol = hor->preferredColor();
		prefcol.setTransparency( 0 );
		auxdata->linestyle_.color_ = prefcol;
		auxdata->fillcolor_ = prefcol;
		auxdata->name_ = hor->name();
		newmarker = false;
		auxdata->enabled_ = horizoninfos_[horidx]->lineenabled_;
		markerlinecount++;
	    }
	    if ( cs_.nrInl() == 1 )
	    {
		if ( is2d_ )
		{
		    int idx = trcnos_.indexOf(bid.crl);
		    auxdata->poly_ += 
			FlatView::Point( distances_[idx], crd.z );
		    if ( isaseed )
			seedsauxdata->poly_ +=
			    FlatView::Point( distances_[idx], crd.z );
		}
		else
		{
		    auxdata->poly_ += FlatView::Point( bid.crl, crd.z );
		    if ( isaseed )
			seedsauxdata->poly_ += 
			    	FlatView::Point( bid.crl, crd.z );
		}
	    }
	    else if ( cs_.nrCrl() == 1 )
	    {
		auxdata->poly_ += FlatView::Point( bid.inl, crd.z );
		if ( isaseed )
		    seedsauxdata->poly_ +=
			FlatView::Point( bid.inl, crd.z );
	    }

	    if ( is2d_ )
		bid.inl = inlfromcs;
	}
    }

    return true;
}


void HorizonPainter::changePolyLineColor( const EM::ObjectID& oid )
{
    mDynamicCastGet(EM::Horizon*,hor,EM::EMM().getObject( oid ));

    int horpos = -1;
    
    for ( int idx = 0; idx<horizoninfos_.size(); idx++ )
	    if ( horizoninfos_[idx]->id_ == oid )
	    { horpos = idx; break; }

    if ( (horpos==-1) || (hormarkerlines_.size() <= 0) || 
	  !hormarkerlines_[horpos] )
	return;

    ObjectSet<SectionMarkerLine>* sectionmarkerlines = hormarkerlines_[horpos];

    for ( int ids=0; ids<hor->nrSections(); ids++ )
    {
	SectionMarkerLine* markerlines = (*sectionmarkerlines)[ids];
	for ( int markidx=0; markidx<markerlines->size(); markidx++ )
	{
	    FlatView::AuxData* auxdata = (*markerlines)[markidx];
	    auxdata->linestyle_.color_ = hor->preferredColor();
	}
    }
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter::enableHorizonLine( const EM::ObjectID& oid, bool enabled )
{
    mDynamicCastGet(EM::Horizon*,hor,EM::EMM().getObject( oid ));

    int horpos = -1;
    for ( int idx = 0; idx<horizoninfos_.size(); idx++ )
	    if ( horizoninfos_[idx]->id_ == oid )
	    { horpos = idx; break; }

    if ( (horpos==-1) || (hormarkerlines_.size() <= 0) || 
	  !hormarkerlines_[horpos] )
	return;

    ObjectSet<SectionMarkerLine>* sectionmarkerlines = hormarkerlines_[horpos];

    for ( int ids=0; ids<hor->nrSections(); ids++ )
    {
	SectionMarkerLine* markerlines = (*sectionmarkerlines)[ids];
	for ( int markidx=0; markidx<markerlines->size(); markidx++ )
	{
	    FlatView::AuxData* auxdata = (*markerlines)[markidx];
	    auxdata->enabled_ = enabled;
	}
    }
    horizoninfos_[horpos]->lineenabled_ = enabled;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter::enableHorizonSeed( const EM::ObjectID& oid, bool enabled )
{
    int horpos = -1;
    for ( int idx = 0; idx<horizoninfos_.size(); idx++ )
	    if ( horizoninfos_[idx]->id_ == oid )
	    { horpos = idx; break; }

    if ( (horpos==-1) || (horsmarkerseeds_.size() <= 0) ||
	  !horsmarkerseeds_[horpos] )
	  return;

    horsmarkerseeds_[horpos]->enabled_ = enabled;
    horizoninfos_[horpos]->seedenabled_ = enabled;

    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter::changePolyLinePosition( const EM::ObjectID& oid,
					     const EM::PosID& pid )
{
    mDynamicCastGet(EM::Horizon*,hor,EM::EMM().getObject( oid ));

    int horpos = -1;
    
    for ( int idx = 0; idx<horizoninfos_.size(); idx++ )
	    if ( horizoninfos_[idx]->id_ == oid )
	    { horpos = idx; break; }

    if ( (horpos==-1) || (hormarkerlines_.size() <= 0) 
	 || !hormarkerlines_[horpos] )
	return;
    ObjectSet<SectionMarkerLine>* sectionmarkerlines = hormarkerlines_[horpos];

    BinID binid = BinID::fromInt64( pid.subID() );

    for ( int ids=0; ids<hor->nrSections(); ids++ )
    {
	SectionMarkerLine* markerlines = (*sectionmarkerlines)[ids];
	for ( int markidx=0; markidx<markerlines->size(); markidx++ )
	{
	    Coord3 crd = hor->getPos( hor->sectionID(ids), pid.subID() );
	    FlatView::AuxData* auxdata = (*markerlines)[markidx];
	    for ( int posidx = 0; posidx < auxdata->poly_.size(); posidx ++ )
		{
		    if ( cs_.nrInl() == 1 )
			{
			    if ( is2d_ )
			    {
				int idx = trcnos_.indexOf(binid.crl);
				if ( distances_[idx] == 
				     auxdata->poly_[posidx].x )
				    {
					auxdata->poly_[posidx].y = crd.z;
					return;
				    }
			    }
			    else if ( binid.crl == auxdata->poly_[posidx].x )
				{
				    auxdata->poly_[posidx].y = crd.z;
				    return;
				}
			    }
		    else if ( cs_.nrCrl() == 1 )
		    {
			if ( binid.inl == auxdata->poly_[posidx].x )
			    {
				auxdata->poly_[posidx].y = crd.z;
				return;
			    }
		    }
		}
	    
	    if ( crd.isDefined() )
	    {
		if ( cs_.nrInl() == 1 )
		    auxdata->poly_ += FlatView::Point( binid.crl, crd.z );
		else if ( cs_.nrCrl() == 1 )
		    auxdata->poly_ += FlatView::Point( binid.inl, crd.z );
	    }
	}
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void HorizonPainter::updateDisplay()
{
    isupdating_ = true;
    for ( int idx=0; idx<hormarkerlines_.size(); idx++ )
	removePolyLine( idx );

    for ( int idx=0; idx<horizoninfos_.size(); idx++ )
    {
	if ( !addPolyLine(horizoninfos_[idx]->id_) )
		continue;
    }

    viewer_.handleChange( FlatView::Viewer::Auxdata );
    isupdating_ = false;
}


void HorizonPainter::setMarkerLineStyle( const LineStyle& ls )
{
    if ( markerlinestyle_==ls )
	return;

    markerlinestyle_ = ls;
    updateDisplay();
}


void HorizonPainter::repaintHorizon( const EM::ObjectID& oid )
{
    int horidx = -1;

    for ( int idx = 0; idx<horizoninfos_.size(); idx++ )
	if ( horizoninfos_[idx]->id_ == oid )
	    { horidx = idx; break; }

    if ( horidx>=0 )
	 removeHorizon( horidx );

    addHorizon(oid);
}


void HorizonPainter::removeHorizon( const MultiID& mid )
{
    EM::ObjectID objid = EM::EMM().getObjectID( mid );
    int horidx = -1;

    for ( int idx = 0; idx<horizoninfos_.size(); idx++ )
	if ( horizoninfos_[idx]->id_ == objid )
	{ horidx = idx; break; }
    
    if ( horidx>=0 )
	removeHorizon( horidx );
}


void HorizonPainter::removePolyLine( int idx )
{
    ObjectSet<SectionMarkerLine>* sectionmarkerlines =
							hormarkerlines_[idx];
    for ( int markidx=sectionmarkerlines->size()-1; markidx>=0; markidx-- )
    {
	SectionMarkerLine* markerlines = (*sectionmarkerlines)[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	    viewer_.removeAuxData( (*markerlines)[idy] );

    }
    deepErase( *hormarkerlines_[idx] );
    delete hormarkerlines_[idx];
    if ( isupdating_ )
	hormarkerlines_.replace( idx, 0 );
    else
	hormarkerlines_.removeSingle( idx );

    delete viewer_.removeAuxData( horsmarkerseeds_[idx] );
    if ( isupdating_ )
	horsmarkerseeds_.replace( idx, 0 );
    else
	horsmarkerseeds_.removeSingle( idx );
}



void HorizonPainter::removeHorizon( int idx )
{
    removePolyLine( idx );
    if ( EM::EMM().getObject(horizoninfos_[idx]->id_) )
    {
	mDynamicCastGet( EM::Horizon*, hor, 
	    EM::EMM().getObject(horizoninfos_[idx]->id_) );
	hor->change.remove( mCB(this,HorizonPainter,horChangeCB) );
    }

    EMObjPainterCallbackData cbdata;
    cbdata.objid_ = horizoninfos_[idx]->id_;
    cbdata.name_ = horizoninfos_[idx]->name_;

    delete horizoninfos_[idx];
    horizoninfos_.removeSingle( idx );
    viewer_.handleChange( FlatView::Viewer::Auxdata );

    horizonRemoved.trigger( cbdata );
}


void HorizonPainter::nrHorChangeCB( CallBacker* cb )
{
    if ( cs_.isEmpty() )
	return;
     	
    for ( int idx=EM::EMM().nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID oid = EM::EMM().objectID( idx );

	for ( int horidx=0; horidx<horizoninfos_.size(); horidx ++ )
	    if ( horizoninfos_[horidx]->id_ == oid )
		continue;

	mDynamicCastGet( EM::Horizon*, hor, EM::EMM().getObject( oid ) );
	if ( !hor )
	    continue;

	hor->change.notify( mCB(this,HorizonPainter,horChangeCB) );
	loadinghorcount_++;
    }

    for ( int idx = horizoninfos_.size()-1; idx>=0; idx-- )
    {
	if ( EM::EMM().getObject( horizoninfos_[idx]->id_ ) )
	    continue;

	removeHorizon( idx );
    }
}


void HorizonPainter::horChangeCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( const EM::EMObjectCallbackData&,
	    			cbdata, caller, cb );
    mDynamicCastGet(EM::EMObject*,emobject,caller);
    if ( !emobject ) return;

    int horidx = -1;

    for ( int idx = 0; idx<horizoninfos_.size(); idx++ )
	if ( horizoninfos_[idx]->id_ == emobject->id() )
	{ horidx = idx; break; }

    switch ( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	    break;
	case EM::EMObjectCallbackData::PrefColorChange:
	{
	    if ( !loadinghorcount_ )
		changePolyLineColor( emobject->id() );

	    break;
	}
	case EM::EMObjectCallbackData::PositionChange:
	{
	    if (  emobject->hasBurstAlert() )
		return;

	    BinID bid = BinID::fromInt64( cbdata.pid0.subID() );
	    if ( cs_.hrg.includes(bid) )
	    {
		if ( !emobject->isInsideSelRemoval() )
		{
		    changePolyLinePosition( emobject->id(), cbdata.pid0 );
		    viewer_.handleChange( FlatView::Viewer::Auxdata );
		}
		else
		{
		    if ( emobject->isSelRemoving() )
		    {
			if ( horidx != -1 )
			    horidtoberepainted_ = emobject->id();
		    }
		    else if ( horidtoberepainted_ == emobject->id() )
		    {
			repaintHorizon( emobject->id() );
			horidtoberepainted_ = -1;
		    }
		}
	    }
	    break;
	}
	case EM::EMObjectCallbackData::BurstAlert:
	{
	    if ( emobject->hasBurstAlert() && !loadinghorcount_ )
	    {
		if ( horidx != -1 )
		    horidtoberepainted_ = emobject->id();
	    }
	    else if ( !emobject->hasBurstAlert() )
	    {
		if ( horidtoberepainted_ == emobject->id() )
		{
		    repaintHorizon( emobject->id() );
		    horidtoberepainted_ = -1;
		}
		else 
		{
		    addHorizon( emobject->id() );
		    loadinghorcount_--;
		}
	    }
	    break;
	}
	default:
	    break;    
    }
}


bool HorizonPainter::isDisplayed( const MultiID& mid ) const
{
    EM::ObjectID objid = EM::EMM().getObjectID( mid );

    int horidx = -1;

    for ( int idx = 0; idx<horizoninfos_.size(); idx++ )
	if ( horizoninfos_[idx]->id_ == objid )
	{ horidx = idx; break; }

	if ( horidx < 0 )
	    return false;

    return horizoninfos_[horidx]->lineenabled_;
}


void HorizonPainter::setLineName( const char* lnm )
{ linenm_ = lnm; }

} // namespace EM

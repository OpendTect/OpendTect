/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id: emhorizonpainter.cc,v 1.5 2009-04-07 04:21:15 cvsraman Exp $
________________________________________________________________________

-*/

#include "emhorizonpainter.h"

#include "emhorizon3d.h"
#include "emmanager.h"
#include "emobject.h"
#include "flatview.h"


namespace EM
{


HorizonPainter::HorizonPainter( FlatView::Viewer& fv )
    : viewer_(fv)
    , markerlinestyle_(LineStyle::Solid,2,Color(0,255,0))
    , horrg_(0,1)
    , underburstalertinfluence_(0)
    , burstcount_(0)
{
    EM::EMM().addRemove.notify( mCB(this,HorizonPainter,nrHorChangeCB) );
}


HorizonPainter::~HorizonPainter()
{
    while ( markerlines_.size() )
    {
	EM::EMM().getObject( horizonids_[0] )->change.remove(
		mCB(this,HorizonPainter,horChangeCB) );
	removeHorizon( 0 );
    }

    EM::EMM().addRemove.remove( mCB(this,HorizonPainter,nrHorChangeCB) );
}


void HorizonPainter::addHorizon( const MultiID& mid )
{
    const EM::ObjectID oid = EM::EMM().getObjectID( mid );

    addHorizon( oid );
}


void HorizonPainter::addHorizon( const EM::ObjectID oid )
{
    if ( horizonids_.indexOf(oid)!=-1 )
	    return;

    horizonids_ += oid;

    if ( !addPolyLine(oid) )
	horizonids_.remove( horizonids_.indexOf(oid) );
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter::setHorizonIDs( const ObjectSet<MultiID>* mids )
{
    if ( !mids ) return;

    horizonids_.erase();
    for ( int idx=0; idx<mids->size(); idx++ )
	horizonids_ += EM::EMM().getObjectID( *(*mids)[idx] );

    updateDisplay();
}


void HorizonPainter::setCubeSampling( const CubeSampling& cs, bool update )
{
    cs_ = cs;
    if ( update )
	updateDisplay();
}


bool HorizonPainter::addPolyLine( const EM::ObjectID oid )
{
    EM::EMObject* emobj = EM::EMM().getObject( oid );
    if ( !emobj ) return false;

    mDynamicCastGet(EM::Horizon3D*,hor3d,emobj)
	if ( !hor3d ) return false;

    if ( !underburstalertinfluence_ )
	hor3d->change.notify( mCB(this,HorizonPainter,horChangeCB) );


    ObjectSet<FlatView::Annotation::AuxData>* sectionmarkerlines =
				new ObjectSet<FlatView::Annotation::AuxData>;
    markerlines_ += sectionmarkerlines;

    for ( int ids=0; ids<hor3d->nrSections(); ids++ )
    {
	FlatView::Annotation::AuxData* auxdata =
	    new FlatView::Annotation::AuxData( hor3d->name() );
	auxdata->namepos_ = 0;
	*sectionmarkerlines += auxdata;
	viewer_.appearance().annot_.auxdata_ += auxdata;

	auxdata->poly_.erase();
	auxdata->linestyle_ = markerlinestyle_;
	auxdata->linestyle_.color_ = hor3d->preferredColor();
	auxdata->fillcolor_ = hor3d->preferredColor();

	EM::SectionID sid( ids );
	HorSamplingIterator iter( cs_.hrg );
	BinID bid;
	while( iter.next(bid) )
	{
	    const Coord3 crd = hor3d->getPos( sid, bid.getSerialized() );
	    if ( !crd.isDefined() )
		continue;

	    if ( cs_.defaultDir() == CubeSampling::Inl )
		auxdata->poly_ += FlatView::Point( bid.crl, crd.z );
	    else if ( cs_.defaultDir() == CubeSampling::Crl )
		auxdata->poly_ += FlatView::Point( bid.inl, crd.z );
	    else if ( cs_.defaultDir() == CubeSampling::Z )
		auxdata->poly_ += FlatView::Point( bid.inl, bid.crl );
	}
    }

    return true;
}


void HorizonPainter::changePolyLineColor( const EM::ObjectID oid )
{
    mDynamicCastGet(EM::Horizon3D*,hor3d,EM::EMM().getObject( oid ));

    ObjectSet<FlatView::Annotation::AuxData>* sectionmarkerlines = 
					markerlines_[horizonids_.indexOf(oid)];

    for ( int ids=0; ids<hor3d->nrSections(); ids++ )
    {
	FlatView::Annotation::AuxData* auxdata = (*sectionmarkerlines)[ids];
	auxdata->linestyle_.color_ = hor3d->preferredColor();
    }
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter::updateDisplay()
{
    for ( int idx=0; idx<markerlines_.size(); idx++ )
	removePolyLine( idx );

    for ( int idx=0; idx<horizonids_.size(); idx++ )
    {
	if ( !addPolyLine(horizonids_[idx]) )
		continue;
    }

    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter::setMarkerLineStyle( const LineStyle& ls )
{
    if ( markerlinestyle_==ls )
	return;

    markerlinestyle_ = ls;
    updateDisplay();
}


void HorizonPainter::removeHorizon( const MultiID& mid )
{
    EM::ObjectID objid = EM::EMM().getObjectID( mid );
    const int horidx = horizonids_.indexOf( objid );
    if ( horidx>=0 )
	removeHorizon( horidx );
}


void HorizonPainter::removePolyLine( int idx )
{
    ObjectSet<FlatView::Annotation::AuxData>* sectionmarkerlines = 
							markerlines_[idx];

    for ( int idy=sectionmarkerlines->size()-1; idy>=0; idy-- )
	viewer_.appearance().annot_.auxdata_ -=  (*sectionmarkerlines)[idy];

    deepErase( *sectionmarkerlines );
    delete sectionmarkerlines;

    markerlines_.remove( idx );
}



void HorizonPainter::removeHorizon( int idx )
{
    removePolyLine( idx );
    horizonids_.remove( idx );
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter::nrHorChangeCB( CallBacker* cb )
{
    for ( int idx=EM::EMM().nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID oid = EM::EMM().objectID( idx );
	if ( horizonids_.indexOf(oid)!=-1 )
	    continue;

	mDynamicCastGet( EM::Horizon3D*, hor, EM::EMM().getObject( oid ) );
	if ( !hor )
	    continue;

	hor->change.notify( mCB(this,HorizonPainter,horChangeCB) );
	underburstalertinfluence_ = true;
	burstcount_++;
    }

    for ( int idx = horizonids_.size()-1; idx>=0; idx-- )
    {
	if ( EM::EMM().getObject( horizonids_[idx] ) )
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

    switch ( cbdata.event )
    {
	case EM::EMObjectCallbackData::Undef:
	case EM::EMObjectCallbackData::PrefColorChange:
	{
	    if ( !underburstalertinfluence_ )
		changePolyLineColor( emobject->id() );
	}
	case EM::EMObjectCallbackData::PositionChange:
	{
	    //TODO implement in rt. way
	    //if ( !underburstalertinfluence_ )
	    //	updateDisplay();
	}
	case EM::EMObjectCallbackData::BurstAlert:
	{
	    if ( emobject->isFullyLoaded() )
	    {
	    addHorizon( emobject->id() );
	    underburstalertinfluence_ = false;
	    }
	}
	default:
	    break;    
    }
}


bool HorizonPainter::isDisplayed( const MultiID& mid ) const
{
    EM::ObjectID objid = EM::EMM().getObjectID( mid );
    return horizonids_.indexOf( objid ) >= 0;
}

} // namespace EM

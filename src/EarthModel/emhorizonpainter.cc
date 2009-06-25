/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Mar 2009
 RCS:		$Id: emhorizonpainter.cc,v 1.12 2009-06-25 12:18:04 cvssatyaki Exp $
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
    , loadinghorcount_(0)
    , is2d_(false) 
    , horidtoberepainted_(-1)
{
    cs_.setEmpty();
    EM::EMM().addRemove.notify( mCB(this,HorizonPainter,nrHorChangeCB) );
}


HorizonPainter::~HorizonPainter()
{
    while ( hormarkerlines_.size() )
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


void HorizonPainter::addHorizon( const EM::ObjectID& oid )
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


bool HorizonPainter::addPolyLine( const EM::ObjectID& oid )
{
    EM::EMObject* emobj = EM::EMM().getObject( oid );
    if ( !emobj ) return false;

    mDynamicCastGet(EM::Horizon*,hor,emobj)
	if ( !hor ) return false;

    if ( !loadinghorcount_ )
	hor->change.notify( mCB(this,HorizonPainter,horChangeCB) );


    ObjectSet<ObjectSet<FlatView::Annotation::AuxData> >* sectionmarkerlines =
		new ObjectSet<ObjectSet<FlatView::Annotation::AuxData> >;
    hormarkerlines_ += sectionmarkerlines;

    for ( int ids=0; ids<hor->nrSections(); ids++ )
    {
	ObjectSet<FlatView::Annotation::AuxData>* markerlines = 
	    			new ObjectSet<FlatView::Annotation::AuxData>;
	(*sectionmarkerlines) += markerlines;
	bool newmarker = true;
	bool coorddefined = true;
	int markerlinecount = 0;
	FlatView::Annotation::AuxData* auxdata = 0;

	EM::SectionID sid( ids );
	HorSamplingIterator iter( cs_.hrg );
	BinID bid;
	while( iter.next(bid) )
	{
	    const Coord3 crd = hor->getPos( sid, bid.getSerialized() );
	   if ( !crd.isDefined() )
	   {
	       coorddefined = false;
	       continue;
	   }
	   else if ( !coorddefined )
	   {
	       coorddefined = true;
	       newmarker = true;
	   }

	   if ( newmarker )
	   {
	       auxdata = new FlatView::Annotation::AuxData( hor->name() );
	       auxdata->namepos_ = 0;
	       (*markerlines) += auxdata;
	       viewer_.appearance().annot_.auxdata_ += auxdata;
	       auxdata->poly_.erase();
	       auxdata->linestyle_ = markerlinestyle_;
	       auxdata->linestyle_.color_ = hor->preferredColor();
	       auxdata->fillcolor_ = hor->preferredColor();
	       newmarker = false;
	       markerlinecount++;
	   }
	   if ( cs_.nrInl() == 1 )
	    {
		if ( is2d_ )
		{
		    int idx = trcnos_.indexOf(bid.crl);
		    auxdata->poly_ += FlatView::Point(
			    distances_[idx], crd.z );
		}
		else
		    auxdata->poly_ += FlatView::Point( bid.crl, crd.z );
	    }
	    else if ( cs_.nrCrl() == 1 )
		auxdata->poly_ += FlatView::Point( bid.inl, crd.z );
	}
    }

    return true;
}


void HorizonPainter::changePolyLineColor( const EM::ObjectID& oid )
{
    mDynamicCastGet(EM::Horizon*,hor,EM::EMM().getObject( oid ));

    if ( (horizonids_.indexOf(oid)==-1) || (hormarkerlines_.size() <= 0) || 
	  !hormarkerlines_[horizonids_.indexOf(oid)] )
	return;
    ObjectSet<ObjectSet<FlatView::Annotation::AuxData> >* sectionmarkerlines = 
				hormarkerlines_[horizonids_.indexOf(oid)];

    for ( int ids=0; ids<hor->nrSections(); ids++ )
    {
	ObjectSet<FlatView::Annotation::AuxData>* markerlines = 
	    					(*sectionmarkerlines)[ids];
	for ( int markidx=0; markidx<markerlines->size(); markidx++ )
	{
	    FlatView::Annotation::AuxData* auxdata = (*markerlines)[markidx];
	    auxdata->linestyle_.color_ = hor->preferredColor();
	}
    }
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter::changePolyLinePosition( const EM::ObjectID& oid,
					     const EM::PosID& pid )
{
    mDynamicCastGet(EM::Horizon*,hor,EM::EMM().getObject( oid ));
    if ( (horizonids_.indexOf(oid)==-1) || (hormarkerlines_.size() <= 0) 
	 || !hormarkerlines_[horizonids_.indexOf(oid)] )
	return;
    ObjectSet<ObjectSet<FlatView::Annotation::AuxData> >* sectionmarkerlines =
				hormarkerlines_[horizonids_.indexOf(oid)];

    BinID binid;
    binid.setSerialized( pid.subID() );

    for ( int ids=0; ids<hor->nrSections(); ids++ )
    {
	ObjectSet<FlatView::Annotation::AuxData>* markerlines =
	    					(*sectionmarkerlines)[ids];
	for ( int markidx=0; markidx<markerlines->size(); markidx++ )
	{
	    Coord3 crd = hor->getPos( hor->sectionID(ids), pid.subID() );
	    FlatView::Annotation::AuxData* auxdata = (*markerlines)[markidx];
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

    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter::updateDisplay()
{
    for ( int idx=0; idx<hormarkerlines_.size(); idx++ )
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


void HorizonPainter::repaintHorizon( const EM::ObjectID& oid )
{
     const int horidx = horizonids_.indexOf( oid );
     if ( horidx>=0 )
	 removeHorizon( horidx );

     addHorizon(oid);
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
    ObjectSet<ObjectSet<FlatView::Annotation::AuxData> >* sectionmarkerlines =
							hormarkerlines_[idx];
    for ( int markidx=sectionmarkerlines->size()-1; markidx>=0; markidx-- )
    {
	ObjectSet<FlatView::Annotation::AuxData>* markerlines = 
	    					(*sectionmarkerlines)[markidx];
	for ( int idy=markerlines->size()-1; idy>=0; idy-- )
	    viewer_.appearance().annot_.auxdata_ -= (*markerlines)[idy];

    }
    deepErase( *hormarkerlines_[idx] );
    hormarkerlines_.remove( idx );
}



void HorizonPainter::removeHorizon( int idx )
{
    removePolyLine( idx );
    if ( EM::EMM().getObject(horizonids_[idx]) )
    {
	mDynamicCastGet( EM::Horizon*, hor, 
			 EM::EMM().getObject(horizonids_[idx]) );
	hor->change.remove( mCB(this,HorizonPainter,horChangeCB) );
    }
    horizonids_.remove( idx );
    viewer_.handleChange( FlatView::Viewer::Annot );
}


void HorizonPainter::nrHorChangeCB( CallBacker* cb )
{
    if ( cs_.isEmpty() )
	return;
     	
    for ( int idx=EM::EMM().nrLoadedObjects()-1; idx>=0; idx-- )
    {
	const EM::ObjectID oid = EM::EMM().objectID( idx );
	if ( horizonids_.indexOf(oid)!=-1 )
	    continue;

	mDynamicCastGet( EM::Horizon*, hor, EM::EMM().getObject( oid ) );
	if ( !hor )
	    continue;

	hor->change.notify( mCB(this,HorizonPainter,horChangeCB) );
	loadinghorcount_++;
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

	    BinID bid;
	    bid.setSerialized( cbdata.pid0.subID() );
	    if ( cs_.hrg.includes(bid) )
	    {
		if ( !emobject->isInsideSelRemoval() )
		{
		    changePolyLinePosition( emobject->id(), cbdata.pid0 );
		    viewer_.handleChange( FlatView::Viewer::Annot );
		}
		else
		{
		    if ( emobject->isSelRemoving() )
		    {
			if ( horizonids_.indexOf(emobject->id()) != -1 )
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
		if ( horizonids_.indexOf(emobject->id()) != -1 )
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
    return horizonids_.indexOf( objid ) >= 0;
}

} // namespace EM

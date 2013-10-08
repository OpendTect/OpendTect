/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Ranojay Sen
 Date:		Mar 2011
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visvw2dpickset.h"

#include "angles.h"
#include "attribdatacubes.h"
#include "attribdatapack.h"
#include "cubesampling.h"
#include "flatposdata.h"
#include "indexinfo.h"
#include "ioobj.h"
#include "ioman.h"
#include "pickset.h"
#include "picksettr.h"
#include "separstr.h"
#include "survinfo.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uiflatauxdataeditor.h"
#include "uigraphicsscene.h"
#include "uirgbarraycanvas.h"
#include "uiworld2ui.h"


mCreateVw2DFactoryEntry( VW2DPickSet );

VW2DPickSet::VW2DPickSet( const EM::ObjectID& picksetidx, uiFlatViewWin* win,
			  const ObjectSet<uiFlatViewAuxDataEditor>& editors )
    : Vw2DDataObject()
    , pickset_(0) 					  
    , deselected_(this)
    , isownremove_(false)
{
    if ( picksetidx >= 0 && Pick::Mgr().size() > picksetidx )
	pickset_ = &Pick::Mgr().get( picksetidx );

    for ( int idx=0; idx<editors.size(); idx++ )
    {
	editors_ += const_cast<uiFlatViewAuxDataEditor*>(editors[idx]);
	viewers_ += &editors[idx]->getFlatViewer();
    	picks_ += viewers_[idx]->createAuxData( "Picks" );
	
    	viewers_[idx]->addAuxData( picks_[idx] );
    	viewers_[idx]->appearance().annot_.editable_ = false; 
    	mAttachCB( viewers_[idx]->dataChanged, VW2DPickSet::dataChangedCB );
    	mAttachCB( viewers_[idx]->viewChanged, VW2DPickSet::dataChangedCB );
	
    	auxids_ += editors_[idx]->addAuxData( picks_[idx], true );
    	editors_[idx]->enableEdit( auxids_[idx], true, true, true );
    	editors_[idx]->enablePolySel( auxids_[idx], true );
    	mAttachCB( editors_[idx]->removeSelected, VW2DPickSet::pickRemoveCB );
    	mAttachCB( editors_[idx]->movementFinished, VW2DPickSet::pickAddChgCB );
    }
    
    mAttachCB( Pick::Mgr().setChanged, VW2DPickSet::dataChangedCB );
    mAttachCB( Pick::Mgr().locationChanged, VW2DPickSet::dataChangedCB );
}


VW2DPickSet::~VW2DPickSet()
{
    for ( int ivwr=0; ivwr<viewers_.size(); ivwr++ )
    {
    	viewers_[ivwr]->removeAuxData( picks_[ivwr] );
    	editors_[ivwr]->removeAuxData( auxids_[ivwr] );
    }
    deepErase( picks_ );
    detachAllNotifiers();
}


void VW2DPickSet::pickAddChgCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewAuxDataEditor*,editor,cb); if ( !editor ) return;
    if ( !isselected_ || editor->getSelPtIdx().size() || editor->isSelActive() )
	return;

    FlatView::Point newpt = editor->getSelPtPos();
    const Coord3 crd = getCoord( newpt );
    if ( !crd.isDefined() ) 
	return;
    // Add
    if ( !pickset_ ) 
	return;
    (*pickset_) += Pick::Location( crd );
    const int locidx = pickset_->size()-1;
    Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::Added,
				 pickset_, locidx );
    Pick::Mgr().reportChange( 0, cd );
}


void VW2DPickSet::pickRemoveCB( CallBacker* cb )
{
    mDynamicCastGet(uiFlatViewAuxDataEditor*,editor,cb);
    if ( !editor || !pickset_ ) return;

    const int editoridx = editors_.indexOf( editor );
    if ( editoridx<0 ) return;

    const TypeSet<int>&	selpts = editor->getSelPtIdx();
    const int selsize = selpts.size();
    isownremove_ = selsize == 1;
    for ( int idx=0; idx<selsize; idx++ )
    {
	const int locidx = selpts[idx];
	if ( !picks_[editoridx]->poly_.validIdx(locidx) )
	    continue;
        
	const int pickidx = picksetidxs_[locidx];
	picksetidxs_.removeSingle( locidx );
	Pick::SetMgr::ChangeData cd( Pick::SetMgr::ChangeData::ToBeRemoved,
				     pickset_, pickidx );
	pickset_->removeSingle( pickidx );
	Pick::Mgr().reportChange( 0, cd );
    }
    
    isownremove_ = false;
}


MarkerStyle2D VW2DPickSet::get2DMarkers( const Pick::Set& ps ) const
{
    MarkerStyle2D style( MarkerStyle2D::Square, ps.disp_.pixsize_, 
			 ps.disp_.color_ );
    switch( ps.disp_.markertype_ )
    {
	case MarkerStyle3D::Plane:
	    style.type_ = MarkerStyle2D::Plane;
	    break;
	case MarkerStyle3D::Sphere:
	    style.type_ = MarkerStyle2D::Circle;
	    break;
	case MarkerStyle3D::Cube:
	    style.type_ = MarkerStyle2D::Square;
	    break;
	case MarkerStyle3D::Cone:
	    style.type_ = MarkerStyle2D::Triangle;
	    break;
	case MarkerStyle3D::Cross:
	    style.type_ = MarkerStyle2D::Plus;
	    break;
	case MarkerStyle3D::Arrow:
	    style.type_ = MarkerStyle2D::Arrow;
	    break;
	case MarkerStyle3D::Cylinder:
	    style.type_ = MarkerStyle2D::Square;
	    break;
	default:
    	    style.type_ = MarkerStyle2D::Circle;
    }

    return style;
}


Coord3 VW2DPickSet::getCoord( const FlatView::Point& pt ) const
{
    const FlatDataPack* fdp = viewers_[0]->pack( true );
    if ( !fdp )	fdp = viewers_[0]->pack( false );

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
    if ( dp3d )
    {
	const CubeSampling cs = dp3d->cube().cubeSampling();
	BinID bid; float z;
	if ( dp3d->dataDir() == CubeSampling::Inl )
	{
	    bid = BinID( cs.hrg.start.inl(), (int)pt.x );
	    z = (float) pt.y;
	}
	else if ( dp3d->dataDir() == CubeSampling::Crl )
	{
	    bid = BinID( (int)pt.x, cs.hrg.start.crl() );
	    z = (float) pt.y;
	}
	else
	{
	    bid = BinID( (int)pt.x, (int)pt.y );
	    z = cs.zrg.start;
	}

	return ( cs.hrg.includes(bid) && cs.zrg.includes(z,false) ) ? 
	    Coord3( SI().transform(bid), z ) : Coord3::udf();
    }

    return Coord3::udf();
}


void VW2DPickSet::updateSetIdx( const CubeSampling& cs )
{
    if ( !pickset_ ) return;
    picksetidxs_.erase();
    for ( int idx=0; idx<pickset_->size(); idx++ )
    {
	const Coord3& pos = (*pickset_)[idx].pos_;
	const BinID bid = SI().transform( pos );
	if ( cs.hrg.includes(bid) )
	    picksetidxs_ += idx;
    }
}


void VW2DPickSet::updateSetIdx( const TypeSet<BinID>& bids )
{
    if ( !pickset_ ) return;
    picksetidxs_.erase();
    for ( int idx=0; idx<pickset_->size(); idx++ )
    {
	const Coord3& pos = (*pickset_)[idx].pos_;
	const BinID bid = SI().transform( pos );
	if ( bids.isPresent(bid) )
	    picksetidxs_ += idx;
    }
}


void VW2DPickSet::drawAll()
{
    const FlatDataPack* fdp = viewers_[0]->pack( true );
    if ( !fdp )	fdp = viewers_[0]->pack( false );
    if ( !fdp || !pickset_ ) return;

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
    mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,fdp);
    if ( !dp3d && !dprdm ) return;
    
    const bool oninl = dp3d ? dp3d->dataDir() == CubeSampling::Inl : false;

    if ( dp3d )
	updateSetIdx( dp3d->cube().cubeSampling() );
    else if ( dprdm->pathBIDs() )
	updateSetIdx( *dprdm->pathBIDs() );

    if ( isownremove_ ) return;

    for ( int ivwr=0; ivwr<viewers_.size(); ivwr++ )
    {
	uiFlatViewer& vwr = *viewers_[ivwr];
    	const uiWorldRect& curvw = vwr.curView();
    	const float zdiff = (float) curvw.height();
    	const float nrzpixels = mCast(float,vwr.getViewRect().vNrPics());
    	const float zfac = nrzpixels / zdiff;
    	const float xdiff = (float) ( curvw.width() *
		( oninl ? SI().crlDistance() : SI().inlDistance() ) );
    	const float nrxpixels = mCast(float,vwr.getViewRect().hNrPics());
    	const float xfac = nrxpixels / xdiff;

	FlatView::AuxData* picks = picks_[ivwr];	
    	picks->poly_.erase();
    	picks->markerstyles_.erase();
    	MarkerStyle2D markerstyle = get2DMarkers( *pickset_ );
    	const int nrpicks = picksetidxs_.size();
    	for ( int idx=0; idx<nrpicks; idx++ )
    	{
    	    const int pickidx = picksetidxs_[idx];
    	    const Coord3& pos = (*pickset_)[pickidx].pos_;
    	    const BinID bid = SI().transform(pos);
    	    if ( dp3d )
    	    {
    		BufferString dipval;
    		(*pickset_)[pickidx].getText( "Dip" , dipval );
    		SeparString dipstr( dipval );
    		const float dip = oninl ? dipstr.getFValue( 1 )
					: dipstr.getFValue( 0 );
    		const float depth = (dip/1000000) * zfac;
    		markerstyle.rotation_ =
    		    mIsUdf(dip) ? 0 : Angle::rad2deg( atan2f(2*depth,xfac) );
    		FlatView::Point point( (oninl ? bid.crl() : bid.inl()), pos.z );
    		picks->poly_ += point;
    	    }
    	    else if ( dprdm )
    	    {
    		const FlatPosData& flatposdata = dprdm->posData();
    		const int bidindex = dprdm->pathBIDs()->indexOf(bid);
    		const double bidpos = flatposdata.position( true, bidindex );
    		FlatView::Point point( bidpos, pos.z );
    		picks->poly_ += point;
    	    }
    	    picks->markerstyles_ += markerstyle;
    	}
    	vwr.handleChange( FlatView::Viewer::Auxdata );
    }
}


void VW2DPickSet::clearPicks()
{
    if ( !pickset_ ) return;
    pickset_->erase();
    drawAll();
}


void VW2DPickSet::enablePainting( bool yn )
{
    for ( int ivwr=0; ivwr<viewers_.size(); ivwr++ )
    {
    	picks_[ivwr]->enabled_ = yn;
    	viewers_[ivwr]->handleChange( FlatView::Viewer::Auxdata );
    }
}


void VW2DPickSet::dataChangedCB( CallBacker* )
{
    drawAll();
}


void VW2DPickSet::selected()
{
   isselected_ = true;
}


void VW2DPickSet::triggerDeSel()
{
    isselected_ = false;
    deselected_.trigger();
}


bool VW2DPickSet::fillPar( IOPar& iop ) const
{
    Vw2DDataObject::fillPar( iop );
    iop.set( sKeyMID(), pickSetID() ); 
    return true;
}


bool VW2DPickSet::usePar( const IOPar& iop )
{
    Vw2DDataObject::usePar( iop );
    MultiID mid;
    iop.get( sKeyMID(), mid );

    PtrMan<IOObj> ioobj = IOM().get( mid );
    if ( Pick::Mgr().indexOf(ioobj->key()) >= 0 )
	return false;
    Pick::Set* newps = new Pick::Set; BufferString bs;
    if ( PickSetTranslator::retrieve(*newps,ioobj,true, bs) )
    {
	Pick::Mgr().set( ioobj->key(), newps );
	pickset_ = newps;
	return true;
    }
    delete newps;
    return false;
}


const MultiID VW2DPickSet::pickSetID() const
{
    return pickset_ ? Pick::Mgr().get( *pickset_ ) : -1;
}

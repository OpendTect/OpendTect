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
    , picks_( 0 )
    , editor_(const_cast<uiFlatViewAuxDataEditor*>(editors[0]))
    , viewer_(editor_->getFlatViewer())
    , deselected_(this)
    , isownremove_(false)
{
    if ( picksetidx >= 0 && Pick::Mgr().size() > picksetidx )
	pickset_ = &Pick::Mgr().get( picksetidx );

    picks_ = viewer_.createAuxData( "Picks" );
    viewer_.addAuxData( picks_ );
    viewer_.appearance().annot_.editable_ = false; 
    viewer_.dataChanged.notify( mCB(this,VW2DPickSet,dataChangedCB) );
    viewer_.viewChanged.notify( mCB(this,VW2DPickSet,dataChangedCB) );

    auxid_ = editor_->addAuxData( picks_, true );
    editor_->enableEdit( auxid_, true, true, true );
    editor_->enablePolySel( auxid_, true );
    editor_->removeSelected.notify( mCB(this,VW2DPickSet,pickRemoveCB) );
    editor_->movementFinished.notify( mCB(this,VW2DPickSet,pickAddChgCB) );
    
    Pick::Mgr().setChanged.notify( mCB(this,VW2DPickSet,dataChangedCB) );
    Pick::Mgr().locationChanged.notify( mCB(this,VW2DPickSet,dataChangedCB) );
}


VW2DPickSet::~VW2DPickSet()
{
    viewer_.removeAuxData( picks_ );
    editor_->removeAuxData( auxid_ );
    delete picks_;

    viewer_.dataChanged.remove( mCB(this,VW2DPickSet,dataChangedCB) );
    viewer_.viewChanged.remove( mCB(this,VW2DPickSet,dataChangedCB) );
    editor_->removeSelected.remove( mCB(this,VW2DPickSet,pickRemoveCB) );
    editor_->movementFinished.remove( mCB(this,VW2DPickSet,pickAddChgCB) );
    Pick::Mgr().setChanged.remove( mCB(this,VW2DPickSet,dataChangedCB) );
    Pick::Mgr().locationChanged.remove( mCB(this,VW2DPickSet,dataChangedCB) );
}


void VW2DPickSet::pickAddChgCB( CallBacker* cb )
{
    if ( !isselected_ || editor_->getSelPtIdx().size() 
	 || editor_->isSelActive() )
	return;

    FlatView::Point newpt = editor_->getSelPtPos();
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
    if ( !pickset_ ) return;
    const TypeSet<int>&	selpts = editor_->getSelPtIdx();
    const int selsize = selpts.size();
    isownremove_ = selsize == 1;
    for ( int idx=0; idx<selsize; idx++ )
    {
	const int locidx = selpts[idx];
	if ( !picks_->poly_.validIdx(locidx) )
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
    const FlatDataPack* fdp = viewer_.pack( true );
    if ( !fdp )	fdp = viewer_.pack( false );

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
    if ( dp3d )
    {
	const CubeSampling cs = dp3d->cube().cubeSampling();
	BinID bid; float z;
	if ( dp3d->dataDir() == CubeSampling::Inl )
	{
	    bid = BinID( cs.hrg.start.inl, (int)pt.x );
	    z = (float) pt.y;
	}
	else if ( dp3d->dataDir() == CubeSampling::Crl )
	{
	    bid = BinID( (int)pt.x, cs.hrg.start.crl );
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
    const FlatDataPack* fdp = viewer_.pack( true );
    if ( !fdp )	fdp = viewer_.pack( false );
    if ( !fdp ) return;

    mDynamicCastGet(const Attrib::Flat3DDataPack*,dp3d,fdp);
    mDynamicCastGet(const Attrib::FlatRdmTrcsDataPack*,dprdm,fdp);
    if ( !dp3d && !dprdm ) return;
    
    const bool oninl = dp3d ? dp3d->dataDir() == CubeSampling::Inl : false;
    dp3d ? updateSetIdx( dp3d->cube().cubeSampling() )
	 : updateSetIdx( *dprdm->pathBIDs() );
    if ( isownremove_ ) return;

    const uiWorldRect& curvw = viewer_.curView();
    const float zdiff = (float) curvw.height();
    const float nrzpixels = mCast( float, viewer_.getViewRect().vNrPics() );
    const float zfac = nrzpixels / zdiff;
    const float xdiff = (float) ( curvw.width() *
	( oninl ? SI().crlDistance() : SI().inlDistance() ) );
    const float nrxpixels = mCast( float, viewer_.getViewRect().hNrPics() );
    const float xfac = nrxpixels / xdiff;

    picks_->poly_.erase();
    picks_->markerstyles_.erase();
    if ( !pickset_ ) return;
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
	    FlatView::Point point( (oninl ? bid.crl : bid.inl), pos.z );
	    picks_->poly_ += point;
	}
	else if ( dprdm )
	{
	    const FlatPosData& flatposdata = dprdm->posData();
	    const int bidindex = dprdm->pathBIDs()->indexOf(bid);
	    const double bidpos = flatposdata.position( true, bidindex );
	    FlatView::Point point( bidpos, pos.z );
	    picks_->poly_ += point;
	}
	picks_->markerstyles_ += markerstyle;
    }
    
    viewer_.handleChange( FlatView::Viewer::Auxdata );
}


void VW2DPickSet::clearPicks()
{
    if ( !pickset_ ) return;
    pickset_->erase();
    drawAll();
}


void VW2DPickSet::enablePainting( bool yn )
{
    picks_->enabled_ = yn;
    viewer_.handleChange( FlatView::Viewer::Auxdata );
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

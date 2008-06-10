/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : J.C. Glas
 * DATE     : October 2007
-*/

static const char* rcsID = "$Id: explfaultsticksurface.cc,v 1.21 2008-06-10 19:42:25 cvsyuancheng Exp $";

#include "explfaultsticksurface.h"

#include "arrayndinfo.h"
#include "cubesampling.h"
#include "datapointset.h"
#include "datacoldef.h"
#include "faultsticksurface.h"
#include "geometry.h"
#include "positionlist.h"
#include "posvecdataset.h"
#include "simpnumer.h"
#include "sorting.h"
#include "sortedtable.h"
#include "survinfo.h"
#include "task.h"
#include "trigonometry.h"

namespace Geometry {


class ExplFaultStickTexturePositionExtracter : public ParallelTask
{
public:
ExplFaultStickTexturePositionExtracter( ExplFaultStickSurface& efss,
       					DataPointSet& dpset )
    : explsurf_( efss )
    , dpset_( dpset )  
    , sz_( efss.getTextureSize() )
{
    const DataColDef texture_i( explsurf_.sKeyTextureI() );
    const DataColDef texture_j( explsurf_.sKeyTextureJ() );

    i_column_ = dpset_.dataSet().findColDef(texture_i,PosVecDataSet::NameExact);
    j_column_ = dpset_.dataSet().findColDef(texture_j,PosVecDataSet::NameExact);
}

    
int totalNr() const
{
    return explsurf_.getTextureSize().col;
}


int minThreadSize() const { return 100; }


bool doWork( int start, int stop, int )
{
    const TypeSet<int>& texturerows = explsurf_.texturerowcoords_;
    for ( int stickpos= start; stickpos<=stop; stickpos++, reportNrDone() )
    {
	int stickidx = -1;
	int panelidx = -1;
	for ( int idy=0; idy<texturerows.size()-1; idy++ )
	{
	    if ( texturerows[idy]==stickpos )
	    {
		stickidx = idy;
		panelidx = -1;
		break;
	    }
	    else if ( texturerows[idy]<stickpos && texturerows[idy+1]>stickpos )
	    {
		panelidx = idy;
		break;
	    }
	    else if ( texturerows[texturerows.size()-1]==stickpos )
	    {
		stickidx = texturerows.size()-1;
		panelidx = -1;
		break;
	    }
	}
	
	if ( stickidx==-1 && panelidx==-1 )
	    return false;

	for( int knotpos=0; knotpos<explsurf_.getTextureSize().row; knotpos++ )
	{
    	    Coord3 pos;
    	    bool found = false;

    	    if ( stickidx!=-1 )
    		found = processPixelOnStick( stickidx, knotpos, pos );
    	    else if ( panelidx!=-1 )
    		found = processPixelOnPanel( panelidx, stickpos, knotpos, pos );

	    if ( !found || !pos.isDefined() )
		continue;
 
	    DataPointSet::Pos dpsetpos( pos );
	    DataPointSet::DataRow datarow( dpsetpos, 1 );
	    datarow.data_.setSize( dpset_.nrCols(), mUdf(float) );
	    datarow.data_[i_column_-dpset_.nrFixedCols()] =  knotpos;
	    datarow.data_[j_column_-dpset_.nrFixedCols()] =  stickpos;
	    dpsetlock_.lock();
	    dpset_.addRow( datarow );
	    dpsetlock_.unLock();
	}
    }
    
    dpset_.dataChanged();
    return true;
}


bool processPixelOnStick( int stickidx, int knotpos, Coord3& pos )
{
    if ( !explsurf_.textureknotcoords_.validIdx(stickidx) )
	return false;

    const TypeSet<int>& knotcoords = *explsurf_.textureknotcoords_[stickidx];
    const int nrknots = knotcoords.size();
    if ( knotpos<knotcoords[0] || knotpos>knotcoords[nrknots-1] )
	return false;

    TypeSet<int> crdindices = explsurf_.sticks_[stickidx]->coordindices_;
    if ( nrknots==1 )
    {
	if ( knotpos==knotcoords[0] )
	{
	    pos = explsurf_.coordlist_->get( crdindices[0] );
	    return true;
	}
	else
	    return false;
    }

    for ( int idx=0; idx<nrknots-1; idx++ )
    {
	if ( knotpos==knotcoords[idx] )
	{
	    pos = explsurf_.coordlist_->get( crdindices[idx] );
	    return true;
	}
	else if ( knotpos>knotcoords[idx] && knotpos<knotcoords[idx+1] )
	{
	    const Coord3 p0 = explsurf_.coordlist_->get(crdindices[idx] );
	    const Coord3 p1 = explsurf_.coordlist_->get(crdindices[idx+1] );
	    pos = p0+(p1-p0)*(knotpos-knotcoords[idx])/(
		    knotcoords[idx+1]-knotcoords[idx]);
	    return true;
	}
    }

    if ( knotpos==knotcoords[nrknots-1] )
    {
	pos = explsurf_.coordlist_->get( crdindices[nrknots-1] );
	return true;
    }

    return true;
}


bool processPixelOnPanel( int panelidx, int stickpos, int knotpos, Coord3& pos )
{
    IndexedGeometry* triangles = explsurf_.paneltriangles_[panelidx];
    if ( !triangles )
	return false;
    
    const int lpos = explsurf_.texturerowcoords_[panelidx];
    const int rpos = explsurf_.texturerowcoords_[panelidx+1];
    if ( stickpos<lpos || stickpos>rpos )
	return false;
    
    const TypeSet<int>& lidx = (*explsurf_.sticks_[panelidx]).coordindices_;
    const TypeSet<int>& ridx = (*explsurf_.sticks_[panelidx+1]).coordindices_;
    if ( lidx.isEmpty() || ridx.isEmpty() )   
	return false;    

    const TypeSet<int>& lknots = *explsurf_.textureknotcoords_[panelidx];
    const TypeSet<int>& rknots = *explsurf_.textureknotcoords_[panelidx+1];
    if ( knotpos<lknots[0] && knotpos<rknots[0] || 
	 knotpos>lknots[lknots.size()-1] && knotpos>rknots[rknots.size()-1] )
	return false;

    const Coord checkpos( stickpos, knotpos );
    const int lendid = lidx.size()-1;
    const int rendid = ridx.size()-1;
    for ( int idx=0; idx<triangles->coordindices_.size()/4; idx++ )
    {
	const int v0 = triangles->coordindices_[4*idx];
	const int v1 = triangles->coordindices_[4*idx+1];
	const int v2 = triangles->coordindices_[4*idx+2];

	const int lp0 = lidx.indexOf(v0);
	const int lp1 = lidx.indexOf(v1);
	const int lp2 = lidx.indexOf(v2);
	const int rp0 = ridx.indexOf(v0);
	const int rp1 = ridx.indexOf(v1);
	const int rp2 = ridx.indexOf(v2);
    
	Coord texture0, texture1, texture2;

	if ( lp0!=-1 && lp1!=-1 && rp2!=-1 )
	{
	    texture0 = Coord( lpos, lknots[lp0] );
	    texture1 = Coord( lpos, lknots[lp1] );
	    texture2 = Coord( rpos, rknots[rp2] );
	}
	else if ( lp0!=-1 && lp2!=-1 && rp1!=-1 )
	{
	    texture0 = Coord( lpos, lknots[lp0] );
	    texture1 = Coord( rpos, rknots[rp1] );
	    texture2 = Coord( lpos, lknots[lp2] );
	}
	else if ( lp1!=-1 && lp2!=-1 && rp0!=-1 )
	{
	    texture0 = Coord( rpos, rknots[rp0] );
	    texture1 = Coord( lpos, lknots[lp1] );
	    texture2 = Coord( lpos, lknots[lp2] );
	}
	else if ( rp0!=-1 && rp1!=-1 && lp2!=-1 )
	{
	    texture0 = Coord( rpos, rknots[rp0] );
	    texture1 = Coord( rpos, rknots[rp1] );
	    texture2 = Coord( lpos, lknots[lp2] );
	}
	else if ( rp0!=-1 && rp2!=-1 && lp1!=-1 )
	{
	    texture0 = Coord( rpos, rknots[rp0] );
	    texture1 = Coord( lpos, lknots[lp1] );
	    texture2 = Coord( rpos, rknots[rp2] );
	}
	else if ( rp1!=-1 && rp2!=-1 && lp0!=-1 )
	{
	    texture0 = Coord( lpos, lknots[lp0] );
	    texture1 = Coord( rpos, rknots[rp1] );
	    texture2 = Coord( rpos, rknots[rp2] );
	}


	bool intriangle = pointInTriangle2D( checkpos, texture0, texture1, 
					     texture2, 1e-5 );
	if ( !intriangle )
	{
	    const bool v01onend = (v0==lidx[0] && v1==ridx[0]) ||
				  (v0==ridx[0] && v1==lidx[0]) ||
				  (v0==lidx[lendid] && v1==ridx[rendid]) ||
				  (v0==ridx[rendid] && v1==lidx[lendid]);
	    const bool v12onend = (v1==lidx[0] && v2==ridx[0]) ||
				  (v1==ridx[0] && v2==lidx[0]) ||
				  (v1==lidx[lendid] && v2==ridx[rendid]) ||
				  (v1==ridx[rendid] && v2==lidx[lendid]);
	    const bool v20onend = (v2==lidx[0] && v0==ridx[0]) ||
				  (v2==ridx[0] && v0==lidx[0]) ||
				  (v2==lidx[lendid] && v0==ridx[rendid]) ||
				  (v2==ridx[rendid] && v0==lidx[lendid]);

	    if ( v01onend )
		intriangle = pointOnEdge2D( checkpos, texture0, texture1, 1 );
	    else if ( v12onend )
		intriangle = pointOnEdge2D( checkpos, texture1, texture2, 1 );
	    else if ( v20onend )
		intriangle = pointOnEdge2D( checkpos, texture2, texture0, 1 );
	}

	if ( intriangle )
	{
	    const Coord d0 = texture1-texture0;
	    const Coord d1 = checkpos-texture0;
	    const Coord d2 = texture1-texture2;
	    const float fchkpt0 = (d0.x*d2.y-d0.y*d2.x)/(d1.x*d2.y-d1.y*d2.x);
	    const double factor12=(d1.x*d0.y-d1.y*d0.x)/(d1.x*d2.y-d1.y*d2.x);

	    const Coord3 p0 = explsurf_.coordlist_->get( v0 );
	    const Coord3 p1 = explsurf_.coordlist_->get( v1 );
	    const Coord3 p2 = explsurf_.coordlist_->get( v2 );
	    const Coord3 intsectptxy = p1+(p2-p1)*factor12;
    	    pos =  p0+(intsectptxy- p0)/fchkpt0;
	    
	    return true;
	}
    }

    return false;
}

    Threads::Mutex		dpsetlock_;
    ExplFaultStickSurface&	explsurf_;
    DataPointSet&		dpset_;
    RowCol			sz_;
    int				i_column_;
    int				j_column_;
};


#undef mStick
#undef mKnot


class ExplFaultStickSurfaceUpdater : public ParallelTask
{
public:
    ExplFaultStickSurfaceUpdater( ExplFaultStickSurface& efss,
	    			  bool updatesticksnotpanels )
    : explsurf_( efss )
    , updatesticksnotpanels_( updatesticksnotpanels )
{
    if ( updatesticksnotpanels_ )
    {
	if ( explsurf_.displaysticks_ || explsurf_.displaypanels_ )
	{
	    for ( int idx=0; idx<explsurf_.sticks_.size(); idx++ )
	    {
		if ( explsurf_.sticks_[idx]->coordindices_.isEmpty() )
		    updatelist_ += idx;
	    }
	}
    }
    else if ( explsurf_.displaypanels_ )
    {
	for ( int idx=0; idx<explsurf_.paneltriangles_.size()-1; idx++ )
	{
	    if ( explsurf_.paneltriangles_[idx] &&
		 !explsurf_.paneltriangles_[idx]->isEmpty() )
		continue;

	    if ( explsurf_.panellines_[idx] &&
		 !explsurf_.panellines_[idx]->isEmpty() )
		continue;

	    updatelist_ += idx;
	}
    }
}


int totalNr() const { return updatelist_.size(); }

int minThreadSize() const { return 1; }


bool doWork( int start, int stop, int )
{
    for ( int idx=start; idx<=stop && idx<updatelist_.size();
	  idx++, reportNrDone() )
    {
	if ( updatesticksnotpanels_ )
	    explsurf_.fillStick( updatelist_[idx] );
	else
	    explsurf_.fillPanel( updatelist_[idx] );
    }
    return true;
}

protected:

    ExplFaultStickSurface&	explsurf_;
    bool			updatesticksnotpanels_;
    TypeSet<int>		updatelist_;
};


ExplFaultStickSurface::ExplFaultStickSurface( FaultStickSurface* surf,
					      float zscale )
    : surface_( 0 )
    , displaysticks_( true )
    , displaypanels_( true )
    , scalefacs_( 1, 1, zscale )
    , needsupdate_( true )
    , maximumtexturesize_( 1024 )
    , texturesize_( mUdf(int), mUdf(int) )
    , texturepot_( true )
    , texturesampling_( BinID( SI().inlStep(), SI().crlStep() ), SI().zStep() )
{
    paneltriangles_.allowNull( true );
    panellines_.allowNull( true );
    setSurface( surf );
}


ExplFaultStickSurface::~ExplFaultStickSurface()
{
    setSurface( 0 );
    deepErase( textureknotcoords_ );
}


void ExplFaultStickSurface::setSurface( FaultStickSurface* fss )
{
    if ( surface_ )
    {
	surface_->nrpositionnotifier.remove(
			mCB(this,ExplFaultStickSurface,surfaceChange) );
	surface_->movementnotifier.remove(
			mCB(this,ExplFaultStickSurface,surfaceMovement) );
    }

    removeAll();
    surface_ = fss;

    if ( surface_ )
    {
	surface_->nrpositionnotifier.notify(
			mCB(this,ExplFaultStickSurface,surfaceChange) );
	surface_->movementnotifier.notify(
			mCB(this,ExplFaultStickSurface,surfaceMovement) );

	if ( coordlist_ )
	{
	    insertAll();
	}
    }
}


void ExplFaultStickSurface::setZScale( float zscale )
{
    scalefacs_.z = zscale;
    for ( int idx=sticks_.size()-2; idx>=0; idx-- )
	emptyPanel( idx );
}


void ExplFaultStickSurface::removeAll()
{
    for ( int idx=sticks_.size()-1; idx>=0; idx-- )
    {
	removePanel( idx );
	removeStick( idx );
    }

    texturesize_ = RowCol( mUdf(int), mUdf(int) );
    textureknotcoords_.erase();
}


void ExplFaultStickSurface::insertAll()
{
    if ( !surface_ || surface_->isEmpty() )
	return;

    for ( int idx=0; idx<=surface_->rowRange().nrSteps(); idx++ )
    {
	insertStick( idx );
	insertPanel( idx );
    }
}


bool ExplFaultStickSurface::update( bool forceall, TaskRunner* tr )
{
    if ( forceall )
    {
	removeAll();
	insertAll();
    }

    //First update the sticks since they are needed for the panels
    PtrMan<ExplFaultStickSurfaceUpdater> updater =
	new ExplFaultStickSurfaceUpdater( *this, true );

    if ( (tr && !tr->execute( *updater ) ) || !updater->execute() )
	return false;

    //Now do panels
    updater = new ExplFaultStickSurfaceUpdater( *this, false );

    if ( (tr && !tr->execute( *updater ) ) || !updater->execute() )
	return false;

    if ( !updateTextureSize() )
	return false;

    updateTextureCoords();
    needsupdate_ = false;
    return true;
}


void ExplFaultStickSurface::updateTextureCoords()
{
    SortedTable<Coord3> texturecoords;

    for ( int stickidx=0; stickidx<sticks_.size(); stickidx++ )
    {
	IndexedGeometry* stick = sticks_[stickidx];
	if ( !stick )
	    continue;

	for ( int idx=0; idx<stick->coordindices_.size(); idx++ )
	{
	    const int ci = stick->coordindices_[idx];
	    if ( ci==-1 )
		continue;

	    const float rowcoord = 
		(texturerowcoords_[stickidx]+0.5)/texturesize_.col;

	    const float knotpos = 
		((*textureknotcoords_[stickidx])[idx]+0.5)/ texturesize_.row;

	    texturecoords.set( ci, Coord3(knotpos,rowcoord,0) );
	}
    }

    for ( int panelidx=0; panelidx<paneltriangles_.size(); panelidx++ )
    {
	IndexedGeometry* triangles = paneltriangles_[panelidx];
	if ( !triangles )
	    continue;

	SortedTable<int> texturecoordindices;
	for ( int idx=0; idx<triangles->coordindices_.size(); idx++ )
	{
	    const int ci = triangles->coordindices_[idx];
	    int tci;
	    if ( ci==-1 )
		tci = -1;
	    else
	    {
		Coord3 tc;
		if ( !texturecoords.get( ci, tc ) )
		{
		    tci = -1;
		    pErrMsg("Hough!");
		}
		else if ( !texturecoordindices.get( ci, tci ) )
		    tci = texturecoordlist_->add( tc );
	    }

	    triangles->texturecoordindices_ += tci;
	}
    }
}


void ExplFaultStickSurface::display( bool ynsticks, bool ynpanels )
{
    if ( displaysticks_!=ynsticks )
    {
	for ( int idx=0; idx<sticks_.size(); idx++ )
	{
	    if ( ynsticks )
		addToGeometries( sticks_[idx] );
	    else
		removeFromGeometries( sticks_[idx] );
	}
    }

    displaysticks_ = ynsticks;

    if ( displaypanels_!=ynpanels )
    {
	for ( int idx=0; idx<paneltriangles_.size(); idx++ )
	{
	    if ( ynpanels )
	    {
		addToGeometries( paneltriangles_[idx] );
		addToGeometries( panellines_[idx] );
	    }
	    else
	    {
		removeFromGeometries( paneltriangles_[idx] );
		removeFromGeometries( panellines_[idx] );
	    }
	}
    }

    displaypanels_ = ynpanels;
}


int ExplFaultStickSurface::textureColSz( const int panelidx )
{
    const IndexedGeometry* triangles = paneltriangles_[panelidx];
    if ( !triangles )
	return 0;

    TypeSet<int> knots0 = sticks_[panelidx]->coordindices_;
    TypeSet<int> knots1 = sticks_[panelidx+1]->coordindices_;
    if ( knots0.size()==1 && knots1.size()==1 )
	return 1;

    int res = 0;
    for ( int idx=0; idx<triangles->coordindices_.size()/4; idx++ )
    {
	const int v0 = triangles->coordindices_[4*idx];
	const int v1 = triangles->coordindices_[4*idx+1];
	const int v2 = triangles->coordindices_[4*idx+2];

	const bool v0onstick0 = knots0.indexOf(v0)!=-1;
	const bool v1onstick0 = knots0.indexOf(v1)!=-1;
	const bool v2onstick0 = knots0.indexOf(v2)!=-1;

	const bool lineonstick0 = (v0onstick0+v1onstick0+v2onstick0)>1;
	int checkpt,lp0,lp1;
	if ( lineonstick0 )
	{
	    if ( !v0onstick0 )
	    { checkpt=v0; lp0=v1; lp1=v2; }
	    else if ( !v1onstick0 )
	    { checkpt=v1; lp0=v0; lp1=v2; }
	    else if ( !v2onstick0 )
	    { checkpt=v2; lp0=v0; lp1=v1; }
	}
	else
	{
	    if ( v0onstick0 )
	    { checkpt=v0; lp0=v1; lp1=v2; }
	    else if ( v1onstick0 )
	    { checkpt=v1; lp0=v0; lp1=v2; }
	    else if ( v2onstick0 )
	    { checkpt=v2; lp0=v0; lp1=v1; }
	}

    	const int sz = point2LineSampleSz( coordlist_->get(checkpt),
		coordlist_->get(lp0), coordlist_->get(lp1) );

	if ( res<sz )
	    res = sz;
    }

    return res;
}


int ExplFaultStickSurface::point2LineSampleSz( const Coord3& point, 
	const Coord3& linept0, const Coord3& linept1 )
{
    const BinID ptbid = SI().transform(point.coord());
    const BinID lp0bid = SI().transform(linept0.coord());
    const BinID lp1bid = SI().transform(linept1.coord());

    const Coord3 lp0relpos((lp0bid.inl-ptbid.inl)/texturesampling_.binid.inl,
			   (lp0bid.crl-ptbid.crl)/texturesampling_.binid.crl,
			   (linept0.z-point.z)/texturesampling_.value);

    const Coord3 lp1relpos((lp1bid.inl-ptbid.inl)/texturesampling_.binid.inl,
			   (lp1bid.crl-ptbid.crl)/texturesampling_.binid.crl,
			   (linept1.z-point.z)/texturesampling_.value);


    const Coord3 dir = lp0relpos-lp1relpos;
    const Coord3 diff = -lp1relpos;
    const float u = ( diff.x*dir.x+diff.y*dir.y+ diff.z*dir.z )/
	(dir.x*dir.x+dir.y*dir.y+dir.z*dir.z);
		
    const float nrsamples = (lp1relpos+u*dir).abs();
    return mNINT( nrsamples );
}


bool ExplFaultStickSurface::updateTextureSize()
{
    if ( sticks_.size()<2 )
	return false;

    int texturerowsz;
    ObjectSet< TypeSet<int> > sticksegments;
    for ( int stickidx=0; stickidx<sticks_.size(); stickidx++ )
    {
	const int sticknr = surface_->rowRange().atIndex( stickidx );
	const StepInterval<int> colrg = surface_->colRange( sticknr );
	
	int sticktexturerowsz = 0;
	TypeSet<int>& segmentsizes = *new TypeSet<int>;
	sticksegments += &segmentsizes;
	
	if ( colrg.start==colrg.stop )
	{
	    sticktexturerowsz = 1;
	    segmentsizes += 1;
	}
	else
	{
	    for ( int knotnr=colrg.start; knotnr<=colrg.stop-colrg.step; 
		  knotnr += colrg.step )
	    {
		const Coord3 pos0 = surface_->getKnot( RowCol(sticknr,knotnr) );
		const Coord3 pos1 =
		    surface_->getKnot( RowCol(sticknr,knotnr+colrg.step));

		const BinIDValue bid0( SI().transform( pos0.coord()), pos0.z );
		const BinIDValue bid1( SI().transform( pos1.coord()), pos1.z );

		const int inlsamples =
		    (bid0.binid.inl-bid1.binid.inl)/texturesampling_.binid.inl;
		const int crlsamples =
		    (bid0.binid.crl-bid1.binid.crl)/texturesampling_.binid.crl;
		const float zsamples =
		    (bid0.value-bid1.value)/texturesampling_.value;

		const float nrsamples = sqrt( inlsamples*inlsamples +
					      crlsamples*crlsamples +
					      zsamples*zsamples );

		const int sz = mNINT( nrsamples );
		sticktexturerowsz += sz;
		segmentsizes += sz;
	    }
	}

	if ( !stickidx || texturerowsz<sticktexturerowsz )
	    texturerowsz = sticktexturerowsz;
    }

    int texturecolsz = 0;
    TypeSet<int> coldists;
    for ( int panelidx=0; panelidx<sticks_.size()-1; panelidx++ )
    {
	const int colsz = textureColSz( panelidx );
	coldists += colsz;
	texturecolsz += colsz;
    }

    if ( !texturerowsz || !texturecolsz )
	return false;
    
    int po2rowsz = texturerowsz;
    int po2colsz = texturecolsz;
    if ( texturepot_ )
    {
	const bool df = !mIsUdf(maximumtexturesize_);
	po2rowsz = df ? getPow2Sz(texturerowsz,true,1,maximumtexturesize_) :
	    		nextPower( texturerowsz, 2 );
	po2colsz = df ? getPow2Sz(texturecolsz,true,1,maximumtexturesize_) :
	    		nextPower( texturecolsz, 2 );
    }

    texturesize_ = RowCol( po2rowsz, po2colsz );

    texturerowcoords_.erase();
    texturerowcoords_ +=0;
    int sum = 0;
    const float colfactor = (float)po2colsz/texturecolsz;
    for ( int idx=0; idx<coldists.size()-1; idx++ )
    {
	sum += coldists[idx];
	texturerowcoords_ += mNINT( colfactor*sum );
    }

    texturerowcoords_ += po2colsz-1;

    deepErase( textureknotcoords_ );
    for ( int idx=0; idx<sticksegments.size(); idx++ )
    {
    	TypeSet<int>& segmentsizes = *new TypeSet<int>;
    	textureknotcoords_ += &segmentsizes;

	if ( (*sticksegments[idx]).size()==1 && (*sticksegments[idx])[0]==1 )
	{
	    segmentsizes += po2rowsz/2;
	    continue; 
	}

	sum = 0;
	for ( int idy=0; idy<(*sticksegments[idx]).size(); idy++ )
	    sum += (*sticksegments[idx])[idy];

	const float rowfactor = (float)po2rowsz/sum;
	const float st0 = (float)po2rowsz*(texturerowsz-sum)/(2*texturerowsz);
	const int start = mNINT( st0 );
	segmentsizes += start;

	sum = 0;
	for ( int idy=0; idy<(*sticksegments[idx]).size()-1; idy++ )
	{
	    sum += (*sticksegments[idx])[idy];
	    segmentsizes += mNINT( rowfactor*sum )+start;
	}

	segmentsizes += po2rowsz-1-start;
    }
   
    deepErase( sticksegments );
    return true;
}


void ExplFaultStickSurface::needUpdateTextureSize( bool yn )
{
    if ( !yn )
	return;

    updateTextureSize();
}


void ExplFaultStickSurface::setMaximumTextureSize( int sz )
{ maximumtexturesize_ = sz; }


void ExplFaultStickSurface::setTexturePowerOfTwo( bool yn )
{ texturepot_ = yn; }


void ExplFaultStickSurface::setTextureSampling( const BinIDValue& bidv )
{ texturesampling_ = bidv; }


const RowCol& ExplFaultStickSurface::getTextureSize() const
{ return texturesize_; }


bool ExplFaultStickSurface::setTexturePositions( DataPointSet& dpset )
{ 
    const DataColDef texture_i( sKeyTextureI() );
    if ( dpset.dataSet().findColDef(texture_i,PosVecDataSet::NameExact)==-1 )
    {
	dpset.dataSet().add( new DataColDef(texture_i) );
	dpset.dataSet().add( new DataColDef(sKeyTextureJ()) );
    }

    PtrMan<ExplFaultStickTexturePositionExtracter> extractor =
	new ExplFaultStickTexturePositionExtracter( *this, dpset );

    return extractor->execute();
}


void ExplFaultStickSurface::addToGeometries( IndexedGeometry* ig )
{
    if ( !ig ) return;
    geometrieslock_.writeLock();
    if ( geometries_.indexOf( ig )!=-1 )
    {
	pErrMsg("Adding more than once");
    }

    ig->ischanged_ = true;
    geometries_ += ig;
    geometrieslock_.writeUnLock();
}


void ExplFaultStickSurface::removeFromGeometries( const IndexedGeometry* ig )
{
    if ( !ig ) return;
    geometrieslock_.writeLock();
    const int idx = geometries_.indexOf( ig );

    if ( idx!=-1 )
	geometries_.removeFast( idx );

    geometrieslock_.writeUnLock();
}


void ExplFaultStickSurface::emptyStick( int stickidx )
{
    if ( !sticks_.validIdx(stickidx) )
	return;

    sticks_[stickidx]->removeAll();

    needsupdate_ = true;
}


void ExplFaultStickSurface::fillStick( int stickidx )
{
    if ( !surface_ || !coordlist_ || !sticks_.validIdx(stickidx) )
	return;

    TypeSet<int>& knots = sticks_[stickidx]->coordindices_;
    if ( !knots.isEmpty() )
	return;

    const int sticknr = surface_->rowRange().atIndex( stickidx );
    const StepInterval<int> colrg = surface_->colRange( sticknr );
    
    for ( int knotnr=colrg.start; knotnr<=colrg.stop; knotnr+=colrg.step )
    {
	const Coord3 pos = surface_->getKnot( RowCol(sticknr,knotnr) );
	knots += coordlist_->add( pos );
    }

    sticks_[stickidx]->ischanged_ = true;
}


void ExplFaultStickSurface::removeStick( int stickidx )
{
    if ( !sticks_.validIdx(stickidx) )
	return;

    removeFromGeometries( sticks_[stickidx] );
    delete sticks_.remove( stickidx );
    needsupdate_ = true;
}


void ExplFaultStickSurface::insertStick( int stickidx )
{
    if ( stickidx<0 || stickidx>sticks_.size() )
	return;

    sticks_.insertAt(
	new IndexedGeometry(IndexedGeometry::Lines,
	    IndexedGeometry::PerFace,coordlist_,0,0),
	stickidx);

    if ( displaysticks_ ) addToGeometries( sticks_[stickidx] );

    needsupdate_ = true;
}


#define mSqDist( coordidx1, coordidx2 ) \
    coordlist_->get(coordidx1).scaleBy(scalefacs_).sqDistTo( \
    coordlist_->get(coordidx2).scaleBy(scalefacs_) ) 


void ExplFaultStickSurface::emptyPanel( int panelidx )
{
    if ( !paneltriangles_.validIdx(panelidx) )
	return;

    if ( paneltriangles_[panelidx] ) paneltriangles_[panelidx]->removeAll();
    if ( panellines_[panelidx] ) panellines_[panelidx]->removeAll();

    needsupdate_ = true;
}


#define mSqDistArr( a, b ) sqdists[(a)*rsize+(b)]

#define mAddConnection( il, ir ) \
{ \
    lconn += il; \
    rconn += ir; \
    mSqDistArr( il, ir ) = mUdf(float); \
}

#define mAddTriangle( a, b, c ) \
{									\
    triangles->coordindices_ += (a);					\
    triangles->coordindices_ += (b);					\
    triangles->coordindices_ += (c);					\
    triangles->coordindices_ += -1;					\
    Coord3 vec1 = coordlist_->get( a );					\
    const Coord3 vec2 = coordlist_->get( b )-vec1;			\
    vec1 = coordlist_->get( c ) - vec1;					\
    const Coord3 normal = vec1.cross( vec2 );				\
    triangles->normalindices_ += normallist_->add( normal );		\
}

void ExplFaultStickSurface::setRightHandedNormals( bool yn )
{
    if ( yn==righthandednormals_ )
	return;

    IndexedShape::setRightHandedNormals( yn );

    if ( !normallist_ )
	return;

    //Undo the inverse that is done by display layer.
    int id = -1;
    while ( true )
    {
	id = normallist_->nextID( id );
	if ( id==-1 )
	    break;

	normallist_->set( id, -normallist_->get( id ) );
    }
}


void ExplFaultStickSurface::fillPanel( int panelidx )
{
    if ( !coordlist_ || !normallist_ || !paneltriangles_.validIdx(panelidx) )
	return;

    IndexedGeometry* triangles = paneltriangles_[panelidx];
    IndexedGeometry* lines = panellines_[panelidx];

    if ( triangles && !triangles->isEmpty() )
	triangles->removeAll();

    if ( lines && !lines->isEmpty() )
	lines->removeAll();
    
    const TypeSet<int>& lknots = sticks_[panelidx]->coordindices_;
    const TypeSet<int>& rknots = sticks_[panelidx+1]->coordindices_;

    const int lsize = lknots.size(); 
    const int rsize = rknots.size();

    if ( !lsize || !rsize )
	return;

    if ( lsize==1 && rsize==1 )
    {
	if ( !lines )
	{
	    lines = new IndexedGeometry( IndexedGeometry::Lines,
		    IndexedGeometry::PerFace, 0, normallist_,texturecoordlist_);
	    panellines_.replace( panelidx, lines );
	    if ( displaypanels_ ) addToGeometries( lines );
	}

	lines->coordindices_ += lknots[0];
	lines->coordindices_ += rknots[0];

	return;
    }

    if ( !triangles )
    {
	triangles = new IndexedGeometry( IndexedGeometry::TriangleStrip,
					 IndexedGeometry::PerFace,
					 0, normallist_, texturecoordlist_ );
	paneltriangles_.replace( panelidx, triangles );
	if ( displaypanels_ ) addToGeometries( triangles );
    }

    float sqdists[lsize*rsize];
    for ( int idx=0; idx<lsize; idx++ )
    {
	for ( int idy=0; idy<rsize; idy++ )
	{
	    const float sqdist = mSqDist( lknots[idx], rknots[idy] );
	    mSqDistArr( idx, idy ) =  sqdist;
	}
    }

    TypeSet<int> lconn,rconn;
    mAddConnection( 0, 0 );
    mAddConnection( lsize-1, rsize-1 );

    while ( true )
    {
	float minsqdist;
	int minl=-1, minr;
	for ( int idx=0; idx<lsize; idx++ )
	{
	    for ( int idy=0; idy<rsize; idy++ )
	    {
		const float sqdist = mSqDistArr( idx, idy );
		if ( mIsUdf(sqdist) )
		    continue;

		if ( minl!=-1 && sqdist>minsqdist )
		    continue;

		minl = idx;
		minr = idy;
		minsqdist = sqdist;
	    }
	}

	if ( minl==-1 )
	    break;

	mAddConnection( minl, minr );
	//Remove connections that are not possible anylonger
	for ( int idx=0; idx<lsize; idx++ )
	{
	    if ( idx==minl )
		continue;

	    for ( int idy=0; idy<rsize; idy++ )
	    {
		if ( idy==minr )
		    continue;

		if ( idx>minl == idy>minr )
		    continue;

		mSqDistArr( idx, idy ) = mUdf(float);
	    }
	}
    }

    for ( int lidx=0; lidx<lsize; lidx++ )
    {
	TypeSet<int> conns;
	for ( int idx=lconn.size()-1; idx>=0; idx-- )
	{
	    if ( lconn[idx]!=lidx )
		continue;

	    conns += rconn[idx];
	}

	sort_array( conns.arr(), conns.size() );

	if ( lidx )
	    mAddTriangle( lknots[lidx], rknots[conns[0]], lknots[lidx-1] );

	for ( int idx=1; idx<conns.size(); idx++ )
	    mAddTriangle(lknots[lidx],rknots[conns[idx]],rknots[conns[idx-1]]);
    }
}


void ExplFaultStickSurface::removePanel( int panelidx )
{
    if ( !paneltriangles_.validIdx(panelidx) )
	return;

    removeFromGeometries( paneltriangles_[panelidx] );
    removeFromGeometries(  panellines_[panelidx] );
    delete paneltriangles_.remove( panelidx );
    delete panellines_.remove( panelidx );
    needsupdate_ = true;
}


void ExplFaultStickSurface::insertPanel( int panelidx )
{
    if ( panelidx<0 || panelidx>paneltriangles_.size() )
	return;

    paneltriangles_.insertAt( 0, panelidx );
    panellines_.insertAt( 0, panelidx );
    needsupdate_ = true;
}


void ExplFaultStickSurface::surfaceChange( CallBacker* cb )
{
    mCBCapsuleUnpack( const TypeSet<GeomPosID>*, pidlist, cb );
    for ( int idx=0; pidlist && idx<pidlist->size(); idx++ )
    {
	RowCol rc;
	rc.setSerialized( (*pidlist)[idx] );
	const int stickidx = rc.r();

	if ( rc.c()==FaultStickSurface::StickChange )
	{
	    emptyPanel( stickidx-1 );
	    emptyPanel( stickidx );
	    emptyStick( stickidx );
	}
	if ( rc.c()==FaultStickSurface::StickInsert )
	{
	    emptyPanel( stickidx-1 );
	    insertPanel( !stickidx ? 0 : stickidx-1 );
	    insertStick( stickidx );
	}
	if ( rc.c()==FaultStickSurface::StickRemove )
	{
	    emptyPanel( stickidx );
	    removePanel( !stickidx ? 0 : stickidx-1 );
	    removeStick( stickidx );
	}
    }

    addVersion();
}


void ExplFaultStickSurface::surfaceMovement( CallBacker* cb )
{
    mCBCapsuleUnpack( const TypeSet<GeomPosID>*, pidlist, cb );
    for ( int idx=0; pidlist && idx<pidlist->size(); idx++ )
    {
	RowCol rc;
	rc.setSerialized( (*pidlist)[idx] );
	const int stickidx = rc.r();

	emptyPanel( stickidx-1 );
	emptyPanel( stickidx );
	emptyStick( stickidx );
    }

    addVersion();
}

}; // namespace Geometry

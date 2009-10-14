
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		May 2008
 RCS:		$Id: emobjectselremoval.cc,v 1.7 2009-10-14 08:09:49 cvsjaap Exp $
________________________________________________________________________

-*/

#include "emobjectselremoval.h"
#include "emobject.h"
#include "parametricsurface.h"
#include "survinfo.h"

namespace EM
{
    
EMObjectSelectionRemoval::EMObjectSelectionRemoval(EMObject& emobj,
						   const SectionID& secid,
						   const Selector<Coord3>& sel,
						   int nrrows, int nrcols,
       						   int startrow, int startcol )
    : emobj_(emobj)
    , sectionid_(secid)
    , selector_(sel)
    , startrow_(startrow)
    , nrrows_(nrrows)
    , startcol_(startcol)
    , nrcols_(nrcols)
{ emobj_.ref(); }


EMObjectSelectionRemoval::~EMObjectSelectionRemoval()
{ emobj_.unRef(); }


od_int64 EMObjectSelectionRemoval::nrIterations() const
{
    int nriteration = ( nrrows_%16 ? (int)(nrrows_/16) + 1 : (int)(nrrows_/16) )
     	* ( nrcols_%16 ? (int)(nrcols_/16) +1 : (int)(nrcols_/16) );
  
    return nriteration;
}


bool EMObjectSelectionRemoval::doWork( od_int64 start, od_int64 stop, 
				       int threadid )
{
    const int nrhorgroupboxes = nrcols_%16 ? (int)(nrcols_/16) + 1
					   : (int)(nrcols_/16);
    const int nrvertgroupboxes = nrrows_%16 ? (int)(nrrows_/16) + 1
					    : (int)(nrrows_/16);

    TypeSet<EM::SubID>   localremovelist;

    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	const int boxrow = (int)(idx/nrhorgroupboxes);
	const int boxcol = idx%nrhorgroupboxes;

	const Geometry::Element* ge = emobj_.sectionGeometry( sectionid_ );
	if ( !ge ) continue;

	mDynamicCastGet(const Geometry::ParametricSurface*,surface,ge);
	if ( !surface ) continue;

	int rowstep = surface->rowRange().step;
	int colstep = surface->colRange().step;

	int versft = boxrow ? 1 : 0;
	int horsft = boxcol ? 1 : 0;

	const StepInterval<int> rowrg( startrow_ + rowstep*boxrow*16 + versft,
		startrow_ + rowstep * (((boxrow+1)==nrvertgroupboxes) ?
		nrrows_ -1 : (boxrow+1)*16), rowstep );
	const StepInterval<int> colrg( startcol_ + colstep*boxcol*16 + horsft, 
		startcol_ + colstep * (((boxcol+1)==nrhorgroupboxes) ?
		nrcols_ - 1 : (boxcol+1)*16), colstep );

	TypeSet<EM::SubID> ids;

	HorSampling horsampling( true );
	horsampling.set( rowrg, colrg );
	HorSamplingIterator iter( horsampling );
	BinID bid;

	Coord3 startcrd = Coord3::udf();
	Coord3 stopcrd = Coord3::udf();

	while ( iter.next(bid) )
	{
	    const Coord3 crd = emobj_.getPos( sectionid_, bid.getSerialized() );
	    if ( !crd.isDefined() )
		continue;

	    if ( mIsUdf(startcrd.x) || startcrd.x>crd.x ) startcrd.x = crd.x;
	    if ( mIsUdf(startcrd.y) || startcrd.y>crd.y ) startcrd.y = crd.y;
	    if ( mIsUdf(startcrd.z) || startcrd.z>crd.z ) startcrd.z = crd.z;
	    if ( mIsUdf(stopcrd.x)  || crd.x>stopcrd.x  ) stopcrd.x  = crd.x;
	    if ( mIsUdf(stopcrd.y)  || crd.y>stopcrd.y  ) stopcrd.y  = crd.y;
	    if ( mIsUdf(stopcrd.z)  || crd.z>stopcrd.z  ) stopcrd.z  = crd.z;
	}

	if ( mIsUdf(startcrd.z) || mIsUdf(stopcrd.z) )
	    continue;

	const int sel = !selector_.canDoRange() ? 1 :
			selector_.includesRange( startcrd, stopcrd );

	if ( sel==0 || sel==3 )
	    continue;		// all outside or all behind projection plane

	iter.reset();
	while ( iter.next(bid) )
	{
	    if ( sel != 2 )	// not all inside
	    {
		const Coord3 crd = emobj_.getPos( sectionid_,
						  bid.getSerialized() );

		if ( !crd.isDefined() || !selector_.includes(crd) )
		    continue;
	    }

	    ids += bid.getSerialized();
	}

	localremovelist.append( ids );
    }	

    mutex_.lock();
    removelist_.append( localremovelist );
    mutex_.unLock();

    return true;
}

} // EM

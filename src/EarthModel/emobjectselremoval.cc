
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		May 2008
 RCS:		$Id: emobjectselremoval.cc,v 1.4 2009-08-21 04:29:36 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emobjectselremoval.h"
#include "parametricsurface.h"
#include "emobject.h"

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

	const StepInterval<int> rowrg( startrow_ + rowstep * boxrow * 16,
		startrow_ + rowstep * (((boxrow+1)==nrvertgroupboxes) ?
		nrrows_ -1 : (boxrow+1)*16), rowstep );
	const StepInterval<int> colrg( startcol_ + colstep* boxcol * 16, 
		startcol_ + colstep * (((boxcol+1)==nrhorgroupboxes) ?
		nrcols_ - 1 : (boxcol+1)*16), colstep );

	HorSampling horsampling( true );
	horsampling.set( rowrg, colrg );

	TypeSet<EM::SubID> ids;
	TypeSet<Coord3> positions;
	Coord3 startcord, stopcord;

	HorSamplingIterator iter( horsampling );
	BinID bid;
	int count = 0;
	while ( iter.next(bid) )
	{
	    const Coord3 examcor = emobj_.getPos( sectionid_, 
		    				  bid.getSerialized() );
	    if ( !examcor.isDefined() )
		continue;

	    if ( !count )
	    {
		startcord = stopcord = examcor;
		count++;
		continue;
	    }

	    positions += examcor;
	    ids += bid.getSerialized();
	    if ( examcor.x < startcord.x ) startcord.x = examcor.x;
	    if ( examcor.y < startcord.x ) startcord.y = examcor.y;
	    if ( examcor.z < startcord.z ) startcord.z = examcor.z;
	    if ( examcor.x > stopcord.x ) stopcord.x = examcor.x;
	    if ( examcor.y > stopcord.y ) stopcord.y = examcor.y;
	    if ( examcor.z > stopcord.z ) stopcord.z = examcor.z;
	}

	int sel = selector_.canDoRange() 
	    ? selector_.includesRange( startcord, stopcord )
	    : 1 ;

	if ( !sel ) continue; // all outside

	if ( sel == 1 )
	{
	    for ( int idy=ids.size()-1; idy>=0; idy-- )
	    {
		if ( !selector_.includes(positions[idy]) )
		{
		    ids.remove( idy );
		    positions.remove( idy );
		}
	    }
	}

	localremovelist.append( ids );
    }	

    mutex_.lock();
    removelist_.append( localremovelist );
    mutex_.unLock();

    return true;
}

} // EM

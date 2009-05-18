
/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		May 2008
 RCS:		$Id: emobjectselremoval.cc,v 1.1 2009-05-18 09:19:08 cvsumesh Exp $
________________________________________________________________________

-*/

#include "emobjectselremoval.h"
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
    int nrthreads = ( nrrows_%16 ? (int)(nrrows_/16) : (int)(nrrows_/16) + 1 )
     	* ( nrcols_%16 ? (int)(nrcols_/16) : (int)(nrcols_/16) + 1 );
  
    return nrthreads;
}


bool EMObjectSelectionRemoval::doWork( od_int64 start, od_int64 stop, 
				       int threadid )
{
    const int nrhorgroupboxes = nrcols_%16 ? (int)(nrcols_/16) 
					: (int)(nrcols_/16) + 1;
    const int nrvertgroupboxes = nrrows_%16 ? (int)(nrrows_/16) 
					    : (int)(nrrows_/16) + 1;

    TypeSet<EM::SubID>   localremovelist;

    for ( int idx=start; idx<=stop; idx++, addToNrDone(1) )
    {
	if ( !shouldContinue() )
	    return false;

	const int boxrow = idx ? (idx%nrhorgroupboxes ? 
		(int)(idx/nrhorgroupboxes): (int)(idx/nrhorgroupboxes)-1)
	    		       : 1;
	const int boxcol = idx ? (idx%nrhorgroupboxes ? nrvertgroupboxes - 1
				  : idx%nrhorgroupboxes - 1)
	    		       : 1;

	const Interval<int> rowrg( startrow_ + boxrow * 16, startrow_ + ( 
				   ((boxrow+1)==nrvertgroupboxes) ? nrrows_ -1 
				   : (boxrow+1)*16 - 1) );
	const Interval<int> colrg( startcol_ + boxcol * 16, startcol_ + (
				   ((boxcol+1)==nrhorgroupboxes) ? nrcols_ - 1
				   : (boxcol+1)*16 -1 ) );

	HorSampling horsampling( true );
	horsampling.set( rowrg, colrg );

	TypeSet<EM::SubID> ids;
	TypeSet<Coord3> positions;
	Coord3 startcord, stopcord;

	HorSamplingIterator iter( horsampling );
	BinID bid;
	while ( iter.next(bid) )
	{
	    const Coord3 examcor = emobj_.getPos( sectionid_, 
		    				  bid.getSerialized() );
	    positions += examcor;
	    ids += bid.getSerialized();
	    if ( examcor < startcord ) startcord = examcor;
	    if ( examcor > stopcord ) stopcord = examcor;
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

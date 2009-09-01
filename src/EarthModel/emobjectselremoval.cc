
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		May 2008
 RCS:		$Id: emobjectselremoval.cc,v 1.6 2009-09-01 05:08:01 cvsumesh Exp $
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
	
	Coord3 startcord, stopcord;
	startcord.z = SI().zRange(true).start;
	stopcord.z = SI().zRange(true).stop;

	Coord coord0 = SI().transform( BinID(rowrg.start,colrg.start) );
	startcord.x = stopcord.x = coord0.x;
	startcord.y = stopcord.y = coord0.y;

	Coord coord1 = SI().transform( BinID(rowrg.start,colrg.stop) );
	if ( startcord.x > coord1.x ) startcord.x = coord1.x;
	if ( startcord.y > coord1.y ) startcord.y = coord1.y;
	if ( coord1.x > stopcord.x ) stopcord.x = coord1.x;
	if ( coord1.y > stopcord.y ) stopcord.y = coord1.y;

	Coord coord2 = SI().transform( BinID(rowrg.stop,colrg.start) );
	if ( startcord.x > coord2.x ) startcord.x = coord2.x;
	if ( startcord.y > coord2.y ) startcord.y = coord2.y;
	if ( coord2.x > stopcord.x ) stopcord.x = coord2.x;
	if ( coord2.y > stopcord.y ) stopcord.y = coord2.y;

	Coord coord3 = SI().transform( BinID(rowrg.stop,colrg.stop) );
	if ( startcord.x > coord3.x ) startcord.x = coord3.x;
	if ( startcord.y > coord3.y ) startcord.y = coord3.y;
	if ( coord3.x > stopcord.x ) stopcord.x = coord3.x;
	if ( coord3.y > stopcord.y ) stopcord.y = coord3.y;

	int sel = selector_.canDoRange()
	    ? selector_.includesRange( startcord, stopcord )
	    : 1;

	if ( !sel ) continue; // all outside

	HorSampling horsampling( true );
	horsampling.set( rowrg, colrg );

	HorSamplingIterator iter( horsampling );
	BinID bid;
	while ( iter.next(bid) )
	{
	    const Coord3 examcor = emobj_.getPos( sectionid_, 
		    				  bid.getSerialized() );
	    if ( !examcor.isDefined() )
		continue;

	    if ( sel == 1 && !selector_.includes(examcor) )
		continue;

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

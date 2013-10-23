#ifndef vishorthreadworks_h
#define vishorthreadworks_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
 RCS:		$Id$
________________________________________________________________________
-*/
// this header file only be used in the classes related to Horzonsection . 
// don't include it in somewhere else !!!


#include "threadwork.h"
#include "task.h"
#include "thread.h"
#include "ranges.h"

namespace osg { class CullStack; }
namespace Geometry { class BinIDSurface; }

class BinIDSurface;
class ZAxisTransform;


namespace visBase
{
    class HorizonSection;
    class HorizonSectionTile;


class HorizonTileRenderPreparer: public ParallelTask
{
public: 
    HorizonTileRenderPreparer( HorizonSection& hrsection, 
			       const osg::CullStack* cs, char res );
    
    ~HorizonTileRenderPreparer()
    { delete [] permutation_; }

    od_int64 nrIterations() const { return nrtiles_; }
    od_int64 totalNr() const { return nrtiles_ * 2; }
    const char* message() const { return "Updating Horizon Display"; }
    const char* nrDoneText() const { return "Parts completed"; }

    bool doPrepare( int );
    bool doWork( od_int64, od_int64, int );

    od_int64*			permutation_;
    HorizonSectionTile**	hrsectiontiles_;
    HorizonSection&		hrsection_;
    int				nrtiles_;
    int				nrcoltiles_;
    char			resolution_;
    int				nrthreads_;
    int				nrthreadsfinishedwithres_;
    Threads::Barrier		barrier_;
    const osg::CullStack*	cs_;
};


class TileTesselator : public SequentialTask
{
public:
    TileTesselator( HorizonSectionTile* tile, char res );

    int	nextStep();
    HorizonSectionTile*		tile_;
    char			res_;
    bool			doglue_;
};


class HorizonSectionTilePosSetup: public ParallelTask
{
public:    
    HorizonSectionTilePosSetup(ObjectSet<HorizonSectionTile> tiles, 
	const Geometry::BinIDSurface& geo,
	StepInterval<int> rrg, StepInterval<int> crg, ZAxisTransform* zat,
	int ssz, char lowresidx);

    ~HorizonSectionTilePosSetup();

    od_int64	nrIterations() const { return hrtiles_.size(); }
    const char*	message() const { return "Creating Horizon Display"; }
    const char*	nrDoneText() const { return "Parts completed"; }

protected:

    bool doWork(od_int64, od_int64, int);

    int					nrcrdspertileside_;
    char				lowestresidx_;
    ObjectSet<HorizonSectionTile> 	hrtiles_;
    const Geometry::BinIDSurface&	geo_;
    StepInterval<int>			rrg_, crg_;
    ZAxisTransform*			zaxistransform_;
};


class TileGlueTesselator : public SequentialTask
{
public:
    TileGlueTesselator( HorizonSectionTile* tile );

    int	nextStep();
    HorizonSectionTile*		tile_;
};


}

#endif


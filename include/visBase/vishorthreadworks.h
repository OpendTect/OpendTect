#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
________________________________________________________________________
-*/
// this header file only be used in the classes related to Horzonsection .
// don't include it in somewhere else !!!

#include "vishorizonsectiondef.h"
#include "threadwork.h"
#include "paralleltask.h"
#include "rowcol.h"
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
/*!
\brief HorizonTileResolutionTesselator class is an independent usage for
tesselating coordinates, normals and primitive set of horizon tiles. it is
specifically for exporting horizon to Pdf3D in which the exported horizon has
no relation with the existing displayed horizon in coordinates, normals and
primitive set. thus we can export horizon into different resolution without
influence current displayed horizon in the secne.
note: the class doesn't do anything with texture.
*/

mExpClass(visBase) HorizonTileResolutionTesselator: public ParallelTask
{ mODTextTranslationClass(HorizonTileResolutionTesselator);
public:
    HorizonTileResolutionTesselator(const HorizonSection* hrsection,char res);
    ~HorizonTileResolutionTesselator();
    od_int64		    nrIterations() const { return nrtiles_; }
    uiString		    message() const
			    { return tr("Tessellating horizon"); }
    uiString		    nrDoneText() const
			    { return tr("Parts completed"); }

    bool		    doPrepare(int);
    bool		    doWork(od_int64,od_int64,int);

    bool		    getTitleCoordinates(int,TypeSet<Coord3>&) const;
    bool		    getTitleNormals(int,TypeSet<Coord3f>&) const;
    bool		    getTitlePrimitiveSet(int,TypeSet<int>&,
						 GeometryType) const;

private:
    bool		    createTiles();
    ObjectSet<HorizonSectionTile>   hrtiles_;
    const HorizonSection*	horsection_;
    int				nrtiles_;
    char			resolution_;
};


class HorizonTileRenderPreparer: public ParallelTask
{ mODTextTranslationClass(HorizonTileRenderPreparer);
public:
    HorizonTileRenderPreparer( HorizonSection& hrsection,
			       const osg::CullStack* cs, char res );

    ~HorizonTileRenderPreparer()
    { delete [] permutation_; }

    od_int64 nrIterations() const { return nrtiles_; }
    od_int64 totalNr() const { return nrtiles_ * 2; }
    uiString message() const { return tr("Updating Horizon Display"); }
    uiString nrDoneText() const { return tr("Parts completed"); }

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
    const osg::CullStack*	tkzs_;
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
{ mODTextTranslationClass(HorizonSectionTilePosSetup);
public:
		HorizonSectionTilePosSetup(const TypeSet<RowCol>& tiles,
			const TypeSet<RowCol>& indexes,
			HorizonSection* horsection,
			const StepInterval<int>& rrg,
			const StepInterval<int>& crg);

		~HorizonSectionTilePosSetup();

    od_int64	nrIterations() const { return hrtiles_.size(); }
    uiString	message() const { return tr("Creating Horizon Display"); }
    uiString	nrDoneText() const { return tr("Parts completed"); }
    void	setTesselationResolution(char res);

protected:

    bool doWork(od_int64, od_int64, int);
    bool doFinish(bool);

    int					nrcrdspertileside_;
    char				resolution_;
    const TypeSet<RowCol>&		hrtiles_;
    const TypeSet<RowCol>&		indexes_;
    const Geometry::BinIDSurface*	geo_;
    StepInterval<int>			rrg_, crg_;
    ZAxisTransform*			zaxistransform_;
    HorizonSection*			horsection_;
    Threads::Lock			lock_;
};


class TileGlueTesselator : public SequentialTask
{
public:
    TileGlueTesselator( HorizonSectionTile* tile );

    int	nextStep();
    HorizonSectionTile*		tile_;
};

} // namespace visBase

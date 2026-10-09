#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

// this header file only be used in the classes related to Horzonsection .
// don't include it in somewhere else !!!

#include "visbasemod.h"

#include "paralleltask.h"
#include "ranges.h"
#include "rowcol.h"
#include "thread.h"
#include "vishorizonsectiondef.h"
#include "zaxistransform.h"

namespace osg { class CullStack; }
namespace Geometry { class BinIDSurface; }

class BinIDSurface;


namespace visBase
{
    class HorizonSection;
    class HorizonSectionTile;
    class HorTileSampleCache;

/*!
\brief HorizonTileResolutionTesselator class is an independent usage for
tesselating coordinates, normals and primitive set of horizon tiles. it is
specifically for exporting horizon to Pdf3D in which the exported horizon has
no relation with the existing displayed horizon in coordinates, normals and
primitive set. thus we can export horizon into different resolution without
influence current displayed horizon in the secne.
note: the class doesn't do anything with texture.
*/

mExpClass(visBase) HorizonTileResolutionTesselator : public ParallelTask
{ mODTextTranslationClass(HorizonTileResolutionTesselator);
public:
			HorizonTileResolutionTesselator(const HorizonSection*,
							char res);
			~HorizonTileResolutionTesselator();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

    bool		getTileCoordinates(int,TypeSet<Coord3>&) const;
    bool		getTileNormals(int,TypeSet<Coord3>&) const;
    bool		getTilePrimitiveSet(int,TypeSet<int>&,
					    GeometryType) const;

private:

    od_int64		nrIterations() const override { return nrtiles_; }

    bool		createTiles();
    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;
    bool		doFinish(bool) override;

    ObjectSet<HorizonSectionTile>   hrtiles_;
    const HorizonSection*	horsection_;
    int				nrtiles_	= 0;
    char			resolution_;
    HorTileSampleCache*		cache_		= nullptr;
};


class HorizonTileRenderPreparer: public ParallelTask
{ mODTextTranslationClass(HorizonTileRenderPreparer);
public:
			HorizonTileRenderPreparer(HorizonSection&,
				const osg::CullStack*,char res);
			~HorizonTileRenderPreparer();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

private:

    od_int64		nrIterations() const override { return nrtiles_; }
    od_int64		totalNr() const override { return nrtiles_ * 2; }

    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;
    bool		doFinish(bool) override;

    od_int64*			permutation_	= nullptr;
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


class TileTesselator final : public SequentialTask
{
public:
				TileTesselator(HorizonSectionTile*,char res);

    HorizonSectionTile*		tile_;
    char			res_;
    bool			doglue_;

private:
    od_int64			totalNr() const override;
    int				nextStep() override;
};


class HorizonSectionTilePosSetup: public ParallelTask
{ mODTextTranslationClass(HorizonSectionTilePosSetup);
public:
			HorizonSectionTilePosSetup(TypeSet<RowCol>& tiles,
						   TypeSet<RowCol>& indexes,
						   HorizonSection*,
						   StepInterval<int>rrg,
						   StepInterval<int>crg);
			~HorizonSectionTilePosSetup();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override;

    void		setTesselationResolution(char res);

private:
    od_int64		nrIterations() const override;

    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;
    bool		doFinish(bool) override;

    int					nrcrdspertileside_;
    char				resolution_;
    const Geometry::BinIDSurface*	geo_		= nullptr;
    StepInterval<int>			rrg_, crg_;
    RefMan<ZAxisTransform>		zaxistransform_;
    HorizonSection*			horsection_;
    TypeSet<RowCol>&			hortiles_;
    TypeSet<RowCol>&			indexes_;
    HorTileSampleCache*			cache_		= nullptr;
};


class TileGlueTesselator : public SequentialTask
{
public:
				TileGlueTesselator(HorizonSectionTile*);

private:

    int				nextStep() override;
    HorizonSectionTile*		tile_;
};

} // namespace visBase

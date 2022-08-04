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

#include "visbasemod.h"
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
			HorizonTileResolutionTesselator(const HorizonSection*,
							char res);
			~HorizonTileResolutionTesselator();

    od_int64		nrIterations() const override { return nrtiles_; }
    uiString		uiMessage() const override
			{ return tr("Tessellating horizon"); }
    uiString		uiNrDoneText() const override
			{ return tr("Parts completed"); }

    bool		doPrepare(int) override;
    bool		doWork(od_int64,od_int64,int) override;

    bool		getTileCoordinates(int,TypeSet<Coord3>&) const;
    bool		getTileNormals(int,TypeSet<Coord3>&) const;
    bool		getTilePrimitiveSet(int,TypeSet<int>&,
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

    od_int64 nrIterations() const override { return nrtiles_; }
    od_int64 totalNr() const override { return nrtiles_ * 2; }
    uiString uiMessage() const override
			 { return tr("Updating Horizon Display"); }
    uiString uiNrDoneText() const override { return tr("Parts completed"); }

    bool doPrepare(int) override;
    bool doWork(od_int64,od_int64,int) override;

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
				TileTesselator(HorizonSectionTile*,char res);

    int				nextStep() override;
    HorizonSectionTile*		tile_;
    char			res_;
    bool			doglue_;
};


class HorizonSectionTilePosSetup: public ParallelTask
{ mODTextTranslationClass(HorizonSectionTilePosSetup);
public:
    HorizonSectionTilePosSetup(TypeSet<RowCol>& tiles,TypeSet<RowCol>& indexes,
	HorizonSection* horsection,StepInterval<int>rrg,StepInterval<int>crg );

    ~HorizonSectionTilePosSetup();

    od_int64	nrIterations() const override;
    uiString	uiMessage() const override
		{ return tr("Creating Horizon Display"); }
    uiString	uiNrDoneText() const override
		{ return tr("Parts completed"); }
    void	setTesselationResolution(char res);


protected:

    bool		doWork(od_int64,od_int64,int) override;
    bool		doFinish(bool) override;

    int					nrcrdspertileside_;
    char				resolution_;
    const Geometry::BinIDSurface*	geo_;
    StepInterval<int>			rrg_, crg_;
    ZAxisTransform*			zaxistransform_;
    HorizonSection*			horsection_;
    Threads::Lock			lock_;
    TypeSet<RowCol>&			hortiles_;
    TypeSet<RowCol>&			indexes_;
};


class TileGlueTesselator : public SequentialTask
{
public:
				TileGlueTesselator(HorizonSectionTile*);

    int				nextStep() override;
    HorizonSectionTile*		tile_;
};

} // namespace visBase

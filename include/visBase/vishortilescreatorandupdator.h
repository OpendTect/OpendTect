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
#include "refcount.h"

#include "geomelement.h"
#include "rowcol.h"
#include "thread.h"
#include "typeset.h"

class TaskRunner;

namespace osg { class CullStack; }
namespace osgGeo { class LayeredTexture; }

namespace visBase
{
class HorizonSection;
class HorizonSectionTile;


class HorTilesCreatorAndUpdator : public ReferencedObject
{
public:
			HorTilesCreatorAndUpdator(HorizonSection*);

    void		updateTiles(const TypeSet<GeomPosID>*,TaskRunner*);
    void		createAllTiles(TaskRunner*);
    void		updateTilesAutoResolution(const osg::CullStack*);
    void		updateTilesPrimitiveSets();
    void		setFixedResolution(char res,TaskRunner*);


protected:
    virtual		~HorTilesCreatorAndUpdator();

private:
    void		updateTileArray(const StepInterval<int>& rrg,
					const StepInterval<int>& crg);
    HorizonSectionTile* createOneTile(int tilerowidx, int tilecolidx);
    void		setNeighbors(HorizonSectionTile* tile, int tilerowidx,
				     int tilecolidx);


    HorizonSection*	horsection_;
    Threads::SpinLock	spinlock_;
    Threads::Mutex	updatelock_;
};

} // namespace visBase

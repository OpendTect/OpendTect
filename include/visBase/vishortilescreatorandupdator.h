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

#include "visbasecommon.h"
#include "typeset.h"
#include "geomelement.h"
#include "rowcol.h"
#include "refcount.h"
#include "thread.h"

namespace osg { class CullStack; }
namespace osgGeo { class LayeredTexture; }


namespace visBase
{

class HorizonSection;
class HorizonSectionTile;


class HorTilesCreatorAndUpdator : public RefCount::Referenced
{
public:
            HorTilesCreatorAndUpdator(HorizonSection*);

    void    updateTiles(const TypeSet<GeomPosID>*,TaskRunner*);
    void    createAllTiles(TaskRunner* tskr);
    void    updateTilesAutoResolution(const osg::CullStack* cs);
    void    updateTilesPrimitiveSets();
    void    setFixedResolution(char res, TaskRunner* tskr);


protected:

            ~HorTilesCreatorAndUpdator();
    void    updateTileArray(const StepInterval<int>& rrg,
			    const StepInterval<int>& crg);
    HorizonSectionTile* createOneTile(int tilerowidx, int tilecolidx);
    void    setNeighbors(HorizonSectionTile* tile, int tilerowidx,
			 int tilecolidx);

    HorizonSection*	horsection_;
    Threads::SpinLock	spinlock_;
    Threads::Mutex	updatelock_;

};



}

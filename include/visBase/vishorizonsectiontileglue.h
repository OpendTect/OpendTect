#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

// this header file only be used in the classes related to Horizonsection . 
// don't include it in somewhere else !!!

#include "typeset.h"
#include "thread.h"
#include "visdata.h"


namespace osg
{
    class Geometry;
    class Geode;
    class Array;
    class DrawElementsUShort;
}


namespace visBase
{
    class HorizonSectionTile;
    class Coordinates;
 
class HorizonSectionTileGlue
{
public:
    HorizonSectionTileGlue();
    ~HorizonSectionTileGlue();
    void			buildGlue(HorizonSectionTile*,
					  HorizonSectionTile*,bool);
    osg::Geode*			getGeode() { return gluegeode_; }
    void			setDisplayTransformation(const mVisTrans*);

protected:
    visBase::Coordinates*	gluevtexarr_;
    visBase::Coordinates*	gluenormalarr_;
    std::vector<osg::Array*>	gluetxcoords_;
    osg::Geode*			gluegeode_;
    osg::Geometry*		gluegeom_;
    osg::DrawElementsUShort*	glueps_;
    osg::DrawElementsUShort*	glueosgps_;

    Threads::Mutex		datalock_;

    const mVisTrans*		transformation_;

    void			buildOsgGeometry();
    void			addGlueTrianglePrimitiveSet(TypeSet<int>&);
    void			removeGlue();
    void			setNrTexCoordLayers(int);
};

} // namespace visBase

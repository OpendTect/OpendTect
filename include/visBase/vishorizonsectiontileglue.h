#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2009
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

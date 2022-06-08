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

#include "typeset.h"
#include "vishorizonsectiontile.h"
#include "vistransform.h"
#include "zaxistransform.h"

#include "thread.h"

namespace osg
{
    class Array;
    class DrawElementsUShort;
    class Geode;
    class Geometry;
    class StateSet;
    class UserDataContainer;
}

namespace osgUtil { class CullVisitor; }

namespace osgGeo { class LayeredTexture; }

namespace visBase
{

class Coordinates;
class HorizonSectionTile;
class TextureChannels;


class TileResolutionData
{
public:
			TileResolutionData(const HorizonSectionTile*,
					   char resolution);
			~TileResolutionData();

    void		setTexture(const unsigned int unit,osg::Array* arr,
				   osg::StateSet* stateset);
    void		enableGeometryTypeDisplay(GeometryType,bool yn);

    bool		tesselateResolution(bool onlyifabsness);
    void		updatePrimitiveSets();
    void		setWireframeColor(OD::Color&);
    void		dirtyGeometry();
    const osg::PrimitiveSet*	getPrimitiveSet(GeometryType) const;
    void		setLineWidth(int);

protected:

    friend class HorizonSectionTile;
    friend class HorizonSectionTileGlue;

    const HorizonSectionTile*	sectile_;
    osg::Switch*		osgswitch_;
    osg::UserDataContainer*	geodes_;

    osg::Array*			osgvertices_;
    const osg::Array*		normals_;
    osg::Array*			linecolor_;

    osg::DrawElementsUShort*	trianglesps_		= nullptr;
    osg::DrawElementsUShort*	linesps_		= nullptr;
    osg::DrawElementsUShort*	pointsps_		= nullptr;
    osg::DrawElementsUShort*	wireframesps_		= nullptr;

    osg::DrawElementsUShort*	trianglesosgps_		= nullptr;
    osg::DrawElementsUShort*	linesosgps_		= nullptr;
    osg::DrawElementsUShort*	pointsosgps_		= nullptr;
    osg::DrawElementsUShort*	wireframesosgps_	= nullptr;

    Threads::Mutex		tesselatemutex_;

    bool			updateprimitiveset_	= true;
    char			needsretesselation_	= cMustRetesselate;
    char			resolution_;
    int				nrverticesperside_	=cNumberNodePerTileSide;
    bool			needsetposition_	= true;
    int				dispgeometrytype_	= Triangle;

private:

    void			buildOsgGeometres();
    void			setPrimitiveSet(unsigned int,
						osg::DrawElementsUShort*);

    void			tesselateCell(int row, int col);
    void			refOsgPrimitiveSets();
    void			unRefOsgPrimitiveSets();
    void			createPrimitiveSets();
    void			buildLineGeometry(int idx, int width);
    void			buildTraingleGeometry(int idx);
    void			buildPointGeometry(int idx);
    void			hideFromDisplay();
    bool			detectIsolatedLine(int crdidx,char direction);
    void			setGeometryTexture(const unsigned int unit,
						   const osg::Array* arr,
						   osg::StateSet*stateset,
						   int geometrytype);
    void			dirtyGeometry(int type);
};

} // namespace visBase


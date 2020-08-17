#pragma once

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


#include "typeset.h"
#include "vistransform.h"
#include "zaxistransform.h"
#include "vishorizonsectiontile.h"

#include "thread.h"

namespace osg
{
    class Geometry;
    class Geode;
    class UserDataContainer;
    class Array;
    class StateSet;
    class DrawElementsUShort;
}

namespace osgUtil { class CullVisitor; }

namespace osgGeo { class LayeredTexture; }

namespace visBase
{
    class HorizonSectionTile;
    class TextureChannels;
    class Coordinates;

class TileResolutionData
{
public:
			TileResolutionData(const HorizonSectionTile*,
					   char resolution);
			~TileResolutionData();

    void		setTexture(const unsigned int unit,osg::Array* arr,
				   osg::StateSet* stateset);
    void		enableGeometryTypeDisplay(GeometryType type,bool yn);

    bool		tesselateResolution(bool onlyifabsness);
    void		updatePrimitiveSets();
    void		setWireframeColor(Color& color);
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

    osg::DrawElementsUShort*	trianglesps_;
    osg::DrawElementsUShort*	linesps_;
    osg::DrawElementsUShort*	pointsps_;
    osg::DrawElementsUShort*	wireframesps_;

    osg::DrawElementsUShort*	trianglesosgps_;
    osg::DrawElementsUShort*	linesosgps_;
    osg::DrawElementsUShort*	pointsosgps_;
    osg::DrawElementsUShort*	wireframesosgps_;

    Threads::Mutex		tesselatemutex_;

    bool			updateprimitiveset_;
    char			needsretesselation_;
    char			resolution_;
    int				nrverticesperside_;
    bool			needsetposition_;
    int				dispgeometrytype_;

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


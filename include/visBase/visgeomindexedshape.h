#ifndef visgeomindexedshape_h
#define visgeomindexedshape_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "valseries.h"
#include "visobject.h"
#include "coltabsequence.h"
#include "coltabmapper.h"
#include "visshape.h"
#include "draw.h"

namespace Geometry { class IndexedGeometry; }
class TaskRunner;
class DataPointSet;

namespace visBase
{
class Transformation;
class Coordinates;
class Normals;
class TextureCoords;
class ForegroundLifter;
class VertexShape;
class TextureChannels;

/*!Visualisation for Geometry::IndexedShape. */

mExpClass(visBase) GeomIndexedShape : public VisualObjectImpl
{
public:
    static GeomIndexedShape*	create()
				mCreateDataObj(GeomIndexedShape);

    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			setSurface(Geometry::IndexedShape*,
	    				   TaskRunner* = 0);
    				//!<Does not become mine, should remain
				//!<in memory

    bool			touch(bool forall,bool createnew=true,
				      TaskRunner* =0);

    void			renderOneSide(int side);
    				/*!< 0 = visible from both sides.
				     1 = visible from positive side
				    -1 = visible from negative side. */

    void			setLineStyle(const LineStyle&);
				/*!<for polylin3d, only the radius is used.*/

    void			enableColTab(bool);
    bool			isColTabEnabled() const;
    void			setDataMapper(const ColTab::MapperSetup&,
	    				      TaskRunner*);
    const ColTab::MapperSetup*	getDataMapper() const;
    void			setDataSequence(const ColTab::Sequence&);
    const ColTab::Sequence*	getDataSequence() const;

    void			getAttribPositions(DataPointSet&,
					mVisTrans* extratrans,
	    				TaskRunner*) const;
    void			setAttribData(const DataPointSet&,
	    				TaskRunner*);

    void			setMaterial(Material*);
    void			updateMaterialFrom(const Material*);

    void			setPrimitiveType(
				const Geometry::PrimitiveSet::PrimitiveType);

    enum			GeomShapeType{ Triangle, PolyLine, PolyLine3D };
    void			setIndexedGeometryShapeType(
						  GeomShapeType geomshapetype);
    void			useOsgNormal(bool);
    void			setNormalBindType(VertexShape::BindType);
    void			setColorBindType(VertexShape::BindType);
    void			addNodeState(visBase::NodeState*);

    void			setTextureChannels(TextureChannels*);

protected:
				~GeomIndexedShape();
    void			reClip();
    void			mapAttributeToColorTableMaterial();
    void			matChangeCB(CallBacker*);
    void			updateGeometryMaterial();

    mExpClass(visBase)			ColorHandler
    {
    public:
					ColorHandler();
					~ColorHandler();
	ColTab::Mapper			mapper_;
	ColTab::Sequence                sequence_;
	visBase::Material*		material_;
	ArrayValueSeries<float,float>	attributecache_;
    };

    static const char*			sKeyCoordIndex() { return "CoordIndex";}

    ColorHandler*				colorhandler_;

    Geometry::IndexedShape*			shape_;
    VertexShape*				vtexshape_;
    bool					colortableenabled_ ;
    int						renderside_;
       					    /*!< 0 = visisble from both sides.
					       1 = visible from positive side
					      -1 = visible from negative side.*/
    Material*					singlematerial_;
    Material*					coltabmaterial_;
    ColTab::Sequence		                sequence_;
    Geometry::PrimitiveSet::PrimitiveType	primitivesettype_;
    GeomShapeType				geomshapetype_;

    LineStyle					linestyle_;
    bool					useosgnormal_;
};

};
	
#endif


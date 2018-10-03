#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		September 2007
________________________________________________________________________

-*/

#include "visshape.h"
#include "visobject.h"
#include "draw.h"

namespace Geometry { class IndexedGeometry; }
class DataPointSet;
class TaskRunnerProvider;

namespace visBase
{
class DrawStyle;
class Transformation;
class Coordinates;
class Normals;
class TextureCoords;
class ForegroundLifter;
class VertexShape;
class TextureChannels;

/*!Visualization for Geometry::IndexedShape. */

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

    void			setRenderMode(RenderMode);

    void			setLineStyle(const OD::LineStyle&);
				/*!<for polylin3d, only the radius is used.*/

    void			enableColTab(bool);
    bool			isColTabEnabled() const;
    void			setColTabMapper(const ColTab::Mapper&);
    const ColTab::Mapper&	getColTabMapper() const;
    void			setColTabSequence(const ColTab::Sequence&);
    const ColTab::Sequence&	getColTabSequence() const;

    void			getAttribPositions(DataPointSet&,
					mVisTrans* extratrans,
					TaskRunner*) const;
    void			setAttribData(const DataPointSet&,
					TaskRunner*);
    void			setAttribData(const DataPointSet&,
					const TaskRunnerProvider&);

    void			setMaterial(Material*);
    void			updateMaterialFrom(const Material*);

    enum			GeomShapeType{ Triangle, PolyLine, PolyLine3D };
    void			setGeometryShapeType(GeomShapeType shapetype,
				 Geometry::PrimitiveSet::PrimitiveType pstype
				 = Geometry::PrimitiveSet::TriangleStrip);
				/*!<remove previous geometry shape and create
				a new geometry shape based on shape type.*/

    void			useOsgNormal(bool);
    void			setNormalBindType(VertexShape::BindType);
    void			setColorBindType(VertexShape::BindType);
    void			addNodeState(visBase::NodeState*);

    void			setTextureChannels(TextureChannels*);
    virtual void	        setPixelDensity(float);

    VertexShape*		getVertexShape() const { return vtexshape_; }

protected:

				~GeomIndexedShape();
    void			mapAttributeToColorTableMaterial();
    void			matChangeCB(CallBacker*);
    void			mappingChgCB(CallBacker*);
    void			updateGeometryMaterial();
    void			updateColors();

    mExpClass(visBase)			ColorHandler
    {
    public:

					ColorHandler();

	ConstRefMan<ColTab::Mapper>	mapper_;
	ConstRefMan<ColTab::Sequence>	sequence_;
	RefMan<visBase::Material>	material_;
	TypeSet<float>			attributecache_;

    };

    static const char*			sKeyCoordIndex() { return "CoordIndex";}

    ColorHandler			colorhandler_;
    Geometry::IndexedShape*		shape_;
    VertexShape*			vtexshape_;
    bool				colortableenabled_ ;
    int					renderside_;
					    /*!< 0 = visisble from both sides.
					       1 = visible from positive side
					      -1 = visible from negative side.*/
    RefMan<Material>			singlematerial_;
    RefMan<Material>			coltabmaterial_;
    GeomShapeType			geomshapetype_;

    OD::LineStyle			linestyle_;
    bool				useosgnormal_;
};

};


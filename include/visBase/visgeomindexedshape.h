#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "coltabmapper.h"
#include "coltabsequence.h"
#include "draw.h"
#include "valseriesimpl.h"
#include "visshape.h"

namespace Geometry { class IndexedGeometry; }
class TaskRunner;
class DataPointSet;

namespace visBase
{
class Coordinates;
class DrawStyle;
class ForegroundLifter;
class Normals;
class TextureChannels;
class TextureCoords;
class Transformation;

/*!Visualization for Geometry::IndexedShape. */

mExpClass(visBase) GeomIndexedShape : public VisualObjectImpl
{
public:
    static RefMan<GeomIndexedShape> create();
				mCreateDataObj(GeomIndexedShape);

    void			setDisplayTransformation(
						const mVisTrans*) override;
    const mVisTrans*		getDisplayTransformation() const override;

    void			setSurface(Geometry::IndexedShape*,
					   TaskRunner* =nullptr);
				//!<Does not become mine, should remain
				//!<in memory

    bool			touch(bool forall,bool createnew=true,
				      TaskRunner* =nullptr);

    void			setRenderMode(RenderMode);

    void			setLineStyle(const OD::LineStyle&);
				/*!<for polylin3d, only the radius is used.*/

    void			enableColTab(bool);
    bool			isColTabEnabled() const;
    void			setDataMapper(const ColTab::MapperSetup&,
					      TaskRunner*);
    const ColTab::MapperSetup*	getDataMapper() const;
    void			setDataSequence(const ColTab::Sequence&);
    const ColTab::Sequence*	getDataSequence() const;

    bool			getAttribPositions(DataPointSet&,
					mVisTrans* extratrans,
					TaskRunner*) const;
    void			setAttribData(const DataPointSet&,
					TaskRunner*);

    void			setMaterial(Material*) override;
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
    void			setPixelDensity(float) override;

    VertexShape*		getVertexShape();
    const VertexShape*		getVertexShape() const;

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
	RefMan<visBase::Material>	material_;
	ArrayValueSeries<float,float>	attributecache_;
    };

    static const char*			sKeyCoordIndex() { return "CoordIndex";}

    ColorHandler*			colorhandler_;
    bool				colortableenabled_ = false;

    Geometry::IndexedShape*		shape_ = nullptr;
    RefMan<VertexShape>			vtexshape_;
    int					renderside_;
				    /*!< 0 = visisble from both sides.
				       1 = visible from positive side
				      -1 = visible from negative side.*/
    RefMan<Material>			singlematerial_;
    RefMan<Material>			coltabmaterial_;
    ColTab::Sequence			sequence_;
    GeomShapeType			geomshapetype_ =GeomShapeType::Triangle;

    OD::LineStyle			linestyle_;
    bool				useosgnormal_ = false;
};

} // namespace visBase

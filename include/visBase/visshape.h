#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "visbasemod.h"

#include "draw.h"
#include "indexedshape.h"
#include "visobject.h"

namespace osg
{
    class Geode;
    class Geometry;
    class PrimitiveSet;
    class StateAttribute;
    class Switch;
}

namespace visBase
{

class NodeState;
class ForegroundLifter;
class VisColorTab;
class Material;
class Coordinates;
class Normals;
class TextureChannels;
class TextureCoords;


mExpClass(visBase) Shape : public VisualObject
{
public:

    inline const Material*	getMaterial() const	{ return material_; }
    inline Material*		getMaterial() override	{ return material_; }
    void			setMaterial(Material*) override;

    void			setMaterialBinding(int);
    static int			cOverallMaterialBinding()	{ return 0; }
    static int			cPerVertexMaterialBinding()	{ return 2; }

    int				getMaterialBinding() const;

    void			setRenderMode(RenderMode);
    void			enableRenderLighting(bool);
    //! osg default render lighting is on

    int				usePar(const IOPar&);
    void			fillPar(IOPar&) const;

protected:
				Shape();
				~Shape();

    RefMan<Material>		material_;

    Material*			gtMaterial();
    const Material*		gtMaterial() const;

    static const char*		sKeyOnOff();
    static const char*		sKeyTexture();
    static const char*		sKeyMaterial();
};


mExpClass(visBase) VertexShape : public Shape
{
    class TextureCallbackHandler;
    class NodeCallbackHandler;

public:

    static RefMan<VertexShape> create();
			mCreateDataObj(VertexShape);

    void		setPrimitiveType(Geometry::PrimitiveSet::PrimitiveType);
			//!<Should be called before adding statesets
    void		setNormals(Normals*);
    void		setTextureCoords(TextureCoords*);

    Normals*		getNormals()		{ return gtNormals(); }
    const Normals*	getNormals() const	{ return gtNormals(); }
    TextureCoords*	getTextureCoords()	{ return gtTextureCoords(); }
    const TextureCoords* getTextureCoords() const { return gtTextureCoords(); }

    virtual  void	  setCoordinates(Coordinates*);
    virtual  Coordinates* getCoordinates() { return coords_; }
    virtual  const Coordinates*   getCoordinates() const { return coords_; }
    virtual  void	  setLineStyle(const OD::LineStyle&){};

    void		removeSwitch();
			/*!<Will turn the object permanently on.
			 \note Must be done before giving away the
			 SoNode with getInventorNode() to take
			 effect. */

    void		setDisplayTransformation(const mVisTrans*) override;
			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates,
			     you will have to setTransformation again.	*/
    const mVisTrans*	getDisplayTransformation() const override;
			/*!<\note Direcly relayed to the coordinates */

    void		dirtyCoordinates();

    void		addPrimitiveSet(Geometry::PrimitiveSet*);
    void		removePrimitiveSet(const Geometry::PrimitiveSet*);
    void		removeAllPrimitiveSets();
    int			nrPrimitiveSets() const;
    virtual void	touchPrimitiveSet(int)			{}
    Geometry::PrimitiveSet*	getPrimitiveSet(int);
    const Geometry::PrimitiveSet* getPrimitiveSet(int) const;
    void		setMaterial(Material*) override;
    void		materialChangeCB(CallBacker*);
    void		coordinatesChangedCB(CallBacker*);
    void		enableCoordinatesChangedCB(bool yn)
			{ usecoordinateschangedcb_ = yn; }
    void		useOsgAutoNormalComputation(bool);
    bool		useOsgAutoNormalComputation() const
			    { return useosgsmoothnormal_; }
    Coord3		getOsgNormal(int) const;

    enum		BindType{ BIND_OFF = 0,BIND_OVERALL,
				       BIND_PER_PRIMITIVE_SET,
				       BIND_PER_PRIMITIVE, BIND_PER_VERTEX};
    void		setColorBindType(BindType);
    int			getNormalBindType();
    void		setNormalBindType(BindType);
    void		updatePartialGeometry(Interval<int>);
    void		useVertexBufferRender(bool yn);
			/*!<If yn=true, osg use vertex buffer to render and
			    ignore displaylist false, osg use display list to
			    render.*/

    void		setTextureChannels(TextureChannels*);
    const unsigned char*  getTextureData(int&,int&,int&) const;
    void		forceRedraw(bool=true);
    void		setAttribAndMode(osg::StateAttribute*);

protected:
			VertexShape(Geometry::PrimitiveSet::PrimitiveType,
				    bool creategeode);
			~VertexShape();

    void		setupOsgNode();

    virtual void	addPrimitiveSetToScene(osg::PrimitiveSet*);
    virtual void	removePrimitiveSetFromScene(const osg::PrimitiveSet*);

    Normals*		gtNormals();
    const Normals*	gtNormals() const;
    TextureCoords*	gtTextureCoords();
    const TextureCoords* gtTextureCoords() const;

    void		setUpdateVar(bool& var,bool yn);
			//!<Will trigger redraw request if necessary

    bool		needstextureupdate_ = false;
			// Only set via setUpdateVar(.)

    RefMan<Normals>	normals_;
    RefMan<Coordinates> coords_;
    RefMan<TextureCoords> texturecoords_;

    osg::Node*		node_ = nullptr;
    osg::Geode*		geode_;
    osg::Geometry*	osggeom_ = nullptr;

    bool		useosgsmoothnormal_ = false;
    bool		usecoordinateschangedcb_ = true;

    BindType		colorbindtype_;
    BindType		normalbindtype_;

    RefMan<TextureChannels>	channels_;
    NodeCallbackHandler*	nodecallbackhandler_ = nullptr;
    TextureCallbackHandler*	texturecallbackhandler_ = nullptr;
    Threads::Lock		redrawlock_;
    bool			isredrawing_ = false;

    Geometry::PrimitiveSet::PrimitiveType	primitivetype_;

    Threads::Lock				lock_;
						/*!<lock protects primitiveset
						and osg color array*/
    RefObjectSet<Geometry::PrimitiveSet>	primitivesets_;

};



class PrimitiveSetCreator : public Geometry::PrimitiveSetCreator
{
    Geometry::PrimitiveSet* doCreate(bool,bool) override;
};

} // namespace visBase

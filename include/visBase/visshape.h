#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "indexedshape.h"
#include "draw.h"

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

    Material*			material_;

    Material*			gtMaterial() const;

    static const char*		sKeyOnOff();
    static const char*		sKeyTexture();
    static const char*		sKeyMaterial();
};


#undef mDeclSetGetItem
#define mDeclSetGetItem( ownclass, clssname, variable ) \
protected: \
    clssname*		   gt##clssname() const; \
public: \
    inline clssname*	   get##clssname()	 { return gt##clssname(); } \
    inline const clssname* get##clssname() const { return gt##clssname(); } \
    void		   set##clssname(clssname*)

mExpClass(visBase) VertexShape : public Shape
{
    class TextureCallbackHandler;
    class NodeCallbackHandler;

public:

    static VertexShape* create()
			mCreateDataObj(VertexShape);

    void		setPrimitiveType(Geometry::PrimitiveSet::PrimitiveType);
			//!<Should be called before adding statesets

    mDeclSetGetItem( VertexShape, Normals, normals_ );
    mDeclSetGetItem( VertexShape, TextureCoords, texturecoords_ );

    virtual  void	  setCoordinates(Coordinates* coords);
    virtual  Coordinates* getCoordinates() { return coords_; }
    virtual  const Coordinates*   getCoordinates() const { return coords_; }
    virtual  void	  setLineStyle(const OD::LineStyle&){};

    void		removeSwitch();
			/*!<Will turn the object permanently on.
			 \note Must be done before giving away the
			 SoNode with getInventorNode() to take
			 effect. */

    virtual void	setDisplayTransformation( const mVisTrans* ) override;
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
    void		setMaterial( Material* mt ) override;
    void		materialChangeCB( CallBacker*  );
    void		coordinatesChangedCB( CallBacker* );
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
			VertexShape( Geometry::PrimitiveSet::PrimitiveType,
				     bool creategeode );
			~VertexShape();

    void		setupOsgNode();

    virtual void	addPrimitiveSetToScene(osg::PrimitiveSet*);
    virtual void	removePrimitiveSetFromScene(const osg::PrimitiveSet*);

    void		setUpdateVar(bool& var,bool yn);
			//!<Will trigger redraw request if necessary

    bool		needstextureupdate_;	// Only set via setUpdateVar(.)

    Normals*		normals_;
    Coordinates*	coords_;
    TextureCoords*	texturecoords_;

    osg::Node*		node_;

    osg::Geode*		geode_;
    osg::Geometry*	osggeom_;

    bool		useosgsmoothnormal_;
    bool		usecoordinateschangedcb_;

    BindType		colorbindtype_;
    BindType		normalbindtype_;

    RefMan<TextureChannels>	channels_;
    NodeCallbackHandler*	nodecallbackhandler_;
    TextureCallbackHandler*	texturecallbackhandler_;
    Threads::Lock		redrawlock_;
    bool			isredrawing_;

    Geometry::PrimitiveSet::PrimitiveType	primitivetype_;

    Threads::Lock				lock_;
						/*!<lock protects primitiveset
						and osg color array*/
    ObjectSet<Geometry::PrimitiveSet>		primitivesets_;

};

#undef mDeclSetGetItem



class PrimitiveSetCreator : public Geometry::PrimitiveSetCreator
{
    Geometry::PrimitiveSet* doCreate(bool,bool) override;
};



}


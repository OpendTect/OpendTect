#ifndef visshape_h
#define visshape_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visshape.h,v 1.18 2006-08-16 10:51:19 cvsbert Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class SoIndexedShape;
class SoMaterialBinding;
class SoNormalBinding;
class SoSeparator;
class SoShape;
class SoShapeHints;
class SoSwitch;
class SoVertexShape;

namespace visBase
{
class VisColorTab;
class Material;
class Texture2;
class Texture3;
class Coordinates;
class Normals;
class TextureCoords;


#undef mDeclSetGetItem
#define mDeclSetGetItem( ownclass, clssname, variable ) \
protected: \
    clssname*		   gt##clssname() const; \
public: \
    inline clssname*	   get##clssname()	 { return gt##clssname(); } \
    inline const clssname* get##clssname() const { return gt##clssname(); } \
    void		   set##clssname(clssname*)


class Shape : public VisualObject
{
public:
    void			turnOn(bool);
    bool			isOn() const;
    void			removeSwitch();
    				/*!<Will turn the object permanently on.
				    \note Must be done before giving away the
				    SoNode with getInventorNode() to take
				    effect. */

    void			setRenderCache(int mode);
				    //!<\param mode=0 off, 1=on, 2=auto (deflt)
    int				getRenderCache() const;

    mDeclSetGetItem( Shape,	Texture2, texture2_ );
    mDeclSetGetItem( Shape,	Texture3, texture3_ );
    mDeclSetGetItem( Shape,	Material, material_ );

    void			setMaterialBinding( int );
    				/*!< 0 = Overall (default)
				     1 = Per face
				     2 = Per vertex */
    int				getMaterialBinding() const;

    int				usePar(const IOPar&);
    void			fillPar(IOPar&,TypeSet<int>&) const;

    SoNode*			getInventorNode();
    void			insertNode( SoNode* );
				    /*!< Inserts the node _before_ the shape */
    void			removeNode(SoNode*);
    virtual void		replaceShape(SoNode*);
    SoNode*			getShape() { return shape_; }

protected:
				Shape( SoNode* );
    virtual			~Shape();

    SoNode*			shape_;
    SoSwitch*			onoff_;

    Texture2*			texture2_;
    Texture3*			texture3_;
    Material*			material_;

private:
    static const char*		sKeyOnOff();
    static const char*		sKeyTexture();
    static const char*		sKeyMaterial();

    SoSeparator*		root_;
    SoMaterialBinding*		materialbinding_;

};


class VertexShape : public Shape
{
public:

    mDeclSetGetItem( VertexShape, Coordinates, coords_ );
    mDeclSetGetItem( VertexShape, Normals, normals_ );
    mDeclSetGetItem( VertexShape, TextureCoords, texturecoords_ );


    void		setDisplayTransformation( Transformation* );
    			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates, 
			     you will have to setTransformation again.  */
    Transformation*	getDisplayTransformation();
    			/*!<\note Direcly relayed to the coordinates */

    void		setNormalPerFaceBinding( bool yn );
    			/*!< If yn==false, normals are set per vertex */
    bool		getNormalPerFaceBinding() const;
    			/*!< If yn==false, normals are set per vertex */

    void		setVertexOrdering(int vo);
    			/*!\param vo=0 clockwise
			   \param vo=1 counterclockwise
			   \param vo=2 unknown
		        */
    int			getVertexOrdering() const;

    void		setFaceType(int);
    			/*!< 0: unknown; 1: convex */
    int			getFaceType() const;
    			/*!< 0: unknown; 1: convex */
    void		setShapeType(int);
    			/*!< 0: unknown; 1: solid */
    int			getShapeType() const;
    			/*!< 0: unknown; 1: solid */

protected:
    			VertexShape( SoVertexShape* );
    			~VertexShape();

    Normals*		normals_;
    Coordinates*	coords_;
    TextureCoords*	texturecoords_;

private:
    SoNormalBinding*	normalbinding_;
    SoShapeHints*	shapehints_;
};

#undef mDeclSetGetItem


class IndexedShape : public VertexShape
{
public:
    int			nrCoordIndex() const;
    void		setCoordIndex(int pos, int idx );
    void		removeCoordIndexAfter(int);
    int			getCoordIndex(int) const;

    int			nrTextureCoordIndex() const;
    void		setTextureCoordIndex(int pos, int idx );
    void		removeTextureCoordIndexAfter(int);
    int			getTextureCoordIndex(int) const;

    int			nrNormalIndex() const;
    void		setNormalIndex( int pos, int idx );
    void		removeNormalIndexAfter(int);
    int			getNormalIndex(int) const;

    int			nrMaterialIndex() const;
    void		setMaterialIndex( int pos, int idx );
    void		removeMaterialIndexAfter(int);
    int			getMaterialIndex(int) const;


    int			getClosestCoordIndex(const EventInfo&) const;
    void		replaceShape(SoNode*);

protected:
    			IndexedShape( SoIndexedShape* );


private:

    SoIndexedShape*	indexedshape_;
};

};


#endif

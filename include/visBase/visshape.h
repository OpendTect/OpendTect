#ifndef visshape_h
#define visshape_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visshape.h,v 1.1 2003-01-07 10:29:56 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class SoIndexedShape;
class SoMaterialBinding;
class SoNormalBinding;
class SoSeparator;
class SoShape;
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

/*!\brief


*/


class Shape : public SceneObject
{
public:
    				Shape( SoShape* );

    void			setTexture2( visBase::Texture2* );
    visBase::Texture2*		getTexture2();

    void			setTexture3( visBase::Texture3* );
    visBase::Texture3*		getTexture3();

    void			setMaterial( visBase::Material* );
    visBase::Material*		getMaterial();
    void			setMaterialBinding( int );
    				/*!< 0 = Overall (default)
				     1 = Per face
				     2 = Per vertex
				 */
    int				getMaterialBinding() const;

    SoNode*			getData();

protected:
    virtual			~Shape();
    void			addNode( SoNode* );
    void			removeNode( SoNode* );

    SoShape*			shape;

    visBase::Texture2*		texture2;
    visBase::Texture3*		texture3;
    visBase::Material*		material;

private:
    SoSeparator*		root;
    SoMaterialBinding*		materialbinding;
};


class VertexShape : public Shape
{
public:
    				VertexShape( SoVertexShape* );
    void			setCoordinates( visBase::Coordinates* );
    visBase::Coordinates*	getCoordinates();

    void			setTextureCoords(visBase::TextureCoords*);
    visBase::TextureCoords*	getTextureCoords();

    void			setNormals( visBase::Normals* );
    visBase::Normals*		getNormals();
    void			setNormalPerFaceBinding( bool yn );
    				/*!< If yn==false, normals are set per vertex */
    bool			getNormalPerFaceBinding() const;
    				/*!< If yn==false, normals are set per vertex */
protected:
    				~VertexShape();

    visBase::Normals*		normals;
    visBase::Coordinates*	coords;
    visBase::TextureCoords*	texturecoords;

private:
    SoNormalBinding*		normalbinding;
};


class IndexedShape : public VertexShape
{
public:
    			IndexedShape( SoIndexedShape* );

    int			nrCoordIndex() const;
    void		setCoordIndex(int pos, int idx );
    int			getCoordIndex(int) const;

    int			nrTextureCoordIndex() const;
    void		setTextureCoordIndex(int pos, int idx );
    int			getTextureCoordIndex(int) const;

    int			nrNormalIndex() const;
    void		setNormalIndex( int pos, int idx );
    int			getNormalIndex(int) const;

    int			nrMaterialIndex() const;
    void		setMaterialIndex( int pos, int idx );
    int			getMaterialIndex(int) const;

private:
    SoIndexedShape*	indexedshape;
};

};


#endif

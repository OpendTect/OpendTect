#ifndef visshape_h
#define visshape_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visshape.h,v 1.2 2003-01-20 08:33:09 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"

class SoIndexedShape;
class SoMaterialBinding;
class SoNormalBinding;
class SoSeparator;
class SoShape;
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

/*!\brief


*/

class Shape : public VisualObject
{
public:
				Shape( SoShape* );
    void			turnOn(bool);
    bool			isOn() const;

    void			setTexture2( Texture2* );
    Texture2*			getTexture2();

    void			setTexture3( Texture3* );
    Texture3*			getTexture3();

    void			setMaterial( Material* );
    Material*			getMaterial();
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
    SoSwitch*			onoff;

    Texture2*			texture2;
    Texture3*			texture3;
    Material*			material;

private:
    SoSeparator*		root;
    SoMaterialBinding*		materialbinding;
};


class VertexShape : public Shape
{
public:
    			VertexShape( SoVertexShape* );
    void		setCoordinates( Coordinates* );
    Coordinates*	getCoordinates();

    void		setTransformation( Transformation* );
    			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates, 
			     you will have to setTransformation again.
			 */
    Transformation*	getTransformation();
    			/*!<\note Direcly relayed to the coordinates */

    void		setTextureCoords(TextureCoords*);
    TextureCoords*	getTextureCoords();

    void		setNormals( Normals* );
    Normals*		getNormals();
    void		setNormalPerFaceBinding( bool yn );
    			/*!< If yn==false, normals are set per vertex */
    bool		getNormalPerFaceBinding() const;
    			/*!< If yn==false, normals are set per vertex */
protected:
    			~VertexShape();

    Normals*		normals;
    Coordinates*	coords;
    TextureCoords*	texturecoords;

private:
    SoNormalBinding*	normalbinding;
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

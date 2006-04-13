#ifndef visshape_h
#define visshape_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visshape.h,v 1.15 2006-04-13 15:29:13 cvskris Exp $
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

/*!\brief


*/

class Shape : public VisualObject
{
public:
				Shape( SoNode* );

    void			turnOn(bool);
    bool			isOn() const;
    void			removeSwitch();
    				/*!<Will turn the object permanently on.
				    \note Must be done before giving away the
				    SoNode with getInventorNode() to take
				    effect. */

    void			setRenderCache(int mode);
    				/*!<\param mode=0 off
				    \param mode=1 on
				    \param mode=2 auto (default)
				*/
    int				getRenderCache() const;

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

    int				usePar( const IOPar& );
    void			fillPar( IOPar&, TypeSet<int>& ) const;

    SoNode*			getInventorNode();

protected:
    virtual			~Shape();
    void			insertNode( SoNode* );
    				/*!< Inserts the node _before_ the shape */
    void			removeNode( SoNode* );

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
    			VertexShape( SoVertexShape* );
    void		setCoordinates( Coordinates* );
    Coordinates*	getCoordinates();
    const Coordinates*	getCoordinates() const;

    void		setDisplayTransformation( Transformation* );
    			/*!<\note The transformation is forwarded to the
			     the coordinates, if you change coordinates, 
			     you will have to setTransformation again.
			 */
    Transformation*	getDisplayTransformation();
    			/*!<\note Direcly relayed to the coordinates */

    void		setTextureCoords(TextureCoords*);
    TextureCoords*	getTextureCoords();

    void		setNormals( Normals* );
    Normals*		getNormals();
    void		setNormalPerFaceBinding( bool yn );
    			/*!< If yn==false, normals are set per vertex */
    bool		getNormalPerFaceBinding() const;
    			/*!< If yn==false, normals are set per vertex */

    void		setVertexOrdering(int);
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
    			~VertexShape();

    Normals*		normals_;
    Coordinates*	coords_;
    TextureCoords*	texturecoords_;

private:
    SoNormalBinding*	normalbinding_;
    SoShapeHints*	shapehints_;
};


class IndexedShape : public VertexShape
{
public:
    			IndexedShape( SoIndexedShape* );

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

private:

    SoIndexedShape*	indexedshape_;
};

};


#endif

#ifndef vistexture3viewer_h
#define vistexture3viewer_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id: vistexture3viewer.h,v 1.9 2002-12-03 13:17:17 kristofer Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "sets.h"

class SoRotation;
class SoTexture2;
class SoTexture3;
class SoCoordinate3;
class SoFaceSet;
class SoTranslateRectangleDragger;
class SoSensor;
class SoFieldSensor;
class SoGroup;
class SoTextureCoordinate3;
class SoDragger;

namespace visBase
{

class Texture3ViewerObject : public VisualObjectImpl
{
public:
    				Texture3ViewerObject( bool sel=true )
				    : VisualObjectImpl( sel ) {}

    virtual void		setTexture( SoTexture3* ) = 0;

    virtual float		position() const = 0;
};


/*!\brief
Is a viewer for 3d textures. Any number of slices, in either x, y or z
direction can be cut through the texture. The positions of the object in
3d is always -1,-1,-1 to 1,1,1. If you want to have it larger, place a scale
in front of it.
*/

class Texture3Viewer : public VisualObjectImpl
{
public:
    static Texture3Viewer*	create()
				mCreateDataObj(Texture3Viewer);

    int			addSlice(int dim,float origpos=0);
    float		slicePosition(int);

    int			getNrObjects() const;
    void		removeObject(int);

    void		showObject(int,bool yn);
    bool		isObjectShown(int) const;

    void		setTexture( SoTexture3* );

protected:
			~Texture3Viewer();
    SoTexture3*         texture;

    ObjectSet<Texture3ViewerObject>	textureobjects;
};


class Texture3Slice : public Texture3ViewerObject
{
public:
    static Texture3Slice*	create()
			mCreateDataObj(Texture3Slice);

    int			dim() const;
    void		setDim( int );
    float 		position() const;
    void		setPosition( float );

    void		setTexture( SoTexture3* );
protected:
				~Texture3Slice();
    void			setUpCoords();

    SoCoordinate3*		coords;
    SoTexture3*			texture;
    SoTextureCoordinate3*	texturecoords;
    SoFaceSet*			faces;
    int				dim_;
    float			pos;
};


class MovableTextureSlice : public Texture3ViewerObject
{
public:
    static MovableTextureSlice*	create()
			mCreateDataObj(MovableTextureSlice);

    int			dim() const;
    void		setDim( int );
    float 		position() const;
    void		setPosition( float );

    void		setTexture( SoTexture3* );

    Notifier<MovableTextureSlice> motion;
protected:
				~MovableTextureSlice();

    static void			fieldsensorCB( void*, SoSensor* );
    SoFieldSensor*		fieldsensor;

    static void			startCB(void*,SoDragger*);
    static void			motionCB(void*,SoDragger*);

    SoRotation*			rotation;
    SoGroup*			group;
    SoTextureCoordinate3*	texturecoords;
    SoTranslateRectangleDragger* dragger;
    int				dim_;
};


class MovableTexture2Slice : public Texture3ViewerObject
{
public:
    static MovableTexture2Slice*	create()
			mCreateDataObj(MovableTexture2Slice);

    int			dim() const;
    void		setDim( int );
    float 		position() const;
    void		setPosition( float );

    void		setTexture( SoTexture3* );
protected:
				~MovableTexture2Slice();

    static void			fieldsensorCB( void*, SoSensor* );
    SoFieldSensor*		fieldsensor;

    SoRotation*			rotation;
    SoGroup*			group;
    SoTranslateRectangleDragger* dragger;
    SoTexture2*			texture2;
    SoTexture3*			texture3;
    int				dim_;
};

};
	
#endif

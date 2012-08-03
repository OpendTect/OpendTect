#ifndef vistexture3viewer_h
#define vistexture3viewer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id: vistexture3viewer.h,v 1.16 2012-08-03 13:01:27 cvskris Exp $
________________________________________________________________________

-*/

#include "visbasemod.h"
#include "visobject.h"
#include "sets.h"

class SoRotation;
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
class Texture3;

mClass(visBase) Texture3ViewerObject : public VisualObjectImpl
{
public:
    				Texture3ViewerObject( bool sel=true )
				    : VisualObjectImpl( sel ) {}

    virtual void		setTexture( Texture3& ) = 0;

    virtual float		position() const = 0;
};


/*!\brief
Is a viewer for 3d textures. Any number of slices, in either x, y or z
direction can be cut through the texture. The positions of the object in
3d is always -1,-1,-1 to 1,1,1. If you want to have it larger, place a scale
in front of it.
*/

mClass(visBase) Texture3Viewer : public VisualObjectImpl
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

    void		setTexture( Texture3& );
    Texture3&		getTexture();

    virtual void	fillPar(IOPar&,TypeSet<int>&) const;
    virtual int		usePar(const IOPar&);

protected:
			~Texture3Viewer();
    Texture3*		texture;

    ObjectSet<Texture3ViewerObject>	textureobjects;

    static const char*	textureidstr();
};


mClass(visBase) Texture3Slice : public Texture3ViewerObject
{
public:
    static Texture3Slice*	create()
			mCreateDataObj(Texture3Slice);

    int			dim() const;
    void		setDim( int );
    float 		position() const;
    void		setPosition( float );

    void		setTexture( Texture3& );
protected:
				~Texture3Slice();
    void			setUpCoords();

    SoCoordinate3*		coords;
    Texture3*			texture;
    SoTextureCoordinate3*	texturecoords;
    SoFaceSet*			faces;
    int				dim_;
    float			pos;
};


mClass(visBase) MovableTextureSlice : public Texture3ViewerObject
{
public:
    static MovableTextureSlice*	create()
			mCreateDataObj(MovableTextureSlice);

    int			dim() const;
    void		setDim( int );
    float 		position() const;
    void		setPosition( float );

    void		setTexture( Texture3& );

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
    Texture3*			texture;
};


};
	
#endif


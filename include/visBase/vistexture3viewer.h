#ifndef vistexture3viewer_h
#define vistexture3viewer_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        K. Tingdahl
 Date:          November 2002
 RCS:           $Id: vistexture3viewer.h,v 1.1 2002-11-08 12:22:25 kristofer Exp $
________________________________________________________________________

-*/

#include "visobject.h"
#include "sets.h"

class SoRotation;
class SoTexture3;
class SoTranslateRectangleDragger;
class SoSensor;
class SoFieldSensor;
class SoGroup;
class SoTextureCoordinate3;

namespace visBase
{

class MovableTextureSlice;

class Texture3Viewer : public VisualObjectImpl
{
public:
    static Texture3Viewer*	create()
				mCreateDataObj0arg(Texture3Viewer);

    int			getNrSlices() const;
    int			addSlice(int dim,float origpos=0);
    void		removeSlice(int);

    void		showSlice(int,bool yn);
    bool		isSliceShown(int) const;

    void		setTexture( SoTexture3* );

protected:
			~Texture3Viewer();
    SoTexture3*         texture;

    ObjectSet<MovableTextureSlice>	textureobjects;
};


class MovableTextureSlice : public VisualObjectImpl
{
public:
    static MovableTextureSlice*	create()
			mCreateDataObj0arg(MovableTextureSlice);

    int			dim() const;
    void		setDim( int );
    float 		position() const;
    void		setPosition( float );

    void		setTexture( SoTexture3* );
protected:
				~MovableTextureSlice();

    static void			fieldsensorCB( void*, SoSensor* );
    SoFieldSensor*		fieldsensor;

    SoRotation*			rotation;
    SoGroup*			group;
    SoTextureCoordinate3*	texturecoords;
    SoTranslateRectangleDragger* dragger;
    int			dim_;
};

};
	
#endif

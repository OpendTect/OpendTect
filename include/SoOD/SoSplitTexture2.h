#ifndef SoSplitTexture2_h
#define SoSplitTexture2_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoSplitTexture2.h,v 1.3 2007-11-20 10:33:47 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoSFVec2i32.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

class SbVec2s;
class SoGLDisplayList;
class SoFieldSensor;
class SoSensor;
class SoGLImage;

/*!Adds a texture to the scene. The texture is however not sent to OpenGL,
   but is put on the state. Multiple SoSplitTexture2Part can then fetch
   (parts of) the texture from the state and send it to OpenGL. The class will
   tag each texture by the current texture unit.
*/


class SoSplitTexture2: public SoNode
{ SO_NODE_HEADER(SoSplitTexture2);
public:
    static		void initClass();
			SoSplitTexture2();

    SoSFImage		image;

protected:
    void		GLRender(SoGLRenderAction*);
    			~SoSplitTexture2();	
    static void		imageChangeCB(void*,SoSensor*);
};


/*!Fetches a texture that SoSplitTexture2 has put on the state and sends a part
   of it to OpenGL. Make sure to use the same texture unit as was used when the
   image was put on the state.  If the requested part is outside the source
   texture, it is clamped to edge.*/

class SoSplitTexture2Part: public SoNode
{ SO_NODE_HEADER(SoSplitTexture2Part);
public:
    static		void initClass();
			SoSplitTexture2Part();

    SoSFVec2i32		origin;
    SoSFVec2i32		size;

protected:
    void		GLRender(SoGLRenderAction*);
    void		callback(SoCallbackAction*);
    void		rayPick(SoRayPickAction*);
    void		doAction(SoAction*);
    			~SoSplitTexture2Part();	
    static void		fieldChangeCB(void*,SoSensor*);


    unsigned char*	imagedata_;
    int			imagesize_;
    int			numcomp_;
    SoGLImage*		glimage_;
    SoFieldSensor*	originsensor_;
    bool		needregeenration_;
};

#endif

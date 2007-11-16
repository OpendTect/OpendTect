#ifndef SoSplitTexture2_h
#define SoSplitTexture2_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoSplitTexture2.h,v 1.1 2007-11-16 21:39:05 cvskris Exp $
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


class SoSplitTexture2: public SoNode
{ SO_NODE_HEADER(SoSplitTexture2);
public:
    static		void initClass();
			SoSplitTexture2();

    SoSFImage		image;

protected:
    void		GLRender(SoGLRenderAction *action);
    			~SoSplitTexture2();	
    static void		imageChangeCB( void*, SoSensor* );
};


class SoSplitTexture2Part: public SoNode
{ SO_NODE_HEADER(SoSplitTexture2Part);
public:
    static		void initClass();
			SoSplitTexture2Part();

    SoSFVec2i32		origin;
    SoSFVec2i32		size;
    SoSFBool		borders;

protected:
    void		GLRender(SoGLRenderAction *action);
    			~SoSplitTexture2Part();	
    static void		fieldChangeCB( void*, SoSensor* );


    unsigned char*	imagedata_;
    int			imagesize_;
    SoGLImage*		glimage_;
    SoFieldSensor*	originsensor_;
    bool		needregeenration_;
};

#endif

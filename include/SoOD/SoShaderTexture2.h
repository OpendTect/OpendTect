#ifndef SoShaderTexture_h
#define SoShaderTexture_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoShaderTexture2.h,v 1.2 2007-01-17 07:39:05 cvsdgb Exp $
________________________________________________________________________


-*/

#include <Inventor/fields/SoSFImage.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/misc/SoGLImage.h>

class SbVec2s;
class SoGLDisplayList;
class SoFieldSensor;
class SoSensor;


class SoShaderTexture2: public SoNode
{ SO_NODE_HEADER(SoShaderTexture2);
public:
    static		void initClass();
			SoShaderTexture2();

    SoSFImage		image;

protected:
    void		GLRender(SoGLRenderAction *action);
    			~SoShaderTexture2();	
    static void		imageChangeCB( void*, SoSensor* );

    SoGLImage*	glimage_;
    SoFieldSensor*	imagesensor_;
    bool		glimagevalid_;
};

#endif

#ifndef SoShaderTexture_h
#define SoShaderTexture_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoShaderTexture2.h,v 1.4 2009-01-08 09:27:06 cvsranojay Exp $
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


mClass SoShaderTexture2: public SoNode
{ SO_NODE_HEADER(SoShaderTexture2);
public:
    static		void initClass();
			SoShaderTexture2();

    SoSFImage		image;

    static int		getMaxSize();
    			/*!<\returns the largest side size that the
			     hardware can handle. */

protected:
    void		GLRender(SoGLRenderAction *action);
    			~SoShaderTexture2();	
    static void		imageChangeCB( void*, SoSensor* );

    SoGLImage*	glimage_;
    SoFieldSensor*	imagesensor_;
    bool		glimagevalid_;
};

#endif

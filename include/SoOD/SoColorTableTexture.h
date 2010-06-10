#ifndef SoColorTableTexture_h
#define SoColorTableTexture_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Sep 2008
 RCS:           $Id: SoColorTableTexture.h,v 1.1 2010-06-10 09:35:26 cvsranojay Exp $
________________________________________________________________________


-*/

#include <Inventor/fields/SoSFImage.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

#include "soodbasic.h"

class SoFieldSensor;
class SoSensor;
class SoGLImage;
class SoElement;


//!Sends the texture to OpenGL, but sets the flags from SoTextureComposerElement


mClass SoColorTableTexture: public SoNode
{ SO_NODE_HEADER(SoColorTableTexture);
public:
    static		void initClass();
			SoColorTableTexture();

    SoSFImage		image;

protected:
    			~SoColorTableTexture();	

    void		GLRender(SoGLRenderAction*);
    void		callback(SoCallbackAction*);
    void		rayPick(SoRayPickAction*);
    void		doAction(SoAction*);
    static void		fieldChangeCB(void*, SoSensor*);

    SoElement*		matchinfo_;
    int			ti_;
    SoGLImage*		glimage_;
    SoFieldSensor*	sensor_;
    bool		needregeneration_;
};

#endif

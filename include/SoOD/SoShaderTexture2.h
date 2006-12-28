#ifndef SoShaderTexture_h
#define SoShaderTexture_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoShaderTexture2.h,v 1.1 2006-12-28 22:18:37 cvskris Exp $
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


/*!Extension of SoGLImage. Major difference is that it returns a custom
   display image that binds our texture and sets the appropriate wrapping. */


class SoGLShaderImage : public SoGLImage
{
public:
			SoGLShaderImage();
			~SoGLShaderImage();

    bool		setTexture(const unsigned char* bytes,
	    			const SbVec2s& size,
	    			int numcomponents,Wrap wrapS, Wrap wrapT, int unit );

    SoGLDisplayList*	getGLDisplayList(SoState * state);

private:
    SoGLDisplayList*	list_;
    unsigned int	texname_;
    SbVec2s		size_;
    int			nc_;
};



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

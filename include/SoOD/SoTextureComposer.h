#ifndef SoTextureComposer_h
#define SoTextureComposer_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoTextureComposer.h,v 1.4 2008-10-28 13:03:17 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/lists/SbPList.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoSFVec3i32.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFShort.h>
#include <Inventor/fields/SoMFUShort.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

class SoFieldSensor;
class SoSensor;
class SoGLImage;
class SoState;
class SoElement;
class SbImage;


/*!  */


class SoTextureComposer: public SoNode
{ SO_NODE_HEADER(SoTextureComposer);
public:
    static		void initClass();
			SoTextureComposer();

    SoSFShort		transparencyInfo;
    static char		cHasTransparency();
    static char		cHasNoTransparency();
    static char		cHasNoIntermediateTransparency();

    SoSFVec3i32		origin;
    SoSFVec3i32		size; //-1,-1,-1 means to the end of the channel

    static void		cutImage(const SbImage&,SbImage&,const SbVec3s& origin,
	    			 const SbVec3s& size);

protected:
    			~SoTextureComposer();	

    void		GLRender(SoGLRenderAction*);
    void		callback(SoCallbackAction*);
    void		rayPick(SoRayPickAction*);
    void		doAction(SoAction*);
    static void		fieldChangeCB(void*,SoSensor*);
    void		GLRenderUnit(int,SoState*,int);
    void		doActionUnit(int,SoState*);
    void		removeTextureData();

    struct TextureData {
				TextureData();
				~TextureData();
	unsigned char*		imagedata_;
	int			imagesize_;
	int			numcomp_;
	SoGLImage*		glimage_;
	char			ti_;
    };

    SbPList		texturedata_;

    SoFieldSensor*	originsensor_;
    bool		needregenration_;
    SoElement*		matchinfo_;
};

#endif

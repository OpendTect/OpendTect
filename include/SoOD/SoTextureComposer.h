#ifndef SoTextureComposer_h
#define SoTextureComposer_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Sep 2008
 RCS:           $Id: SoTextureComposer.h,v 1.11 2012-08-03 13:00:42 cvskris Exp $
________________________________________________________________________


-*/

#include "soodmod.h"
#include <Inventor/lists/SbPList.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/fields/SoSFVec3i32.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFShort.h>
#include <Inventor/fields/SoMFUShort.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

#include "soodbasic.h"

class SoFieldSensor;
class SoSensor;
class SoGLImage;
class SoState;
class SoElement;
class SbImage;


/*!Picks up SoTextureComposerSetElement's image and sends parts of it
   to OpenGL */


mClass(SoOD) SoTextureComposer: public SoNode
{ SO_NODE_HEADER(SoTextureComposer);
public:
    static		void initClass();
			SoTextureComposer();

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


mClass(SoOD) SoTextureComposerInfo : public SoNode
{ SO_NODE_HEADER(SoTextureComposerInfo );
public:
    static		void initClass();
			SoTextureComposerInfo();

    SoSFShort		transparencyInfo;
    static char		cHasTransparency();
    static char		cHasNoTransparency();
    static char		cHasNoIntermediateTransparency();

    SoMFUShort		units;

protected:
    void		GLRender(SoGLRenderAction*);
    void		callback(SoCallbackAction*);
    void		rayPick(SoRayPickAction*);
    void		doAction(SoAction*);
};

#endif


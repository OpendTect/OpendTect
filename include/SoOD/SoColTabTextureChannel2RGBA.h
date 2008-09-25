#ifndef SoColTabTextureChannel2RGBA_h
#define SoColTabTextureChannel2RGBA_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoColTabTextureChannel2RGBA.h,v 1.1 2008-09-25 18:38:16 cvskris Exp $
________________________________________________________________________


-*/


#include "SoMFImage.h"
#include <Inventor/fields/SoMFBool.h>
#include <Inventor/fields/SoMFShort.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

class SoFieldSensor;
class SoSensor;
class SoElement;
class SoState;

/*!
Reads any number of texture channels from the state, merges them with one
colorsequence per channel, and outputs 4 texture channels (RGBA) on the state.
*/


class SoColTabTextureChannel2RGBA : public SoNode
{ SO_NODE_HEADER(SoColTabTextureChannel2RGBA);
public:
    static		void initClass();
			SoColTabTextureChannel2RGBA();

    SoMFImage		colorsequences;
			/*!< Colorsequences for all images. The colorsequence
			     is an 1-dimensional image where the size in
			     dim0 and dim1 are 1. */
    SoMFBool		enabled;
    SoMFShort		opacity;
    			/*!< Specifies the maximum opacity of each image.
			     0 is completely transperant, 255 is completely
			     opaque. */


protected:
			~SoColTabTextureChannel2RGBA();
    void		sendRGBA(SoState*);
    bool		getTransparencyStatus( const SbImage* channels,
	    		    long size, int channelidx, bool& fullyopaque,
			    bool& fullytrans) const;
    void		computeRGBA( const SbImage* channels,
	    			     int start, int stop,
				     int firstchannel, int lastchannel );

    bool		needsregeneration_;
    SoFieldSensor*	sequencesensor_;
    SoFieldSensor*	opacitysensor_;
    SoFieldSensor*	enabledsensor_;

    SbImage		rgba_[4];
    SoElement*		matchinfo_;

    static void		fieldChangeCB( void* data, SoSensor* );

    void		GLRender(SoGLRenderAction*);
};

#endif

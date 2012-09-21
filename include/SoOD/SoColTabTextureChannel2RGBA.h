#ifndef SoColTabTextureChannel2RGBA_h
#define SoColTabTextureChannel2RGBA_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id$
________________________________________________________________________


-*/


#include "soodmod.h"
#include "SoMFImage.h"
#include <Inventor/fields/SoMFBool.h>
#include <Inventor/fields/SoMFShort.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

#include "soodbasic.h"

class SoFieldSensor;
class SoSensor;
class SoElement;
class SoState;

/*!
Reads any number of texture channels from the state, merges them with one
colorsequence per channel, and outputs 4 texture channels (RGBA) on the state.
*/


mSoODClass SoColTabTextureChannel2RGBA : public SoNode
{ SO_NODE_HEADER(SoColTabTextureChannel2RGBA);
public:
    static		void initClass();
			SoColTabTextureChannel2RGBA();

    SoMFImagei32	colorsequences;
			/*!< Colorsequences for all images. The colorsequence
			     is an 1-dimensional image where the size in
			     dim0 and dim1 are 1. */
    SoMFBool		enabled;
    SoMFShort		opacity;
    			/*!< Specifies the maximum opacity of each image.
			     0 is completely transperant, 255 is completely
			     opaque. */

    const SbImagei32&	getRGBA(int rgba) const;
			/*!<\param rgba is 0 for red, 1 for green, 2 for
			           blue and 3 for opacity. */

    void		processChannels( const SbImagei32* channels,
	    				 int nrchannels);
protected:
			~SoColTabTextureChannel2RGBA();
    void		sendRGBA(SoState*,const SbList<uint32_t>& dep);
    void		getTransparencyStatus( const SbImagei32* channels,
	    		    int size, int channelidx, char& fullyopaque,
			    char& fullytrans) const;
    void		computeRGBA( const SbImagei32* channels,
	    			     int start, int stop,
				     int firstchannel, int lastchannel );
    static void		fieldChangeCB( void* data, SoSensor* );

    void		GLRender(SoGLRenderAction*);

    bool					needsregeneration_;
    SoFieldSensor*				sequencesensor_;
    SoFieldSensor*				opacitysensor_;
    SoFieldSensor*				enabledsensor_;

    SbImagei32					rgba_[4];
    char					ti_;
    SoElement*					matchinfo_;

};

#endif


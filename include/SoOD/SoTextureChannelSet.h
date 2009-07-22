#ifndef SoTextureChannelSet_h
#define SoTextureChannelSet_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoTextureChannelSet.h,v 1.5 2009-07-22 16:01:19 cvsbert Exp $
________________________________________________________________________


-*/

#include <SoMFImage.h>
#include <Inventor/fields/SoMFBool.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

#include "soodbasic.h"

class SoFieldSensor;
class SoSensor;

/*!
*/


mClass SoTextureChannelSet : public SoNode
{ SO_NODE_HEADER(SoTextureChannelSet);
public:
    static		void initClass();
			SoTextureChannelSet();

    SoMFImage		channels;

protected:
    void		GLRender(SoGLRenderAction*);
    			~SoTextureChannelSet();	
};


#endif

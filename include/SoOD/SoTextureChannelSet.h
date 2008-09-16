#ifndef SoTextureChannelSet_h
#define SoTextureChannelSet_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          Dec 2006
 RCS:           $Id: SoTextureChannelSet.h,v 1.1 2008-09-16 16:17:01 cvskris Exp $
________________________________________________________________________


-*/

#include <SoMFImage.h>
#include <Inventor/fields/SoMFBool.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>

class SoFieldSensor;
class SoSensor;

/*!
*/


class SoTextureChannelSet : public SoNode
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

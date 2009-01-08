#ifndef SoCameraInfo_h
#define SoCameraInfo_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoCameraInfo.h,v 1.3 2009-01-08 09:27:06 cvsranojay Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoNode.h"
#include "Inventor/nodes/SoSubNode.h"

#include "Inventor/fields/SoSFInt32.h"

/*!\brief
Puts information about the camera/rendertype into the state.
*/

mClass SoCameraInfo : public SoNode
{
    SO_NODE_HEADER(SoCameraInfo);
    typedef SoNode inherited;
public:

    SoSFInt32		cameraInfo;

    enum cameraStatus	{
			    NORMAL=0x0000,
			    MOVING=0x0001,	/*!< The camera is moving
						     without user interaction */
			    INTERACTIVE=0x0002, /*!< The camera is moving
						     steered by user
						     interaction*/
    			    STEREO=0x0004	/*!< This is a second render
						     in a stereo render */
    			};
    
    static void		initClass();
    			SoCameraInfo();

protected:
    void		GLRender(SoGLRenderAction*);
};

#endif


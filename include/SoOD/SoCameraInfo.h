#ifndef SoCameraInfo_h
#define SoCameraInfo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoCameraInfo.h,v 1.9 2012-08-27 13:16:47 cvskris Exp $
________________________________________________________________________


-*/

#include "soodmod.h"
#include "Inventor/nodes/SoNode.h"
#include "Inventor/nodes/SoSubNode.h"

#include "Inventor/fields/SoSFInt32.h"

#include "soodbasic.h"

/*!\brief
Puts information about the camera/rendertype into the state.
*/

mSoODClass SoCameraInfo : public SoNode
{
    SO_NODE_HEADER(SoCameraInfo);
    typedef SoNode inherited;
public:

    SoSFInt32		cameraInfo;

    enum CameraStatus	{
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



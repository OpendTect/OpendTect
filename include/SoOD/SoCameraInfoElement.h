#ifndef SoCameraInfoElement_h
#define SoCameraInfoElement_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoCameraInfoElement.h,v 1.7 2009-06-24 18:04:09 cvskris Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoInt32Element.h>

#include "soodbasic.h"

/*!\brief
The means to transfer information about the camera to the scene.
*/

mClass SoCameraInfoElement : public SoInt32Element
{
    SO_ELEMENT_HEADER(SoCameraInfoElement);

public:
    static void		initClass();
    void		init(SoState*);
    static void		set(SoState*,SoNode*,COIN_INT32_T);
    			/*<!The integer is the bitwise combination of
			  SoCameraInfo::CameraStatus. */
    static int		get(SoState*);
    			/*<!\returns the bitwise combination of
			  SoCameraInfo::CameraStatus. */
    static int		getDefault();
    			/*<!\returns the bitwise combination of
			  SoCameraInfo::CameraStatus. */

private:
    			~SoCameraInfoElement();
};

#endif


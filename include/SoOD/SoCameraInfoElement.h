#ifndef SoCameraInfoElement_h
#define SoCameraInfoElement_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include <Inventor/elements/SoInt32Element.h>

#include "soodbasic.h"
#include "soodmod.h"


/*!\brief
The means to transfer information about the camera to the scene.
*/

mSoODClass SoCameraInfoElement : public SoInt32Element
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


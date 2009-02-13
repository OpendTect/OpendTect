#ifndef SoOD_h
#define SoOD_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoOD.h,v 1.7 2009-02-13 10:47:31 cvsnanne Exp $
________________________________________________________________________


-*/

#include "soodbasic.h"

/*!\brief A function that initiases the OpendTect Inventor classes. */

class SoOD
{
public:
mGlobal static int	supportsFragShading();
    			/*!<\retval -1 not supported
			    \retval  0 don't know
			    \retval  1 supported
			*/
			    	     
};

/*!\mainpage
SoOD cointains extensions to OpenInventor that are used to visualize 3D
objects.
*/

#endif

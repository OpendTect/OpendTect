#ifndef SoOD_h
#define SoOD_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoOD.h,v 1.10 2009-07-22 16:01:19 cvsbert Exp $
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
mGlobal static int	supportsVertexShading();
    			/*!<\retval -1 not supported
			    \retval  0 don't know
			    \retval  1 supported
			*/

mGlobal static int	maxNrTextureUnits();
			/*!<If not know, function will return 1.  */
			    	     
};

/*!\mainpage
SoOD cointains extensions to OpenInventor that are used to visualize 3D
objects.
*/

#endif

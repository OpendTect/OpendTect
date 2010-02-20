#ifndef SoOD_h
#define SoOD_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoOD.h,v 1.11 2010-02-20 00:58:42 cvskarthika Exp $
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

mGlobal static void	getLineWidthBounds( int& min, int& max );
			/*!<Get the bounds of the line width, which are 
			 * OpenGL-dependent. */

};

/*!\mainpage
SoOD cointains extensions to OpenInventor that are used to visualize 3D
objects.
*/

#endif

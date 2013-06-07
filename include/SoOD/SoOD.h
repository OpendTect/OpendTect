#ifndef SoOD_h
#define SoOD_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "soodbasic.h"

/*!\brief A function that initiases the OpendTect Inventor classes. */

class SoOD
{
public:

mGlobal static bool	getAllParams();
			/*!<Calls all of the below to make sure their
			    static variables are set. */

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
			/*!<If not known, function will return 1.  */

mGlobal static int	maxTexture2DSize();
			/*!<If not known, function will return 1024.
			    which is a safe default.*/

mGlobal static void	getLineWidthBounds( int& min, int& max );
			/*!<Get the bounds of the line width, which are 
			 * OpenGL-dependent. */

};


/*!\page SoOD Coin Extensions
SoOD cointains extensions to OpenInventor that are used to visualize 3D
objects.
*/

#endif

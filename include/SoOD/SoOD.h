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

#include "soodmod.h"
#include "soodbasic.h"

/*!
\ingroup SoOD
\brief A function that initiases the OpendTect Inventor classes. */

class SoOD
{
public:

mSoODGlobal static bool	getAllParams();
			/*!<Calls all of the below to make sure their
			    static variables are set. */

mSoODGlobal static int	supportsFragShading();
    			/*!<\retval -1 not supported
			    \retval  0 don't know
			    \retval  1 supported
			*/
mSoODGlobal static int	supportsVertexShading();
    			/*!<\retval -1 not supported
			    \retval  0 don't know
			    \retval  1 supported
			*/

mSoODGlobal static int	maxNrTextureUnits();
			/*!<If not known, function will return 1.  */

mSoODGlobal static int	maxTexture2DSize();
			/*!<If not known, function will return 1024.
			    which is a safe default.*/

mSoODGlobal static void	getLineWidthBounds( int& min, int& max );
			/*!<Get the bounds of the line width, which are 
			 * OpenGL-dependent. */

};

#endif


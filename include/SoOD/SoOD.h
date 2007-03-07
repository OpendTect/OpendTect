#ifndef SoOD_h
#define SoOD_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoOD.h,v 1.3 2007-03-07 16:08:48 cvskris Exp $
________________________________________________________________________


-*/

/*!\brief A function that initiases the OpendTect Inventor classes. */

class SoOD
{
public:
static void		init();
static int		supportsFragShading();
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

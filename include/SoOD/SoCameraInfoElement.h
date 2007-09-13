#ifndef SoCameraInfoElement_h
#define SoCameraInfoElement_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoCameraInfoElement.h,v 1.3 2007-09-13 19:35:49 cvsnanne Exp $
________________________________________________________________________


-*/

#include <Inventor/elements/SoInt32Element.h>

/*!\brief

*/

class SoCameraInfoElement : public SoInt32Element
{
    SO_ELEMENT_HEADER(SoCameraInfoElement);

public:
    static void		initClass();
    void		init(SoState*);
    static void		set( SoState*, SoNode*, COIN_INT32_T );
    static int		get( SoState* );
    static int		getDefault();

private:
    			~SoCameraInfoElement();
};

#endif


#ifndef vispickstyle_h
#define vispickstyle_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispickstyle.h,v 1.1 2004-01-05 09:43:47 kristofer Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoPickStyle;

namespace visBase
{

/*!\brief


*/

class PickStyle : public DataObject
{
public:
    static PickStyle*	create()
			mCreateDataObj(PickStyle);

    enum Style		{ Shape, BoundingBox, Unpickable };

    void		setStyle( Style );
    Style		getStyle() const;

    SoNode*		getInventorNode();
    int			usePar( const IOPar& );
    void		fillPar( IOPar&, TypeSet<int>& ) const;

protected:
			~PickStyle();

    SoPickStyle*	pickstyle;

    static const char*	stylestr;
};

}; // Namespace


#endif

#ifndef vispickstyle_h
#define vispickstyle_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispickstyle.h,v 1.2 2009-01-08 10:15:41 cvsranojay Exp $
________________________________________________________________________


-*/

#include "visdata.h"

class SoPickStyle;

namespace visBase
{

/*!\brief


*/

mClass PickStyle : public DataObject
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

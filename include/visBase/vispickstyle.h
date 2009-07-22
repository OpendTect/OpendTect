#ifndef vispickstyle_h
#define vispickstyle_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispickstyle.h,v 1.3 2009-07-22 16:01:24 cvsbert Exp $
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

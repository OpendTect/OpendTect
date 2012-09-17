#ifndef vispickstyle_h
#define vispickstyle_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispickstyle.h,v 1.4 2011/04/28 07:00:12 cvsbert Exp $
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

    int			usePar( const IOPar& );
    void		fillPar( IOPar&, TypeSet<int>& ) const;

protected:
			~PickStyle();

    SoPickStyle*	pickstyle;

    static const char*	stylestr;

    virtual SoNode*	gtInvntrNode();

};

}; // Namespace


#endif

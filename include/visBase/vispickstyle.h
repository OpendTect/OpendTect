#ifndef vispickstyle_h
#define vispickstyle_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vispickstyle.h,v 1.5 2012-08-03 13:01:25 cvskris Exp $
________________________________________________________________________


-*/

#include "visbasemod.h"
#include "visdata.h"

class SoPickStyle;

namespace visBase
{

/*!\brief


*/

mClass(visBase) PickStyle : public DataObject
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


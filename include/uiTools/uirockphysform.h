#ifndef uirockphysform_h
#define uirockphysform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id: uirockphysform.h,v 1.1 2012-02-03 13:00:59 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "propertyref.h"
class uiComboBox;


mClass uiRockPhysForm : public uiGroup
{
public:

			uiRockPhysForm(uiParent*);

    void		setType(PropertyRef::StdType);
    BufferString	getText() const;

protected:

    uiComboBox*		nmfld_;

    void		nameSel(CallBacker*);

};


#endif

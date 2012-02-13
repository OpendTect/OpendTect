#ifndef uirockphysform_h
#define uirockphysform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2011
 RCS:           $Id: uirockphysform.h,v 1.2 2012-02-13 12:49:11 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "propertyref.h"
class uiComboBox;


mClass uiRockPhysForm : public uiGroup
{
public:

			uiRockPhysForm(uiParent*);
			uiRockPhysForm(uiParent*,PropertyRef::StdType);

    PropertyRef::StdType getType() const;
    void		setType(PropertyRef::StdType);
    				//!< only works when 1st constructor used
    const char*		formulaName() const;
    void		setFormulaName(const char*);

    BufferString	getText() const;

protected:

    uiComboBox*		typfld_;
    uiComboBox*		nmfld_;
    const PropertyRef::StdType fixedtype_;

    void		typSel(CallBacker*);
    void		nameSel(CallBacker*);

    void		createFlds(uiObject*);

};


#endif

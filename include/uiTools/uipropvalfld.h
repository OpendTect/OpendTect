#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class PropertyRef;
class UnitOfMeasure;
class uiGenInput;
class uiUnitSel;

/*!\brief Interface for displaying/editing a value based on a PropertyRef
	  All input/output value (must) match the PropertyRef unit
*/


mExpClass(uiTools) uiPropertyValFld : public uiGroup
{
public:

			uiPropertyValFld(uiParent*,const PropertyRef&,
					 float defval);
			~uiPropertyValFld();

    float		getValue() const;
    void		setValue(float val);
			//!< using
    const char*		getUnitName() const;
    const UnitOfMeasure* getUnit() const;
    void		setUnit(const UnitOfMeasure* uom=nullptr);
    void		setUnitName(const char*);

    void		setReadOnly(bool); //!< will still allow unit selection
    const char*		propName() const;

    Notifier<uiPropertyValFld> valueChanged;

protected:

    const UnitOfMeasure* pruom_;
    uiGenInput*		valfld_;
    uiUnitSel*		unfld_;
    mutable float	lastsetvalue_;

    void		handleValChg(float,bool) const;

    void		valChg(CallBacker*);
    void		unChg(CallBacker*);

};



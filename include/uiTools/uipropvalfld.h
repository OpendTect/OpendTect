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


mExpClass(uiTools) uiPropertyValFld : public uiGroup
{
public:

			uiPropertyValFld(uiParent*,const PropertyRef&,
					 float defval=mUdf(float),
					 const UnitOfMeasure* defunit=0);

    float		getValue(bool internal=true) const;
    void		setValue(float val,bool isinternal=true);
    const char*		getUnitName() const;
    const UnitOfMeasure* getUnit() const;
    void		setUnit(const UnitOfMeasure* uom=0);
    void		setUnitName(const char*);

    void		setReadOnly(bool); //!< will still allow unit selection
    const char*		propName() const;

    Notifier<uiPropertyValFld> valueChanged;

protected:

    uiGenInput*		valfld_;
    uiUnitSel*		unfld_;
    const UnitOfMeasure* prevuom_;
    mutable float	lastsetvalue_;

    void		handleValChg(float,bool) const;

    void		valChg(CallBacker*);
    void		unChg(CallBacker*);

};



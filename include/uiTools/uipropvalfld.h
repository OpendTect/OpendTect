#ifndef uipropvalfld_h
#define uipropvalfld_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Aug 2012
 RCS:           $Id: uipropvalfld.h,v 1.3 2012/08/30 14:50:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
class PropertyRef;
class UnitOfMeasure;
class uiGenInput;
class uiUnitSel;


mClass uiPropertyValFld : public uiGroup
{
public:

			uiPropertyValFld(uiParent*,const PropertyRef&,
					 float defval=mUdf(float),
					 const UnitOfMeasure* defunit=0);

    float		getValue(bool internal=true) const;
    void		setValue(float val,bool isinternal=true);
    const char*		getUnitName() const;
    const UnitOfMeasure* getUnit() const	{ return curuom_; }
    void		setUnit(const UnitOfMeasure* uom=0);
    void		setUnitName(const char*);

    void		setReadOnly(bool); //!< will allow user to select unit!
    const char*		propName() const;

protected:

    uiGenInput*		valfld_;
    uiUnitSel*		unfld_;
    const UnitOfMeasure* curuom_;

    void		unChg(CallBacker*);

};


#endif

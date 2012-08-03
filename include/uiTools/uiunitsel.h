#ifndef uiunitsel_h
#define uiunitsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Feb 2010
 RCS:           $Id: uiunitsel.h,v 1.6 2012-08-03 13:01:16 cvskris Exp $
________________________________________________________________________

-*/


#include "uitoolsmod.h"
#include "uigroup.h"
#include "propertyref.h"

class uiComboBox;
class UnitOfMeasure;


mClass(uiTools) uiUnitSel : public uiGroup
{
public:
    				uiUnitSel(uiParent*,PropertyRef::StdType,
					  const char* lbltxt=0,
					  bool dispsymbols=false,
					  bool withempty=false);

    void			setUnit(const char*);
    const UnitOfMeasure*	getUnit() const;

    void			setPropType(PropertyRef::StdType);

    uiComboBox*			inpFld() const	{ return inpfld_; }

    Notifier<uiUnitSel>		selChange;

protected:

    PropertyRef::StdType	proptype_;
    ObjectSet<const UnitOfMeasure>	units_;
    bool			symbolsdisp_;

    uiComboBox*			inpfld_;
    bool 			withempty_;

    void			selChg( CallBacker* )	{ selChange.trigger(); }
    void			update();
};


#endif


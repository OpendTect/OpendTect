#ifndef uiunitsel_h
#define uiunitsel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Raman K Singh
 Date:          Feb 2010
 RCS:           $Id: uiunitsel.h,v 1.5 2012-04-12 14:46:54 cvsbruno Exp $
________________________________________________________________________

-*/


#include "uigroup.h"
#include "propertyref.h"

class uiComboBox;
class UnitOfMeasure;


mClass uiUnitSel : public uiGroup
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

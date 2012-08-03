#ifndef uiprice_h
#define uiprice_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2011
 RCS:		$Id: uiprice.h,v 1.3 2012-08-03 13:01:14 cvskris Exp $
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class uiLineEdit;
class uiComboBox;
class Price;

mClass(uiTools) uiPrice : public uiGroup
{
public:
		uiPrice( uiParent* p, const char* label,
			 const Price* price = 0 );
    void	setPrice(const Price&);
    bool	getPrice(Price&) const;

    void	allowCurrencyEdit( bool );

protected:
    uiComboBox*	currencyselfld_;
    uiLineEdit*	valuefld_;
};



#endif


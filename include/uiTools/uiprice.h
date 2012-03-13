#ifndef uiprice_h
#define uiprice_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2011
 RCS:		$Id: uiprice.h,v 1.2 2012-03-13 15:44:04 cvskris Exp $
________________________________________________________________________

-*/

#include "uigroup.h"

class uiLineEdit;
class uiComboBox;
class Price;

mClass uiPrice : public uiGroup
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

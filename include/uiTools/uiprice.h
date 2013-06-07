#ifndef uiprice_h
#define uiprice_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Dec 2011
 RCS:		$Id$
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

protected:
    uiComboBox*	currencyselfld_;
    uiLineEdit*	valuefld_;
};



#endif

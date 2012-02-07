#ifndef price_h
#define price_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2011
 RCS:           $Id: price.h,v 1.3 2012-02-07 10:29:42 cvsbert Exp $
________________________________________________________________________

-*/

#include "fixedstring.h"
#include "manobjectset.h"

class BufferStringSet;

/* Class to hande currencies */
mStruct Currency
{
				Currency(const char* abrevation, short devisor)
				    : devisor_( devisor )
				    , abrevation_( abrevation )
				{}

    short			devisor_;
    FixedString			abrevation_;

    static const Currency*	getCurrency(const char* abrevation);
    static void			getCurrencyStrings(BufferStringSet&);

    static ManagedObjectSet<const Currency>	repository_;
};


mStruct Price
{
                        Price( float userprice = 0 )
			    : currency_( Currency::getCurrency("EUR") )
			{
			    setUserPrice( userprice );
			}

    float		getUserPrice() const
    			{ return ((float) amount_)/currency_->devisor_; }
    void		setUserPrice( float p )
			{ amount_ = currency_ ? mNINT(p*currency_->devisor_)
					      : mNINT(p); }

    int			amount_; //In lowest devisible unit
    const Currency*	currency_;
};


#endif

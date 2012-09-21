#ifndef price_h
#define price_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Dec 2011
 RCS:           $Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "fixedstring.h"
#include "manobjectset.h"

class BufferStringSet;

/* Class to hande currencies */
mClass(General) Currency
{
public:
				Currency(const char* abrevation, short devisor)
				    : devisor_( devisor )
				    , abrevation_( abrevation )
				{}

    short			devisor_;
    FixedString			abrevation_;

    static const Currency*	getCurrency(const char* abrevation);
    static void			getCurrencyStrings(BufferStringSet&);

    static ManagedObjectSet<const Currency>	repository_;
    static const char*		sKeyEUR() { return "EUR"; }
};


mClass(General) Price
{
public:
               Price( double userprice = 0,
		       const char* currencystr=Currency::sKeyEUR() )
		    : currency_( Currency::getCurrency(currencystr) )
		{
		    setUserPrice( userprice );
		}

    bool	operator==(const Price& p) const
		{
		    return p.amount_==amount_ &&
			   currency_->abrevation_==p.currency_->abrevation_;
		}

    double	getUserPrice() const
    		{ return ((double) amount_)/currency_->devisor_; }
    void	setUserPrice( double p )
		{ amount_ = currency_ ? mNINT32(p*currency_->devisor_)
				      : mNINT32(p); }

    int			amount_; //In lowest devisible unit
    const Currency*	currency_;
};

#endif

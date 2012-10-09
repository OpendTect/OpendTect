#ifndef classify_h
#define classify_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		Nov 2005
 RCS:		$Id$
________________________________________________________________________

-*/

#include "typeset.h"


/*!\brief Finds out which class is the best from a series of weighted
          data points
*/

template <class T>
mClass WeightedClassCounter
{
public:
    		WeightedClassCounter( bool canbeneg=true )
		: wts_(0)
		, canbeneg_(canbeneg)		{}

    void	add(int val,T wt=1);

    bool	valid() const			{ return wts_; }
    int		result() const;

    void	clear()				{ delete wts_; wts_ = 0; }

protected:

    bool	canbeneg_;
    TypeSet<T>*	wts_;

};


template <class T>
inline void WeightedClassCounter<T>::add( int val, T wt )
{
    const int arridx = !canbeneg_ ? val
		     : (val < 0 ? -1 - 2 * val : 2 * val);
			// encode 0=>0 -1<=1 1=>2 -2=>3 2=>4 etc.

    if ( !wts_ || wts_->size() <= arridx )
    {
	TypeSet<T>* tmp = wts_ ? new TypeSet<T>( *wts_ ) : 0;
	delete wts_;
	wts_ = new TypeSet<T>( arridx+1, -2. );
	if ( tmp )
	    memcpy( wts_->arr(), tmp->arr(), tmp->size() * sizeof(T) );
    }

    T& curwt = (*wts_)[arridx];
    if ( curwt < -1 )
	curwt = wt;
    else
	curwt += wt;
}


template <class T>
inline int WeightedClassCounter<T>::result() const
{
    if ( !valid() ) return 0;

    int winner = -1; T maxwt = -1;
    for ( int idx=0; idx<wts_->size(); idx++ )
    {
	if ( (*wts_)[idx] > maxwt )
	    { winner = idx; maxwt = (*wts_)[idx]; }
    }

    int res = winner;
    if ( canbeneg_ )
    {
	// decode 0=>0 1=>-1 2=>1 3=>-2 4=>2 etc.
	res /= 2;
	if ( winner % 2 ) res = -res;
    }
    return res;
}


#endif

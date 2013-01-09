#ifndef odset_h
#define odset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2009
 RCS:		$Id$
________________________________________________________________________

-*/

#ifndef gendefs_h
# include "gendefs.h"
#endif

namespace OD
{

/*!
\ingroup Basic
\brief Base class for all sets used in OpendTect. 
*/

mClass(Basic) Set
{
public:

    virtual		~Set()					{}

    virtual od_int64	nrItems() const				= 0;
    inline bool		isEmpty() const		{ return nrItems() == 0; }
    virtual bool	validIdx(od_int64) const		= 0;
    virtual void	swap(od_int64,od_int64)			= 0;
    virtual void	erase()					= 0;
    virtual void	removeRange(od_int64 start,od_int64 stop)  = 0;
};

} // namespace

#endif


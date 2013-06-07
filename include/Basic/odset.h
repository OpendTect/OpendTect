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

mClass Set
{
public:

    virtual		~Set()			{}

    virtual int		nrItems() const		= 0;
    inline bool		isEmpty() const		{ return nrItems() == 0; }
    virtual bool	validIdx(int) const	= 0;
    virtual void	swap(int,int)		= 0;
    virtual void	erase()			= 0;
    virtual void	remove(int from,int to)	= 0;

};

} // namespace

#endif

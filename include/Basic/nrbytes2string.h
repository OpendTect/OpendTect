#ifndef nrbytes2string_h
#define nrbytes2string_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris
 Date:		2013
 RCS:		$Id$
________________________________________________________________________
-*/

#include "basicmod.h"
#include "string2.h"



/*!Converts integer with number of bytes to a string with KB, GB or similar
   unit. */

mExpClass(Basic) NrBytesToStringCreator
{
public:
			NrBytesToStringCreator();
    enum Unit		{ Bytes=0, KB=1, MB=2, GB=3, TB=4, PB=5 };

    void		setUnitFrom(od_uint64 number,bool maximum=true);
			/*!<Sets the unit (B, KB, MB, GB, TB) based on the
			 number.
			 \param maximum will only change unit if a larger
			 unit is needed.
			 */

    FixedString		getString(od_uint64 number,int nrdecimals=2,
				  bool withunit=true) const;
			/*!<Use string before doing anything else, as it will be
			    overwritten at next call from same thread. */

    FixedString		getUnitString() const;
    static FixedString	toString(Unit);

protected:

    Unit		unit_;

};


#endif

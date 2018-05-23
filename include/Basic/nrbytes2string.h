#pragma once

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kris
 Date:		2013
________________________________________________________________________
-*/

#include "basicmod.h"
#include "bufstring.h"



/*!Converts integer with number of bytes to a string with KB, GB or similar
   unit. */

mExpClass(Basic) NrBytesToStringCreator
{
public:
			NrBytesToStringCreator();
			NrBytesToStringCreator(od_int64 nrbytes);
				/*!< Unit is set from nrbytes*/
    enum Unit		{ Bytes=0, KB=1, MB=2, GB=3, TB=4, PB=5, EB=6 };

    void		setUnitFrom(od_int64 number,bool maximum=true);
			/*!<Sets the unit (B, KB, MB, GB, TB, EB) based on the
			 number.
			 \param maximum will only change unit if a larger
			 unit is needed.
			 */

    BufferString	getString(od_int64 number,int nrdecimals=2,
				  bool withunit=true) const;
			/*!<Use string before doing anything else, as it will be
			    overwritten at next call from same thread. */

    BufferString	getUnitString() const;
    static const char*	toString(Unit);

protected:

    Unit		unit_;

};

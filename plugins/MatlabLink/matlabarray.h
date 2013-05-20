#ifndef matlabarray_h
#define matlabarray_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		January 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "matlablinkmod.h"

class mxArray;
template <class T> class ArrayND;

//mExpClass(MatlabLink) MatlabArrayND
class MatlabArrayND
{
public:
			MatlabArrayND(const ArrayND<float>&);
			~MatlabArrayND();

protected:

    mxArray*			mxarr_;
    const ArrayND<float>&	arrnd_;

};

#endif

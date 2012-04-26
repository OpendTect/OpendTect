#ifndef odsysmem_h
#define odsysmem_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2012
 RCS:		$Id: odsysmem.h,v 1.2 2012-04-26 10:35:49 cvsbert Exp $
________________________________________________________________________

*/

#include "gendefs.h"
class IOPar;

namespace OD
{
    mGlobal void	getSystemMemory(float& total,float& free);
    mGlobal void	dumpMemInfo(IOPar&);

    mGlobal bool	haveMemInfo(); //!< legacy: returns true on all plfs now
}


#endif

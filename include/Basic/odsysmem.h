#ifndef odsysmem_h
#define odsysmem_h
/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		April 2012
 RCS:		$Id: odsysmem.h,v 1.3 2012/05/17 05:57:31 cvsbert Exp $
________________________________________________________________________

*/

#include "gendefs.h"
class IOPar;

namespace OD
{
    mGlobal void	getSystemMemory(float& total,float& free);
    mGlobal void	dumpMemInfo(IOPar&);
}


#endif

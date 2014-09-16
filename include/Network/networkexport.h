#ifndef networkexport_h
#define networkexport_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2013
 RCS:		$Id: basicexport.h 33063 2014-01-17 16:02:10Z kristofer.tingdahl@dgbes.com $
________________________________________________________________________

-*/

# ifdef do_import_export

#  include "hostdata.h"

mExportTemplClassInst( Network ) ManagedObjectSet<HostData>;

# endif
#endif

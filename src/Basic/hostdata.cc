/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: hostdata.cc,v 1.1 2002-04-05 16:30:24 bert Exp $
________________________________________________________________________

-*/

#include "hostdata.h"
#include <netdb.h>


const char* HostData::shortestName() const
{
    int len = name_.size();
    int ishortest = -1;
    for ( int idx=0; idx<aliases_.size(); idx++ )
	if ( aliases_[idx]->size() < len )
	    { len = aliases_[idx]->size(); ishortest = idx; }

    return (const char*)(ishortest < 0 ? name_ : *aliases_[ishortest]);
}


HostDataList::HostDataList()
{
#ifndef __msvc__
    sethostent(0);
    struct hostent* he;
    while ( (he = gethostent()) )
    {
	HostData* newhd = new HostData( he->h_name );
	char** al = he->h_aliases;
	while ( *al )
	    { newhd->aliases_ += new BufferString(*al); al++; }
	*this += newhd;
    }
    endhostent();
#endif
}

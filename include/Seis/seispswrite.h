#ifndef seispswrite_h
#define seispswrite_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispswrite.h,v 1.1 2004-12-30 15:04:40 bert Exp $
________________________________________________________________________

-*/

#include "datachar.h"
class SeisTrc;


/*!\brief writes to a pre-stack seismic data store.

 Expected is a supply of traces with correct BinID, offset and azimuth.
 The supply MUST be per gather. Additionally, inline(1) and crossline(2)
 sorting may be required.

*/

class SeisPSWriter
{
public:

    virtual		~SeisPSWriter()			{}

    virtual bool	fullSortingRequired() const	{ return true; }
    virtual void	setPrefStorType( DataCharacteristics::UserType )
							{}
    virtual void	close()				{}

    virtual bool	put(const SeisTrc&)		= 0;
    virtual const char*	errMsg() const			= 0;

};


#endif

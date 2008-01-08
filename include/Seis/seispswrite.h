#ifndef seispswrite_h
#define seispswrite_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id: seispswrite.h,v 1.3 2008-01-08 11:54:18 cvsbert Exp $
________________________________________________________________________

-*/

#include "datachar.h"
class IOPar;
class SeisTrc;


/*!\brief writes to a pre-stack (3D) seismic data store.

 Expected is a supply of traces with correct BinID, offset and azimuth.
 The supply MUST be per gather. Additionally, inline(1) and crossline(2)
 sorting may be required.

*/

class SeisPSWriter
{
public:

    virtual		~SeisPSWriter()			{}

    virtual void	usePar(const IOPar&)		{}
    virtual bool	fullSortingRequired() const	{ return true; }
    virtual void	setPrefStorType( DataCharacteristics::UserType )
							{}
    virtual void	close()				{}

    virtual bool	put(const SeisTrc&)		= 0;
    virtual const char*	errMsg() const			= 0;

};


/*!\brief writes to a 2D pre-stack seismic data store.

 Expected is a supply of traces with correct trace number, coordinate,
 offset and azimuth.
 The supply MUST be per gather; sorting by trace number requirement is very
 likely.

*/

class SeisPS2DWriter
{
public:

    virtual		~SeisPS2DWriter()		{}

    virtual void	usePar(const IOPar&)		{}
    virtual void	close()				{}

    virtual bool	put(const SeisTrc&)		= 0;
    virtual const char*	errMsg() const			= 0;

};


#endif

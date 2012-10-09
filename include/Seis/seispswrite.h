#ifndef seispswrite_h
#define seispswrite_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2004
 RCS:		$Id$
________________________________________________________________________

-*/

#include "datachar.h"
class IOPar;
class SeisTrc;
class BufferStringSet;


/*!\brief writes to a pre-stack seismic data store.

 Expected is a supply of traces with correct offset and azimuth.
 The supply MUST be per gather. For 3D, inline and crossline must be correct,
 for 2D a valid trace number is required.

 If sorting is required, it is on inline (primary) and crossline (secondary)
 or trace number (for 2D).

 If supported, you may be able to set a symbolic name for each sample (e.g. an
 attribute name). If so, do it before the first put.

*/

mClass SeisPSWriter
{
public:

    virtual		~SeisPSWriter()			{}

    virtual void	usePar(const IOPar&)		{}
    virtual bool	fullSortingRequired() const	{ return true; }
    virtual void	setPrefStorType( DataCharacteristics::UserType ) {}
    virtual bool	setSampleNames(const BufferStringSet&) const
    			{ return false; }

    virtual bool	put(const SeisTrc&)		= 0;
    virtual const char*	errMsg() const			= 0;

    virtual void	close()				{}

};


#endif

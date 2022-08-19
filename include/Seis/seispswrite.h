#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "datachar.h"
class SeisTrc;
class BufferStringSet;


/*!\brief writes to a prestack seismic data store.

 Expected is a supply of traces with correct offset and azimuth.
 The supply MUST be per gather. For 3D, inline and crossline must be correct,
 for 2D a valid trace number is required.

 If sorting is required, it is on inline (primary) and crossline (secondary)
 or trace number (for 2D).

 If supported, you may be able to set a symbolic name for each sample (e.g. an
 attribute name). If so, do it before the first put.

*/

mExpClass(Seis) SeisPSWriter
{
public:

    virtual		~SeisPSWriter()			{}

    virtual void	usePar(const IOPar&)		{}
    virtual bool	fullSortingRequired() const	{ return true; }
    virtual void	setPrefStorType( DataCharacteristics::UserType ) {}
    virtual bool	setSampleNames(const BufferStringSet&) const
    			{ return false; }

    virtual bool	put(const SeisTrc&)		= 0;
    virtual uiString	errMsg() const			= 0;

    virtual void	close()				{}

};

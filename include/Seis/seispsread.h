#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Dec 2004
________________________________________________________________________

-*/

#include "seismod.h"
#include "bufstring.h"
#include "binid.h"
#include "posgeomid.h"
#include "uistring.h"

class IOObj;
class SeisTrc;
class SeisTrcBuf;
class BufferStringSet;
namespace PosInfo { class CubeData; class Line2DData; }


/*!\brief reads from a prestack seismic data store.

 Some data stores like attribute stores have a symbolic name for each sample. In
 that case, getSampleNames may return true.

*/

mExpClass(Seis) SeisPSReader
{
public:

    virtual		~SeisPSReader()					{}
    virtual bool	is3D() const					= 0;
    virtual bool	is2D() const					= 0;
    virtual Pos::GeomID geomID() const					= 0;

    virtual void	usePar(const IOPar&)				{}

    virtual uiString	errMsg() const					= 0;
    virtual SeisTrc*	getTrace(const BinID&,int nr=0) const;
    virtual bool	getGather(const BinID&,SeisTrcBuf&) const	= 0;

    virtual bool	getSampleNames(BufferStringSet&) const
			{ return false; }

    virtual StepInterval<float>	getZRange() const;

};

/*!\brief reads from a 3D prestack seismic data store. */

mExpClass(Seis) SeisPS3DReader : public SeisPSReader
{
public:

    bool		is3D() const override	{ return true; }
    bool		is2D() const override	{ return false; }
    Pos::GeomID		geomID() const override;

    virtual const PosInfo::CubeData&	posData() const			= 0;

};


/*!\brief reads from a 2D prestack seismic data store. */

mExpClass(Seis) SeisPS2DReader : public SeisPSReader
{
public:
			SeisPS2DReader(const char* lnm);
			SeisPS2DReader(Pos::GeomID);

    bool		is3D() const override	{ return false; }
    bool		is2D() const override	{ return true; }
    const char*		lineName() const	{ return lnm_.buf(); }
    Pos::GeomID		geomID() const override { return geomid_; }

    SeisTrc*		getTrc(int trcnr,int nr=0) const;
    bool		getGath(int trcnr,SeisTrcBuf&) const;

    virtual const PosInfo::Line2DData&	posData() const		= 0;

protected:

    BufferString	lnm_;
    Pos::GeomID		geomid_;

};



#ifndef maddefs_h
#define maddefs_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id: madio.h,v 1.1 2007-10-01 15:20:08 cvsbert Exp $
-*/

#include "bufstring.h"
#include "strmdata.h"
#include "seistype.h"
class IOObj;
class IOPar;
class SeisTrc;
class StreamData;
class SeisSelData;
class SeisTrcReader;

namespace ODMad
{

class FileSpec
{
public:

    			FileSpec(bool forread,const char* fnm=0,
				 const char* datapath=0);
			//!< Not absolute: uses datapath, if null $DATAPATH

    BufferString&	fileName()		{ return fname_; }
    const BufferString&	fileName() const	{ return fname_; }
    StreamData		open() const;

    static const char*	defDataPath();
    static const char*	sKeyDataPath;

protected:

    BufferString	fname_;
    bool		forread_;

};


class SeisInp
{
public:

    virtual bool	usePar(const IOPar&) const	= 0;
    virtual bool	get(SeisTrc&) const		= 0;
    const char*		errMsg() const			{ return errmsg_; }

protected:

			SeisInp();
    virtual		~SeisInp();

    mutable BufferString errmsg_;


};


class MadSeisInp : public SeisInp
{
public:

    			MadSeisInp();
    virtual		~MadSeisInp();

    virtual bool	usePar(const IOPar&);
    virtual bool	get(SeisTrc&) const;

    Seis::GeomType	geomType() const	{ return geom_; }
    const SeisSelData&	selData() const		{ return seldata_; }

protected:

    FileSpec		fspec_;
    StreamData		sd_;
    Seis::GeomType	geom_;
    SeisSelData&	seldata_;

};


class ODSeisInp : public SeisInp
{
public:

    			ODSeisInp() : rdr_(0)		{}

    virtual bool	usePar(const IOPar&);
    virtual bool	get(SeisTrc&) const;

protected:

    SeisTrcReader*	rdr_;


};


} // namespace ODMad

#endif

#ifndef seisstor_h
#define seisstor_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		20-1-98
 RCS:		$Id$
________________________________________________________________________

Trace storage objects handle seismic data storage.

-*/


#include "seisinfo.h"
class Conn;
class IOObj;
class Translator;
class SeisTrcBuf;
class Seis2DLineSet;
class SeisPSIOProvider;
class SeisTrcTranslator;
namespace Seis		{ class SelData; }


/*!\brief base class for seis reader and writer. */

mClass SeisStoreAccess
{
public:

    virtual		~SeisStoreAccess();
    virtual bool	close();

    bool		is2D() const		{ return is2d; }
    bool		isPS() const		{ return psioprov; }
    Seis::GeomType	geomType() const
    			{ return Seis::geomTypeOf(is2D(),isPS()); }

    const char*		errMsg() const
			{ return errmsg_.str(); }
    int			tracesHandled() const
			{ return nrtrcs; }

    const IOObj*	ioObj() const
			{ return ioobj; }
    void		setIOObj(const IOObj*);
    const Seis::SelData* selData() const
			{ return seldata; }
    void		setSelData(Seis::SelData*);
			//!< The Seis::SelData becomes mine

    virtual void	usePar(const IOPar&);
				// Afterwards check whether curConn is still OK.
    virtual void	fillPar(IOPar&) const;

    static const char*	sNrTrcs;

    // Note that the Translator is always created, but only actually used for 3D
    Translator*		translator()
			{ return trl; }
    Translator*		translator() const
			{ return trl; }

    // 3D only
    Conn*		curConn3D();
    const Conn*		curConn3D() const;

    // 3D and 2D
    SeisTrcTranslator*	seisTranslator()
			{ return strl(); }
    const SeisTrcTranslator* seisTranslator() const
			{ return strl(); }
    // 2D only
    Seis2DLineSet*	lineSet()
			{ return lset; }
    const Seis2DLineSet* lineSet() const
			{ return lset; }

    // Pre-Stack only
    const SeisPSIOProvider* psIOProv() const
			{ return psioprov; }

protected:

			SeisStoreAccess(const IOObj*);
			SeisStoreAccess(const char*,bool is2d,bool isps);
    virtual void	init()			{}
    bool		cleanUp(bool alsoioobj=true);

    IOObj*		ioobj;
    bool		is2d;
    int			nrtrcs;
    Translator*		trl;
    Seis2DLineSet*	lset;
    Seis::SelData*	seldata;
    const SeisPSIOProvider* psioprov;
    BufferString	errmsg_;

    SeisTrcTranslator*	strl() const;

};


#endif

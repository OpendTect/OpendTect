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


#include "seismod.h"
#include "seisinfo.h"
class Conn;
class IOObj;
class Translator;
class SeisTrcBuf;
class Seis2DDataSet;
class Seis2DLineSet;
class SeisPSIOProvider;
class SeisTrcTranslator;
namespace Seis		{ class SelData; }


/*!\brief base class for seis reader and writer. */

mExpClass(Seis) SeisStoreAccess
{
public:

    virtual		~SeisStoreAccess();
    virtual bool	close();

    bool		is2D() const		{ return is2d_; }
    bool		isPS() const		{ return psioprov_; }
    Seis::GeomType	geomType() const
    			{ return Seis::geomTypeOf(is2D(),isPS()); }

    const char*		errMsg() const
			{ return errmsg_.str(); }
    int			tracesHandled() const
			{ return nrtrcs_; }

    const IOObj*	ioObj() const
			{ return ioobj_; }
    void		setIOObj(const IOObj*);
    const Seis::SelData* selData() const
			{ return seldata_; }
    void		setSelData(Seis::SelData*);
			//!< The Seis::SelData becomes mine

    virtual void	usePar(const IOPar&);
				// Afterwards check whether curConn is still OK.
    virtual void	fillPar(IOPar&) const;

    static const char*	sNrTrcs;

    // Note that the Translator is always created, but only actually used for 3D
    Translator*		translator()			{ return trl_; }
    Translator*		translator() const		{ return trl_; }

    // 3D only
    Conn*		curConn3D();
    const Conn*		curConn3D() const;

    // 3D and 2D
    SeisTrcTranslator*	seisTranslator()		{ return strl(); }
    const SeisTrcTranslator* seisTranslator() const	{ return strl(); }
    // 2D only
    Seis2DLineSet*	lineSet()			{ return lset_; }
    const Seis2DLineSet* lineSet() const		{ return lset_; }
    Seis2DDataSet*	dataSet()				{ return dataset_; }
    const Seis2DDataSet* dataSet() const		{ return dataset_; }

    // Pre-Stack only
    const SeisPSIOProvider* psIOProv() const		{ return psioprov_; }

protected:

			SeisStoreAccess(const IOObj*);
			SeisStoreAccess(const char*,bool is2d,bool isps);
    virtual void	init()			{}
    bool		cleanUp(bool alsoioobj=true);

    IOObj*			ioobj_;
    bool			is2d_;
    int				nrtrcs_;
    Translator*			trl_;
    Seis2DLineSet*		lset_;
    Seis2DDataSet*		dataset_;
    Seis::SelData*		seldata_;
    const SeisPSIOProvider*	psioprov_;
    BufferString		errmsg_;

    SeisTrcTranslator*		strl() const;

};


#endif


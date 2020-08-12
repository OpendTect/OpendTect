#pragma once

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
#include "uistring.h"

class Conn;
class IOObj;
class Translator;
class Seis2DDataSet;
class SeisPSIOProvider;
class SeisTrcTranslator;
namespace Seis		{ class SelData; }


/*!\brief base class for seis reader and writer. */

mExpClass(Seis) SeisStoreAccess
{ mODTextTranslationClass(SeisStoreAccess);
public:

    virtual		~SeisStoreAccess();
    virtual bool	close();

    bool		is2D() const		{ return is2d_; }
    bool		isPS() const		{ return psioprov_; }
    Seis::GeomType	geomType() const
			{ return Seis::geomTypeOf(is2D(),isPS()); }

    uiString		errMsg() const		{ return errmsg_; }
    int			tracesHandled() const
			{ return nrtrcs_; }

    const IOObj*	ioObj() const
			{ return ioobj_; }
    void		setIOObj(const IOObj*);
    const Seis::SelData* selData() const
			{ return seldata_; }
    virtual void	setSelData(Seis::SelData*);
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
    Seis2DDataSet*	dataSet()			{ return dataset_; }
    const Seis2DDataSet* dataSet() const		{ return dataset_; }

    // Prestack only
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
    Seis2DDataSet*		dataset_;
    Seis::SelData*		seldata_;
    const SeisPSIOProvider*	psioprov_;
    mutable uiString		errmsg_;

    SeisTrcTranslator*		strl() const;

};



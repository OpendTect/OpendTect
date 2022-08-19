#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
namespace Coords	{ class CoordSystem; }
namespace Seis		{ class SelData; }


/*!\brief base class for seis reader and writer. */

mExpClass(Seis) SeisStoreAccess
{ mODTextTranslationClass(SeisStoreAccess);
public:

    mExpClass(Seis) Setup
    {
    public:
			Setup(const IOObj&,const Seis::GeomType*);
    explicit		Setup(const IOObj&,Pos::GeomID,const Seis::GeomType*);
			Setup(const Setup&);
			~Setup();

    Setup&		operator =(const Setup&);

    PtrMan<IOObj>	getIOObj() const;

    void		usePar(const IOPar&);
    Setup&		ioobj(const IOObj&);
    Setup&		geomtype(Seis::GeomType);
    Setup&		seldata(const Seis::SelData*);
    Setup&		coordsys(const Coords::CoordSystem&);

    const IOObj*	ioobj_ = nullptr;
    Seis::GeomType	geomtype_ = Seis::Vol;
    mDefSetupMemb(Pos::GeomID,geomid);
    ConstRefMan<Coords::CoordSystem> coordsys_;
    Seis::SelData*	seldata_ = nullptr;
    mDefSetupMemb(BufferString,hdrtxt);
    mDefSetupMemb(int,compnr);	// -1 = all
    };

    virtual		~SeisStoreAccess();
    virtual bool	close();

    bool		is2D() const		{ return is2d_; }
    bool		isPS() const		{ return psioprov_; }
    Seis::GeomType	geomType() const
			{ return Seis::geomTypeOf(is2D(),isPS()); }

    bool		isOK() const		{ return errmsg_.isEmpty(); }
    bool		isPrepared() const	{ return prepared_; }
    uiString		errMsg() const		{ return errmsg_; }
    int			tracesHandled() const
			{ return nrtrcs_; }

    const IOObj*	ioObj() const
			{ return ioobj_; }
    void		setIOObj(const IOObj*);
    void		setIOObj(const Setup&);
    const Seis::SelData* selData() const
			{ return seldata_; }
    virtual void	setSelData(Seis::SelData*);
			//!< The Seis::SelData becomes mine

    virtual void	usePar(const IOPar&);
				// Afterwards check whether curConn is still OK.
    virtual void	fillPar(IOPar&) const;
    static PtrMan<IOObj> getFromPar(const IOPar&);

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
    virtual Pos::GeomID geomID() const;

    // Prestack only
    const SeisPSIOProvider* psIOProv() const		{ return psioprov_; }

    static const char*	sKeyHeader();

    static IOObj&	getTmp(const char* fnm,bool isps,bool is2d);

protected:

			SeisStoreAccess(const MultiID&,Seis::GeomType);
			SeisStoreAccess(const IOObj*,const Seis::GeomType*);
			SeisStoreAccess(const IOObj*,Pos::GeomID,
					const Seis::GeomType*);
			SeisStoreAccess(const Setup&);
    bool		cleanUp(bool alsoioobj=true);

    IOObj*			ioobj_ = nullptr;
    bool			is2d_ = false;
    int				nrtrcs_;
    bool			prepared_ = false;
    Translator*			trl_ = nullptr;
    Seis2DDataSet*		dataset_ = nullptr;
    Seis::SelData*		seldata_ = nullptr;
    const SeisPSIOProvider*	psioprov_ = nullptr;
    mutable uiString		errmsg_;

    SeisTrcTranslator*	strl() const;

};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "seistype.h"
#include "bufstring.h"
#include "factory.h"
#include "uistring.h"

class SeisTrc;
class SeisTrcBuf;
class SeisPSReader;
class SeisTrcReader;
class SeisTrcWriter;
class BufferStringSet;
namespace Seis		{ class SelData; }

namespace Seis
{

class Bounds;

/*!\brief Base class for Seismic Sequential IO classes */

mExpClass(Seis) SeqIO
{ mODTextTranslationClass(SeqIO);
public:
			SeqIO();
    virtual		~SeqIO();

    virtual const char* type() const			= 0;
    virtual Seis::GeomType geomType() const		= 0;
    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&)		= 0;

    uiString		errMsg() const			{ return errmsg_; }

    static const char*	sKeyODType;

protected:

    mutable uiString errmsg_;
};


/*!\brief Base class for Seismic Sequential input classes */

mExpClass(Seis) SeqInp : public SeqIO
{ mODTextTranslationClass(SeqInp);
public:
			SeqInp();
			~SeqInp();

    virtual bool	get(SeisTrc&) const		= 0;

    void		fillPar(IOPar&) const override;

    virtual Seis::Bounds* getBounds() const		{ return 0; }
    virtual int		estimateTotalNumber() const	{ return -1; }

    mDefineFactoryInClass(SeqInp,factory);

protected:

    static Seis::GeomType getGeomType(const IOPar&);

};


/*!\brief OpendTect-internal Seismic Sequential input

  Set the reader via usePar or explicitly. The idea is to either provide
  a PreStackReader or a SeisTrcReader. If both present, SeisTrcReader will
  be used.

 */


mExpClass(Seis) ODSeqInp : public SeqInp
{ mODTextTranslationClass(ODSeqInp);
public:

			ODSeqInp();
			~ODSeqInp();

    const char*		type() const override		{ return sKeyODType; }

    Seis::GeomType	geomType() const override;

    bool		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    bool		get(SeisTrc&) const override;

    Seis::Bounds*	getBounds() const override;
    int			estimateTotalNumber() const override;

    static void		initClass();
    static SeqInp*	create()		{ return new ODSeqInp; }

    SeisTrcReader*	rdr_;
    SeisPSReader*	psrdr_;

protected:

    SeisTrcBuf&		gath_;
    mutable int		curposidx_;
    mutable int		segidx_;
    mutable int		ldidx_;

};


/*!\brief Base class for Seismic Sequential output classes */

mExpClass(Seis) SeqOut : public SeqIO
{ mODTextTranslationClass(SeqOut);
public:
			SeqOut();
			~SeqOut();

    virtual bool	put(const SeisTrc&)		= 0;

    static BufferStringSet& classNames();
    static SeqOut*	make(const char*);
    static void		addClass(SeqOut*);

    mDefineFactoryInClass(SeqOut,factory);

};


/*!\brief Seismic Sequential output via SeistrcWriter

  The wrr_ will be deleted on destruction.

 */

mExpClass(Seis) ODSeqOut : public SeqOut
{ mODTextTranslationClass(ODSeqOut);
public:

			ODSeqOut() : wrr_(0)	{}
			~ODSeqOut();

    const char*		type() const override		{ return sKeyODType; }
    Seis::GeomType	geomType() const override;

    bool		usePar(const IOPar&) override;
    void		fillPar(IOPar&) const override;
    bool		put(const SeisTrc&) override;

    static void		initClass();
    static SeqOut*	create()		{ return new ODSeqOut; }

    SeisTrcWriter*	wrr_;

};

} // namespace Seis

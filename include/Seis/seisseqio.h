#ifndef seisseqio_h
#define seisseqio_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id: seisseqio.h,v 1.7 2007-11-23 11:59:06 cvsbert Exp $
-*/

#include "seistype.h"
#include "bufstring.h"
#include "factory.h"

class IOPar;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
class BufferStringSet;
namespace Seis		{ class SelData; }

namespace Seis
{

class Bounds;

/*!\brief Base class for Seismic Sequential IO classes */

class SeqIO
{
public:

    virtual const char*	type() const			= 0;
    virtual Seis::GeomType geomType() const		= 0;
    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&)		= 0;

    const char*		errMsg() const			{ return errmsg_; }

    static const char*	sKeyODType;

protected:

    mutable BufferString errmsg_;
};


/*!\brief Base class for Seismic Sequential input classes */

class SeqInp : public SeqIO
{
public:

    virtual bool	get(SeisTrc&) const		= 0;
    virtual const Seis::SelData& selData() const	= 0;

    virtual void	fillPar(IOPar&) const;

    virtual Seis::Bounds* getBounds() const		{ return 0; }

    mDefineFactoryInClass(SeqInp,factory);

protected:

    Seis::SelData&	emptySelData() const;
};


/*!\brief OpendTect-internal Seismic Sequential input */


class ODSeqInp : public SeqInp
{
public:

    			ODSeqInp() : rdr_(0)	{}
    			~ODSeqInp();

    virtual const char*	type() const		{ return sKeyODType; }

    virtual Seis::GeomType geomType() const;
    virtual const Seis::SelData& selData() const;
    virtual void	setSelData(const Seis::SelData&);

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual bool	get(SeisTrc&) const;

    virtual Seis::Bounds* getBounds() const;

    static void		initClass();
    static SeqInp*	create()		{ return new ODSeqInp; }

protected:


    SeisTrcReader*	rdr_;
};


/*!\brief Base class for Seismic Sequential output classes */

class SeqOut : public SeqIO
{
public:

    virtual bool	put(const SeisTrc&)		= 0;

    static BufferStringSet& classNames();
    static SeqOut*	make(const char*);
    static void		addClass(SeqOut*);

    mDefineFactoryInClass(SeqOut,factory);

};


class ODSeqOut : public SeqOut
{
public:

    			ODSeqOut() : wrr_(0)	{}
    			~ODSeqOut();

    virtual const char*	type() const		{ return sKeyODType; }
    virtual Seis::GeomType geomType() const;

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual bool	put(const SeisTrc&);

    static void		initClass();
    static SeqOut*	create()		{ return new ODSeqOut; }

protected:

    SeisTrcWriter*	wrr_;

};


} // namespace Seis


#endif

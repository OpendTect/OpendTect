#ifndef seisseqio_h
#define seisseqio_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id$
-*/

#include "seismod.h"
#include "seistype.h"
#include "bufstring.h"
#include "factory.h"

class IOPar;
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

mClass(Seis) SeqIO
{
public:

    virtual const char*	type() const			= 0;
    virtual Seis::GeomType geomType() const		= 0;
    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&)		= 0;

    const char*		errMsg() const			{ return errmsg_.str(); }

    static const char*	sKeyODType;

protected:

    mutable BufferString errmsg_;
};


/*!\brief Base class for Seismic Sequential input classes */

mClass(Seis) SeqInp : public SeqIO
{
public:

    virtual bool	get(SeisTrc&) const		= 0;

    virtual void	fillPar(IOPar&) const;

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


mClass(Seis) ODSeqInp : public SeqInp
{
public:

    			ODSeqInp();
    			~ODSeqInp();

    virtual const char*	type() const		{ return sKeyODType; }

    virtual Seis::GeomType geomType() const;

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual bool	get(SeisTrc&) const;

    virtual Seis::Bounds* getBounds() const;
    virtual int		estimateTotalNumber() const;

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

mClass(Seis) SeqOut : public SeqIO
{
public:

    virtual bool	put(const SeisTrc&)		= 0;

    static BufferStringSet& classNames();
    static SeqOut*	make(const char*);
    static void		addClass(SeqOut*);

    mDefineFactoryInClass(SeqOut,factory);

};


/*!\brief Seismic Sequential output via SeistrcWriter

  The wrr_ will be deleted on destruction.
 
 */

mClass(Seis) ODSeqOut : public SeqOut
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

    SeisTrcWriter*	wrr_;

};


} // namespace Seis


#endif


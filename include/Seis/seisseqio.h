#ifndef seisseqio_h
#define seisseqio_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id: seisseqio.h,v 1.1 2007-10-05 11:11:09 cvsbert Exp $
-*/

#include "seistype.h"
#include "bufstring.h"
class IOPar;
class SeisTrc;
class SeisSelData;
class SeisTrcReader;
class SeisTrcWriter;
class BufferStringSet;

namespace Seis
{

/*!\brief Base class for Seismic Sequential IO classes */

class SeqIO
{
public:

    virtual const char*	type() const			= 0;
    virtual void	fillPar(IOPar&) const		= 0;
    virtual bool	usePar(const IOPar&)		= 0;

    const char*		errMsg() const			{ return errmsg_; }

    static const char*	sKeyODType;

protected:

    mutable BufferString errmsg_;

    virtual SeqIO*	makeNew() const			= 0;
};


/*!\brief Base class for Seismic Sequential input classes */

class SeqInp : public SeqIO
{
public:

    virtual bool	get(SeisTrc&) const		= 0;
    virtual const SeisSelData& selData() const;

    static BufferStringSet& classNames();
    static SeqInp*	make(const char* clssnm);
    static void		addClass(SeqInp*);


};


/*!\brief OpendTect-internal Seismic Sequential input */


class ODSeqInp : public SeqInp
{
public:

    			ODSeqInp() : rdr_(0)	{}
    			~ODSeqInp();
    virtual const char*	type() const		{ return sKeyODType; }

    virtual const SeisSelData& selData() const;

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual bool	get(SeisTrc&) const;

protected:

    virtual SeqIO*	makeNew() const		{ return new ODSeqInp; }

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

};


class ODSeqOut : public SeqOut
{
public:

    			ODSeqOut() : wrr_(0)	{}
    			~ODSeqOut();
    virtual const char*	type() const		{ return sKeyODType; }

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual bool	put(const SeisTrc&);

protected:

    virtual SeqIO*	makeNew() const		{ return new ODSeqOut; }

    SeisTrcWriter*	wrr_;

};


} // namespace Seis


#endif

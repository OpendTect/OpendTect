#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "madio.h"
#include "seisseqio.h"

namespace ODMad
{

mClass(Madagascar) SeisSeqIO
{
public:

    virtual		~SeisSeqIO();

    ODMad::IOType	getType() const			= 0;

    virtual bool	init()			= 0;

protected:

    			SeisSeqIO();

    ODMad::IOType	type_;

    virtual void	setErrMsg(const char*)	= 0;
};


mClass(Madagascar) SeisSeqInp : public SeisSeqIO
{
public:

    			SeisSeqInp();
    virtual		~SeisSeqInp();

    virtual const char*	type() const		{ return getType(); }
    virtual Seis::GeomType geomType() const	{ return gt_; }

    virtual bool	usePar(const IOPar&);
    virtual void	fillPar(IOPar&) const;
    virtual bool	get(SeisTrc&) const;
    virtual bool	getMadHeader(IOPar&) const;

    virtual bool	open();

    static void		initClass();
    static Seis::SeqInp* create()		{ return new SeisSeqInp; }

protected:

    virtual void	setErrMsg( const char* s ) { errmsg_ = s; }

};


mClass(Madagascar) SeisSeqOut : public Seis::SeqOut
		 , public SeisSeqIO
{
public:

    			SeisSeqOut(Seis::GeomType gt=Seis::Vol);
    			SeisSeqOut(Seis::GeomType,const FileSpec&);
    virtual		~SeisSeqOut();

    virtual const char*	type() const		{ return getType(); }
    virtual Seis::GeomType geomType() const	{ return gt_; }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual bool	put(const SeisTrc&);

    virtual bool	open();

    static void		initClass();
    static Seis::SeqOut* create()		{ return new SeisSeqOut; }

protected:

    virtual void	setErrMsg( const char* s ) { errmsg_ = s; }
};


} // namespace ODMad

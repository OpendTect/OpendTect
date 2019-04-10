#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Sep 2007
-*/

#include "madio.h"
#include "seistype.h"

namespace ODMad
{

mClass(Madagascar) SeisSeqIO
{
public:

    virtual		~SeisSeqIO();

    ODMad::IOType	getType() const		= 0;

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


mClass(Madagascar) SeisSeqOut : public SeisSeqIO
{
public:

			SeisSeqOut(Seis::GeomType gt=Seis::Vol);
			SeisSeqOut(Seis::GeomType,const FileSpec&);
    virtual		~SeisSeqOut();

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

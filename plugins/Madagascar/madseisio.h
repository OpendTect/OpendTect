#ifndef madseisio_h
#define madseisio_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Bert
 * DATE     : Sep 2007
 * ID       : $Id: madseisio.h,v 1.1 2007-11-15 12:53:04 cvsbert Exp $
-*/

#include "madio.h"
#include "seisseqio.h"

namespace ODMad
{

class SeisSeqIO
{
public:

    virtual		~SeisSeqIO();

    const char*		getType() const		{ return sKeyMadagascar; }
    Seis::GeomType&	getGeomType()		{ return gt_; }
    Seis::GeomType	getGeomType() const	{ return gt_; }

    FileSpec&		fileSpec()		{ return fspec_; }
    const FileSpec&	fileSpec() const	{ return fspec_; }
    StreamData&		sd()			{ return sd_; }
    const StreamData&	sd() const		{ return sd_; }

    bool		fromPar(const IOPar&);
    void		toPar(IOPar&) const;

    virtual bool	open()			= 0;

protected:

    			SeisSeqIO(Seis::GeomType,bool);

    Seis::GeomType	gt_;
    FileSpec		fspec_;
    StreamData		sd_;

    virtual void	setErrMsg(const char*)	= 0;
};


class SeisSeqInp : public Seis::SeqInp
		 , public SeisSeqIO
{
public:

    			SeisSeqInp(Seis::GeomType gt=Seis::Vol);
    			SeisSeqInp(Seis::GeomType,const FileSpec&,
				   const SeisSelData&);
    virtual		~SeisSeqInp();

    virtual const char*	type() const		{ return getType(); }
    virtual Seis::GeomType geomType() const	{ return getGeomType(); }

    virtual void	fillPar(IOPar&) const;
    virtual bool	usePar(const IOPar&);
    virtual bool	get(SeisTrc&) const;

    SeisSelData&	selData()		{ return seldata_; }
    const SeisSelData&	selData() const		{ return seldata_; }

    virtual bool	open();

    static void		initClass();
    static Seis::SeqInp* create()		{ return new SeisSeqInp; }

protected:

    SeisSelData&	seldata_;

    virtual void	setErrMsg( const char* s ) { errmsg_ = s; }
};


class SeisSeqOut : public Seis::SeqOut
		 , public SeisSeqIO
{
public:

    			SeisSeqOut(Seis::GeomType gt=Seis::Vol);
    			SeisSeqOut(Seis::GeomType,const FileSpec&);
    virtual		~SeisSeqOut();

    virtual const char*	type() const		{ return getType(); }
    virtual Seis::GeomType geomType() const	{ return getGeomType(); }

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

#endif

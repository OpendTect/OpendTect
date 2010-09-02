#ifndef gmtarray2dinterpol_h
#define gmtarray2dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id: gmtarray2dinterpol.h,v 1.3 2010-09-02 07:01:53 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "array2dinterpol.h"

#include "bufstring.h"
#include "iopar.h"
#include "strmdata.h"


mClass GMTArray2DInterpol : public Array2DInterpol
{
public:
    				GMTArray2DInterpol();
    				~GMTArray2DInterpol();

    virtual void		setPar(const IOPar&)		=0;
    virtual bool		mkCommand(BufferString&)	=0;

protected:
    od_int64			nrIterations() const;
    od_int64			totalNr() const		{ return nrrows_; }
    od_int64			nrDone() const		{ return nrdone_; }
    const char*			message() const;
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);
    int				maxNrThreads() const	{ return 1; }

    int				nrdone_;
    BufferString		msg_;
    IOPar			iopar_;
    StreamData			sd_;
    StreamData			sdmask_;
    BufferString		path_;
    BufferString		defundefpath_;
    bool*			nodes_;
};


mClass GMTSurfaceGrid : public GMTArray2DInterpol
{
public:
    				GMTSurfaceGrid();

    static const char*		sType();
    const char*			type() const		{ return sType(); }
    static void			initClass();
    static Array2DInterpol*	create();

    void			setPar(const IOPar&);
    bool			mkCommand(BufferString&);
    const char*			infoMsg() const;
};


mClass GMTNearNeighborGrid : public GMTArray2DInterpol
{
public:
    				GMTNearNeighborGrid();

    static const char*		sType();
    const char*			type() const		{ return sType(); }
    static void			initClass();
    static Array2DInterpol*	create();

    void			setPar(const IOPar&);
    bool			mkCommand(BufferString&);
    const char*			infoMsg() const;
};

#endif

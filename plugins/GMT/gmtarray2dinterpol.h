#ifndef gmtarray2dinterpol_h
#define gmtarray2dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id: gmtarray2dinterpol.h,v 1.5 2012-08-03 13:01:31 cvskris Exp $
________________________________________________________________________

-*/

#include "gmtmod.h"
#include "array2dinterpol.h"

#include "bufstring.h"
#include "strmdata.h"


mClass(GMT) GMTArray2DInterpol : public Array2DInterpol
{
public:
    				GMTArray2DInterpol();
    				~GMTArray2DInterpol();

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
    StreamData			sd_;
    StreamData			sdmask_;
    BufferString		path_;
    BufferString		defundefpath_;
    bool*			nodes_;
};


mClass(GMT) GMTSurfaceGrid : public GMTArray2DInterpol
{
public:
    				GMTSurfaceGrid();

    static const char*		sType();
    const char*			type() const		{ return sType(); }
    static void			initClass();
    static Array2DInterpol*	create();

    bool			mkCommand(BufferString&);
    const char*			infoMsg() const;

    void			setTension(float);
    bool			usePar(const IOPar&);
    bool			fillPar(IOPar&) const;

protected:
    float			tension_;
};


mClass(GMT) GMTNearNeighborGrid : public GMTArray2DInterpol
{
public:
    				GMTNearNeighborGrid();

    static const char*		sType();
    const char*			type() const		{ return sType(); }
    static void			initClass();
    static Array2DInterpol*	create();

    void			setRadius(float);
    bool			mkCommand(BufferString&);
    const char*			infoMsg() const;
    
    bool			usePar(const IOPar&);
    bool			fillPar(IOPar&) const;

protected:
    float			radius_;
};

#endif


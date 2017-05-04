#ifndef gmtarray2dinterpol_h
#define gmtarray2dinterpol_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
 RCS:           $Id$
________________________________________________________________________

-*/

#include "gmtmod.h"
#include "array2dinterpol.h"

#include "bufstring.h"
#include "strmdata.h"
#include "factory.h"


mExpClass(GMT) GMTArray2DInterpol : public Array2DInterpol
{ mODTextTranslationClass(GMTArray2DInterpol);
public:
    				GMTArray2DInterpol();
    				~GMTArray2DInterpol();

    virtual bool		mkCommand(BufferString&)	=0;

protected:
    od_int64			nrIterations() const;
    od_int64			totalNr() const		{ return nrrows_; }
    od_int64			nrDone() const		{ return nrdone_; }
    uiString			uiMessage() const;
    bool			doPrepare(int);
    bool			doWork(od_int64,od_int64,int);
    bool			doFinish(bool);
    int				maxNrThreads() const	{ return 1; }

    int				nrdone_;
    uiString		        msg_;
    StreamData			sd_;
    StreamData			sdmask_;
    BufferString		path_;
    BufferString		defundefpath_;
    bool*			nodes_;
};


mExpClass(GMT) GMTSurfaceGrid : public GMTArray2DInterpol
{ mODTextTranslationClass(GMTSurfaceGrid);
public:
	mDefaultFactoryInstantiation(Array2DInterpol,
				 GMTSurfaceGrid, "Continuous curvature(GMT)",
				 tr("Continuous curvature(GMT)"))
    				GMTSurfaceGrid();

    static const char*		sType();
    const char*			type() const		{ return sType(); }
    static Array2DInterpol*	create();

    bool			mkCommand(BufferString&);
    uiString			infoMsg() const;

    void			setTension(float);
    bool			usePar(const IOPar&);
    bool			fillPar(IOPar&) const;
    float			getTension() const { return tension_; }

protected:
    float			tension_;
};


mExpClass(GMT) GMTNearNeighborGrid : public GMTArray2DInterpol
{ mODTextTranslationClass(GMTNearNeighborGrid);
public:
    mDefaultFactoryInstantiation(Array2DInterpol,
				 GMTNearNeighborGrid, "Nearest neighbor(GMT)",
				 tr("Nearest neighbor(GMT)"))
    				GMTNearNeighborGrid();

    static const char*		sType();
    const char*			type() const		{ return sType(); }
    static Array2DInterpol*	create();

    void			setRadius(float);
    bool			mkCommand(BufferString&);
    uiString			infoMsg() const;
    
    bool			usePar(const IOPar&);
    bool			fillPar(IOPar&) const;
    float			 getRadius() const { return radius_; }

protected:
    float			radius_;
};

#endif

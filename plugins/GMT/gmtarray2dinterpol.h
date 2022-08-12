#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          Aug 2010
________________________________________________________________________

-*/

#include "gmtmod.h"
#include "array2dinterpol.h"

#include "bufstring.h"
#include "strmdata.h"
#include "factory.h"

namespace OS { class MachineCommand; }


mExpClass(GMT) GMTArray2DInterpol : public Array2DInterpol
{ mODTextTranslationClass(GMTArray2DInterpol);
public:
				~GMTArray2DInterpol();

protected:
				GMTArray2DInterpol();

    od_int64			nrIterations() const override;
    od_int64			totalNr() const override { return nrrows_; }
    od_int64			nrDone() const override { return nrdone_; }
    uiString			uiMessage() const override;
    bool			doPrepare(int) override;
    bool			doWork(od_int64,od_int64,int) override;
    bool			doFinish(bool) override;
    int				maxNrThreads() const override	{ return 1; }

    int				nrdone_;
    uiString		        msg_;
    od_ostream*			sd_ = nullptr;
    od_ostream*			sdmask_ = nullptr;
    BufferString		path_;
    BufferString		defundefpath_;
    bool*			nodes_;

private:

    virtual bool		fillCommand(OS::MachineCommand&)	= 0;

    BufferString		workingdir_;

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

    uiString			infoMsg() const override;

    void			setTension(float);
    bool			usePar(const IOPar&) override;
    bool			fillPar(IOPar&) const override;
    float			getTension() const { return tension_; }

private:

    bool			fillCommand(OS::MachineCommand&) override;

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
    uiString			infoMsg() const override;

    bool			usePar(const IOPar&) override;
    bool			fillPar(IOPar&) const override;
    float			getRadius() const { return radius_; }

private:

    bool			fillCommand(OS::MachineCommand&) override;

    float			radius_;
};


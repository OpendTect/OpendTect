#ifndef seisscanner_h
#define seisscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Feb 2004
 RCS:		$Id: horizonscanner.h,v 1.1 2005-03-25 15:33:13 cvsnanne Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"
#include "ranges.h"

class IOPar;
class PosGeomDetector;

class HorizonScanner : public Executor
{
public:

			HorizonScanner(const char* fnm);
			HorizonScanner(const BufferStringSet& fnms);
			~HorizonScanner();

    const char*		message() const;
    int			totalNr() const;
    int			nrDone() const;
    const char*		nrDoneText() const;

    void		setUndefValue(float udf)	{ udfval = udf; }
    void		setPosIsXY(bool yn)		{ isxy = yn; }
    bool		posIsXY() const			{ return isxy; }
    bool		needZScaling() const		{ return doscale; }
    bool		analyzeData();

    StepInterval<int>	inlRg() const;
    StepInterval<int>	crlRg() const;

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(IOPar&) const;

protected:

    int			nextStep();

    void		init();

    int			chnksz;
    mutable int		totalnr;
    PosGeomDetector&	geomdetector;
    BufferStringSet	filenames;

    bool		firsttime;
    bool		isxy;
    bool		doscale;
    float		udfval;
    TypeSet<Interval<float> > valranges;

};


#endif

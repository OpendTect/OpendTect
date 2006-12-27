#ifndef horizonscanner_h
#define horizonscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		Feb 2004
 RCS:		$Id: horizonscanner.h,v 1.7 2006-12-27 15:28:46 cvsnanne Exp $
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

    virtual const char*	message() const;
    virtual int		totalNr() const;
    virtual int		nrDone() const;
    virtual const char*	nrDoneText() const;

    void		setUndefValue(float udf)	{ udfval = udf; }
    void		setPosIsXY(bool yn)		{ isxy = yn; }
    bool		posIsXY() const			{ return isxy; }
    bool		needZScaling() const		{ return doscale; }
    bool		analyzeData();

    int			nrPositions() const;
    StepInterval<int>	inlRg() const;
    StepInterval<int>	crlRg() const;
    bool		gapsFound(bool inl) const;
    int			nrAttribValues() const;

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(IOPar&) const;

protected:
    virtual int		nextStep();

    void		init();

    int			chnksz;
    mutable int		totalnr;
    PosGeomDetector&	geomdetector;
    BufferStringSet	filenames;
    BufferStringSet	rejectedlines;

    bool		firsttime;
    bool		isxy;
    bool		doscale;
    float		udfval;
    int			nrattribvals;
    TypeSet<Interval<float> > valranges;
};


#endif

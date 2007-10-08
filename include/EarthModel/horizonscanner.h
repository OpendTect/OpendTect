#ifndef horizonscanner_h
#define horizonscanner_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	N. Hemstra
 Date:		Feb 2004
 RCS:		$Id: horizonscanner.h,v 1.8 2007-10-08 12:07:14 cvsraman Exp $
________________________________________________________________________

-*/

#include "executor.h"
#include "bufstringset.h"
#include "ranges.h"

class IOPar;
class PosGeomDetector;
class BinIDValueSet;
namespace EM { class Horizon3DAscIO; }
namespace Table { class FormatDesc; }

class HorizonScanner : public Executor
{
public:

			HorizonScanner(const BufferStringSet& fnms,
					Table::FormatDesc& fd, bool isgeom);
			~HorizonScanner();

    virtual const char*	message() const;
    virtual int		totalNr() const;
    virtual int		nrDone() const;
    virtual const char*	nrDoneText() const;

    bool		reInitAscIO(const char*);
    void		setUndefValue(float udf)	{ udfval_ = udf; }
    void		setPosIsXY(bool yn)		{ isxy_ = yn; }
    bool		posIsXY() const			{ return isxy_; }
    bool		analyzeData();

    int			nrPositions() const;
    StepInterval<int>	inlRg() const;
    StepInterval<int>	crlRg() const;
    bool		gapsFound(bool inl) const;

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(IOPar&) const;

    ObjectSet<BinIDValueSet> getSections()		{ return sections_; }

protected:
    virtual int		nextStep();

    void		init();

    mutable int		totalnr_;
    PosGeomDetector&	geomdetector_;
    EM::Horizon3DAscIO*	ascio_;
    BufferStringSet	filenames_;
    int			fileidx_;
    BufferStringSet	rejectedlines_;

    bool		firsttime_;
    bool		isgeom_;
    bool		isxy_;
    bool		doscale_;
    float		udfval_;
    TypeSet<Interval<float> > valranges_;
    Table::FormatDesc&	fd_;

    BinIDValueSet*	bvalset_;
    ObjectSet<BinIDValueSet> sections_;
};


#endif

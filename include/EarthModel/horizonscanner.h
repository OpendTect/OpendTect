#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		Feb 2004
________________________________________________________________________

-*/

#include "emcommon.h"
#include "executor.h"
#include "bufstringset.h"
#include "ranges.h"

class BinnedValueSet;
class od_istream;
namespace EM { class Horizon3DAscIO; }
namespace Table { class FormatDesc; }
namespace PosInfo { class Detector; }

/*!\brief Executor to scan horizons. */

mExpClass(EarthModel) HorizonScanner : public Executor
{ mODTextTranslationClass(HorizonScanner);
public:

			HorizonScanner(const BufferStringSet& fnms,
					Table::FormatDesc& fd, bool isgeom);
			~HorizonScanner();

    virtual uiString	message() const;
    virtual od_int64	totalNr() const;
    virtual od_int64	nrDone() const;
    virtual uiString	nrDoneText() const;

    bool		reInitAscIO(const char*);
    void		setPosIsXY(bool yn)		{ isxy_ = yn; }
    bool		posIsXY() const			{ return isxy_; }
    bool		analyseData();

    int			nrPositions() const;
    StepInterval<int>	inlRg() const;
    StepInterval<int>	crlRg() const;
    bool		gapsFound(bool inl) const;

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(IOPar&) const;

    const ObjectSet<BinnedValueSet>& getSections()	{ return sections_; }

protected:

    virtual int		nextStep();

    void		init();

    mutable int		totalnr_;
    int			nrdone_;
    PosInfo::Detector&	dtctor_;
    EM::Horizon3DAscIO*	ascio_;
    BufferStringSet	filenames_;
    int			fileidx_;
    BufferStringSet	rejectedlines_;

    bool		firsttime_;
    bool		isgeom_;
    bool		isxy_;
    bool		selxy_;
    bool		doscale_;
    TypeSet<Interval<float> > valranges_;
    Table::FormatDesc&	fd_;
    od_istream*		strm_;

    BinnedValueSet*	bvalset_;
    ObjectSet<BinnedValueSet> sections_;

    mutable uiString	curmsg_;
};

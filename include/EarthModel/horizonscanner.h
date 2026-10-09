#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "task.h"
#include "atomic.h"
#include "bufstringset.h"
#include "ranges.h"

class BinIDValueSet;
class ZAxisTransform;
namespace EM { class Horizon3DAscIO; }
namespace Table { class FormatDesc; }
namespace PosInfo { class Detector; }
namespace ZDomain { class Info; }

/*!
\brief Executor to scan horizons.
*/

mExpClass(EarthModel) HorizonScanner : public ReportingTask
{ mODTextTranslationClass(HorizonScanner);
public:
			HorizonScanner(const BufferStringSet& fnms,
					Table::FormatDesc& fd, bool isgeom,
					const ZDomain::Info&);
    mDeprecatedDef	HorizonScanner(const BufferStringSet& fnms,
					Table::FormatDesc& fd, bool isgeom);
			~HorizonScanner();

    bool		execute() override;

    uiString		uiMessage() const override;
    od_int64		totalNr() const override;
    od_int64		nrDone() const override;
    uiString		uiNrDoneText() const override;

    bool		reInitAscIO(const char*);
    void		setPosIsXY(bool yn)		{ isxy_ = yn; }
    bool		posIsXY() const			{ return isxy_; }
    bool		analyzeData();

    int			nrPositions() const;
    StepInterval<int>	inlRg() const;
    StepInterval<int>	crlRg() const;
    bool		gapsFound(bool inl) const;

    static const char*	defaultUserInfoFile();
    void		launchBrowser(const char* fnm=0) const;
    void		report(StringPairSet&) const;
    mDeprecated("Use with a StringPairSet")
    void		report(IOPar&) const;

    const ObjectSet<BinIDValueSet>& getSections()	{ return sections_; }

protected:

    void			getConvValue(float&);

    void			init();
    bool			importFile(const char*);
    bool			isInsideSurvey(const BinID&,float) const;

    Threads::Atomic<od_int64>	totalnr_	= -1;
    Threads::Atomic<od_int64>	nrdone_		= -1;
    PosInfo::Detector&		dtctor_;
    EM::Horizon3DAscIO*		ascio_		= nullptr;
    BufferStringSet		filenames_;
    BufferStringSet		rejectedlines_;
    bool			isgeom_;
    bool			isxy_		= false;
    bool			selxy_		= false;
    bool			doscale_	= false;
    TypeSet<Interval<float> >	valranges_;
    Table::FormatDesc&		fd_;

    ObjectSet<BinIDValueSet>	sections_;

    const ZDomain::Info&	zinfo_;

    mutable uiString		curmsg_;
};

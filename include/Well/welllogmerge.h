#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Khushnood Qadir
 Date:		May 2020
________________________________________________________________________

-*/

#include "wellmod.h"

#include "enums.h"
#include "executor.h"
#include "bufstringset.h"
#include "multiid.h"
#include "uistring.h"

namespace Well
{
    class Data;
    class Log;
    class LogSampler;
    class LogSet;


mExpClass(Well) LogMerger : public Executor
{ mODTextTranslationClass(LogMerger)
public:
				LogMerger(const MultiID&,
					  const BufferStringSet&,
					  Log&);
				~LogMerger();


    static const char* sKeyAverage()	{ return "Use average"; }
    static const char* sKeyOneLog()	{ return "Use one log"; }
    static uiString sAverage()		{ return tr("Use average"); }
    static uiString sOneLog()		{ return tr("Use one log"); }

    enum OverlapAction		{ UseAverage, UseOneLog };
				mDeclareEnumUtils(OverlapAction)

    void		setSamplingDist(float);
    void		setOverlapAction(OverlapAction);
    void		setDoInterpolation(bool);
    void		setDoExtrapolation(bool);

private:

    OverlapAction		overlapaction_;

    Data*			wd_ = nullptr;

    BufferStringSet		lognms_;
    Log&			outputlog_;
    LogSampler*			logsamp_=nullptr;
    uiString			msg_;
    float			zsampling_;
    bool			interpolate_=true;
    bool			extrapolate_=true;
    od_int64			nrdone_=0;

    int				nextStep() override;
    od_int64			totalNr() const override;
    od_int64			nrDone() const override      { return nrdone_; }
    uiString			uiMessage() const override   { return msg_; }
    uiString			uiNrDoneText() const override
				    { return tr("merging"); }

    bool			goImpl(od_ostream*,bool,bool,int) override;

    int				prepare();
    int				merge();
    int				extrapolateOutLog();
};

} // namespace Well

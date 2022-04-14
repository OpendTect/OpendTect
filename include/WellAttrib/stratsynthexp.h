#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2013
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "executor.h"
#include "uistring.h"

class IOObj;
class SeparString;
class SyntheticData;
class SeisTrcWriter;

namespace PosInfo { class Line2DPos; }


namespace StratSynth
{


mExpClass(WellAttrib) Exporter : public Executor
{ mODTextTranslationClass(Exporter);
public:
				Exporter(const ObjectSet<const SyntheticData>&,
					 Pos::GeomID,const SeparString&,
					 bool replaceudf=false);
				~Exporter();

    od_int64			nrDone() const override;
    od_int64			totalNr() const override;
    uiString			uiNrDoneText() const override
				{ return tr("Data Sets Created"); }
    uiString			uiMessage() const override;

private:

    int				nextStep() override;
    int				writePostStackTrace();
    int				writePreStackTraces();
    bool			prepareWriter();

    const ObjectSet<const SyntheticData>& sds_;
    const Pos::GeomID			geomid_;
    const TypeSet<PosInfo::Line2DPos>*	linepos_ = nullptr;
    SeisTrcWriter*			writer_ = nullptr;
    BufferString			prefixstr_;
    BufferString			postfixstr_;
    const bool				replaceudf_;
    uiString				errmsg_;
    int					cursdidx_ = 0;
    int					posdone_ = 0;
    int					postobedone_ = 0;

};

} // namespace StratSynth

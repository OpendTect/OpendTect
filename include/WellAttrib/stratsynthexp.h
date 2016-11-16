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

namespace PosInfo { class Line2DData; }


mExpClass(WellAttrib) StratSynthExporter : public Executor
{ mODTextTranslationClass(StratSynthExporter);
public:
				StratSynthExporter(
				    const ObjectSet<const SyntheticData>& sds,
				    Pos::GeomID geomid,
				    PosInfo::Line2DData* newgeom,
				    const SeparString&);
				~StratSynthExporter();

    od_int64				nrDone() const;
    od_int64				totalNr() const;
    uiString				nrDoneText() const
					{ return tr("Data Sets Created"); }
    uiString				message() const;
protected:

    int				nextStep();
    int					writePostStackTrace();
    int					writePreStackTraces();
    bool				prepareWriter();

    bool				isps_;
    const ObjectSet<const SyntheticData>& sds_;
    Pos::GeomID				geomid_;
    PosInfo::Line2DData*		linegeom_;
    SeisTrcWriter*			writer_;
    BufferString			prefixstr_;
    BufferString			postfixstr_;
    uiString				errmsg_;
    int					cursdidx_;
    int					posdone_;
    int					postobedone_;
};

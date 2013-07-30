#ifndef stratsynthexp_h
#define stratsynthexp_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Satyaki Maitra
 Date:		July 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "wellattribmod.h"
#include "executor.h"

class IOObj;
class SeparString;
class SyntheticData;
class SeisTrcWriter;

namespace PosInfo { class Line2DData; }


mExpClass(WellAttrib) StratSynthExporter : public Executor
{
public:
    				StratSynthExporter(
			    	    const IOObj& outobj,
				    const ObjectSet<const SyntheticData>& sds,
				    const PosInfo::Line2DData& geom,
				    const SeparString&);
    				~StratSynthExporter();

    od_int64				nrDone() const;
    od_int64				totalNr() const;
protected:

    int 				nextStep();
    int					writePostStackTrace();
    int					writePreStackTraces();
    void				prepareWriter();

    bool				isps_;
    const ObjectSet<const SyntheticData>& sds_;
    const PosInfo::Line2DData&		linegeom_;
    const IOObj&			outobj_;
    SeisTrcWriter*			writer_;
    const SeparString&			prepostfixstr_;
    int					cursdidx_;
    int					posdone_;
};

#endif



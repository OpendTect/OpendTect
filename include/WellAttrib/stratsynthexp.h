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
namespace PosInfo { class Line2DData; }
namespace SynthSeis { class DataSet; }
namespace Seis { class Storer; }


mExpClass(WellAttrib) StratSynthExporter : public Executor
{ mODTextTranslationClass(StratSynthExporter);
public:

    mExpClass(WellAttrib) Setup
    {
    public:

			    Setup( Pos::GeomID geomid )
				: geomid_(geomid)	{}

	const Pos::GeomID   geomid_;
	BufferString	    prefix_;
	BufferString	    postfix_;
	bool		    replaceudfs_		= false;

    };

    typedef SynthSeis::DataSet		DataSet;
    typedef ObjectSet<const DataSet>	DataSetSet;
    typedef PosInfo::Line2DData		Line2DData;

			StratSynthExporter(const Setup&,const DataSetSet&,
					    const Line2DData&);
			~StratSynthExporter();

    od_int64		nrDone() const;
    od_int64		totalNr() const;
    uiString		nrDoneText() const { return tr("Data Sets Created"); }
    uiString		message() const;
protected:

    int			nextStep();
    int			writePostStackTrace();
    int			writePreStackTraces();
    bool		prepareStorer();
    void		prepTrc4Store(SeisTrc&) const;

    const Setup		setup_;
    const DataSetSet&	sds_;
    const Line2DData&	linegeom_;
    Seis::Storer*	storer_;
    BufferString	prefixstr_;
    BufferString	postfixstr_;
    uiString		errmsg_;
    int			cursdsidx_;
    int			posdone_;
    int			postobedone_;

};

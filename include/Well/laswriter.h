#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "executor.h"

#include "bufstringset.h"
#include "ptrman.h"
#include "ranges.h"

namespace Well { class Data; class LogSet; }

mExpClass(Well) LASWriter : public Executor
{
public:
			LASWriter(const Well::Data&,
				  const BufferStringSet& lognms,
				  const char* lasfnm);
			~LASWriter();

    void		setNullValue(const char*);
    void		setMDRange(const StepInterval<float>&);
    void		setZInFeet( bool yn )		{ zinfeet_ = yn; }
    void		setColumnWidth( int w )		{ columnwidth_ = w; }
    void		writeLogName(bool yn)		{ writelognm_ = yn; }

    od_int64		totalNr() const  override	{ return totalnr_; }
    od_int64		nrDone() const override		{ return nrdone_; }

protected:
    int			nextStep() override;

    bool		writeVersionInfoSection(od_ostream&);
    bool		writeWellInfoSection(od_ostream&);
    bool		writeCurveInfoSection(od_ostream&);
    bool		writeParameterInfoSection(od_ostream&);
    bool		writeOtherSection(od_ostream&);
    bool		writeLogData(od_ostream&);

    ConstRefMan<Well::Data>	wd_;
    Well::LogSet&	logs_;
    BufferStringSet	lognms_;
    BufferString	lasfnm_;

    StepInterval<float> mdrg_;
    BufferString	nullvalue_;
    bool		zinfeet_			= false;
    bool		writelognm_			= false;

    od_int64		totalnr_			= 1;
    od_int64		nrdone_				= 0;

    int			columnwidth_			= 12;
};

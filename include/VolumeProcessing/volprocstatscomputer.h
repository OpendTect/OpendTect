#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Helene Huck
 Date:		September 2015
________________________________________________________________________


-*/


#include "volumeprocessingmod.h"
#include "volprocstep.h"
#include "trckeyzsampling.h"
#include "paralleltask.h"

template<class T> class Array3D;

namespace VolProc
{

/*!
\brief Processor for Volume Statistics
*/

mExpClass(VolumeProcessing) StatsCalculator : public Step
{ mODTextTranslationClass(StatsCalculator);
public:
				mDefaultFactoryInstantiation(
					Step, StatsCalculator,
					"Volume Statistics",
					tr("Volume Statistics") )

				StatsCalculator();

    static const char*		sZSampMargin()	{ return "Z samples margin"; }

    void			setStepout( BinID bid ) { stepout_ = bid; }
    void			setZMargin( int nsamp ) { nzsampextra_ = nsamp;}
    void			setStatsType( BufferString str )
							{ statstype_ = str; }

    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);

private:

    ReportingTask*		createTask();

    virtual bool		needsInput() const		{ return true; }
    virtual bool		needsFullVolume() const		{ return false;}
    virtual bool		canInputAndOutputBeSame() const { return false;}
    virtual bool		areSamplesIndependent() const	{ return true; }
    virtual bool		copyComponentsSel(const InputSlotID,
						  OutputSlotID&) const;

    virtual BinID		getHorizontalStepout() const
							    { return stepout_; }
    virtual int			getVerticalStepout() const	{ return 0; }
    virtual od_int64		extraMemoryUsage(OutputSlotID,
						 const TrcKeyZSampling&) const
				{ return 0; }

    BinID			stepout_;
    int				nzsampextra_;	//extra on both sides
    BufferString		statstype_;

};


mClass(VolumeProcessing) StatsCalculatorTask : public ParallelTask
{ mODTextTranslationClass(StatsCalculatorTask);
public:

				StatsCalculatorTask(const Array3D<float>&,
						const TrcKeyZSampling& tkzsin,
						const TrcKeyZSampling& tkzsout,
						BinID,int,BufferString,
						Array3D<float>& out);

    static const char*		sKeyEllipse()	{ return "Ellipse"; }
				//for now

    od_int64			totalNr() const		{ return totalnr_; }
    uiString			message() const;

protected:

    bool			doWork(od_int64,od_int64,int);
    void			prepareWork();

    od_int64			nrIterations() const	{ return totalnr_; }

    od_int64			totalnr_;

    const Array3D<float>&	input_;
    Array3D<float>&		output_;

    BinID			stepout_;
    int				nzsampextra_;	//extra on both sides
    BufferString		statstype_;
    BufferString		shape_;

    TypeSet<BinID>		positions_;

    TrcKeyZSampling		tkzsin_;
    TrcKeyZSampling		tkzsout_;
};

} // namespace VolProc

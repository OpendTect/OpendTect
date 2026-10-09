#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "earthmodelmod.h"

#include "datapointset.h"
#include "emhorizon3d.h"
#include "task.h"

class TaskRunner;
namespace EM { class Horizon3D; class EMObjectIterator; }


// Will calculate the distance between hor2 and hor1 (hor2-hor1).
// When signed_=false (default) the result is the absolute value

mExpClass(EarthModel) IsochronMaker : public SequentialTask
{ mODTextTranslationClass(IsochronMaker)
public:
			IsochronMaker(const EM::Horizon3D& hor1,
				      const EM::Horizon3D& hor2,
				      const char* attrnm,int dataidx,
				      DataPointSet* dps=nullptr);
			~IsochronMaker();

    uiString		uiMessage() const override;
    uiString		uiNrDoneText() const override
			{ return tr("Positions handled"); }

    void		useSignedValue( bool yn )	{ signed_ = yn; }
    void		setUnits( const bool isinmsc )	{ inmsec_ = isinmsc; }

    static const char*	sKeyHorizonID();
    static const char*	sKeyCalculateToHorID();
    static const char*	sKeyAttribName();
    static const char*	sKeyOutputInMilliSecYN();
    static const char*	sKeyIsOverWriteYN();

private:

    bool		doPrepare(od_ostream* =nullptr) override;
    int			nextStep() override;
    bool		doFinish(bool success,od_ostream* =nullptr) override;

    od_int64		nrDone() const override		{ return nrdone_; }
    od_int64		totalNr() const override	{ return totnr_; }

    int				totnr_;
    od_int64			nrdone_			= 0;
    mutable uiString		msg_;

    int				sidcolidx_		= mUdf(int);
    int				dataidx_;
    ConstRefMan<EM::Horizon3D>	hor1_;
    ConstRefMan<EM::Horizon3D>	hor2_;
    RefMan<DataPointSet>	dps_;
    EM::EMObjectIterator*	iter_;
    bool			inmsec_			= false;
    bool			signed_			= false;
};

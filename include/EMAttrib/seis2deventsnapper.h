#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Satyaki Maitra
 Date:          October 2009
________________________________________________________________________

-*/

#include "emattribmod.h"
#include "executor.h"
#include "seiseventsnapper.h"
#include "seistrc.h"
#include "seisread.h"
#include "posinfo2dsurv.h"

namespace EM { class Hor2DSeisLineIterator; class Horizon2D; }
namespace Seis { class Horizon2D; }
class SeisTrcReader;
class IOObj;

/*!
\brief SeisEventSnapper for 2D.
*/

mExpClass(EMAttrib) Seis2DLineEventSnapper : public SeisEventSnapper
{
public:

    mExpClass(EMAttrib) Setup
    {
    public:
				Setup(const IOObj* seisobj,Pos::GeomID gmid,
				      const Interval<float>& gt)
				    : ioobj_(seisobj)
				    , geomid_(gmid)
				    , gate_(gt)	{}
	mDefSetupMemb(const IOObj*,ioobj)
	mDefSetupMemb(Pos::GeomID,geomid)
	mDefSetupMemb(Interval<float>,gate)
    };
				Seis2DLineEventSnapper(const EM::Horizon2D&,
					   EM::Horizon2D&,
					  const Seis2DLineEventSnapper::Setup&);
				~Seis2DLineEventSnapper();

protected:
    int				nextStep() override;

    Pos::GeomID			geomid_;
    SeisTrc			trc_;
    SeisTrcReader*		seisrdr_;
    const EM::Horizon2D&	orghor_;
    EM::Horizon2D&		newhor_;
};


/*!
\brief ExecutorGroup to snap 2D seismic line set event.
*/

mExpClass(EMAttrib) SeisEventSnapper2D : public ExecutorGroup
{
public:

    mExpClass(EMAttrib) Setup
    {
    public:
				Setup(const IOObj* ioobj,int typ,
				      const Interval<float>& gt)
				    : seisioobj_(ioobj)
				    , type_(typ)
				    , gate_(gt) {}
	mDefSetupMemb(const IOObj*,seisioobj)
	mDefSetupMemb(int,type)
	mDefSetupMemb(Interval<float>,gate)
    };
				SeisEventSnapper2D(const EM::Horizon2D*,
					EM::Horizon2D*,const Setup&);
				~SeisEventSnapper2D();
protected:
    int				type_;
    Interval<float>		gate_;
    const EM::Horizon2D*	orghor_;
    EM::Horizon2D*		newhor_;
    EM::Hor2DSeisLineIterator*	hor2diterator_;
};


#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "executor.h"

class IOObj;
class Scaler;
class SeisSingleTraceProc;
class SeisTrc;
class SeisTrcReader;
class SeisTrcWriter;
namespace Seis { class SelData; }
namespace Vel { class Worker; }
namespace ZDomain { class Info; }


/*!\brief Copies cubes. The IOPar constructor wants an IOPar as you would pass
	to a SeisSingleTraceProc. */

mExpClass(Seis) SeisCubeCopier : public Executor
{ mODTextTranslationClass(SeisCubeCopier);
public:

				SeisCubeCopier(const IOObj& inobj,
					    const IOObj& outobj,const IOPar&,
					    int compnr=-1);
				SeisCubeCopier(SeisSingleTraceProc*,
						int compnr=-1);
						//!< trcproc becomes mine
				~SeisCubeCopier();

    od_int64			totalNr() const override;
    od_int64			nrDone() const override;
    uiString			uiMessage() const override;
    uiString			uiNrDoneText() const override;

protected:

    bool			goImpl(od_ostream*,bool,bool,int) override;
    int				nextStep() override;
    bool			resampleVels(const SeisTrc&,SeisTrc&) const;
    mDeprecatedDef
    void			cropComponents(SeisTrc&) const;

    SeisSingleTraceProc*	stp_;
    uiString			errmsg_;
    int				compnr_;
    int				veltype_; //deprecated
    const Vel::Worker*		worker_() const;
    Vel::Worker*		worker_();

    void			setInputCB(CallBacker*);
    void			doProc(CallBacker*);

private:

    bool			inited_ = false;
    bool			init();

};


/*!\brief Copies line sets.

  Expects an IOPar with subpars Line.N.*, each of those containing:
  - sKey::GeomID()
  - sKey::TrcRange()
  - sKey::ZRange()
  Failing to provide one of those will exclude the line.

*/

mExpClass(Seis) Seis2DCopier : public Executor
{ mODTextTranslationClass(Seis2DCopier);
public:


			Seis2DCopier(const IOObj& inobj,const IOObj& outob,
					  const IOPar&);

			~Seis2DCopier();

    od_int64		totalNr() const override	{ return totalnr_; }
    od_int64		nrDone() const override		{ return nrdone_; }
    uiString		uiMessage() const override	{ return msg_; }
    uiString		uiNrDoneText() const override;

protected:

    int				nextStep() override;
    bool			goImpl(od_ostream*,bool,bool,int) override;
    bool			initNextLine();

    const IOObj&		inioobj_;
    const IOObj&		outioobj_;
    SeisTrcReader*		rdr_ = nullptr;
    SeisTrcWriter*		wrr_ = nullptr;
    uiString			msg_;
    ObjectSet<Seis::SelData>	seldatas_;

    Scaler*			scaler_ = nullptr;
    int				lineidx_ = -1;
    od_int64			nrdone_ = 0;
    od_int64			totalnr_ = 0;


};

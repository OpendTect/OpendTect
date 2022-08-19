#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"
#include "bufstring.h"
#include "uistring.h"
#include "multiid.h"
#include "ranges.h"
#include "sets.h"
#include "uistring.h"
#include <iosfwd>
class IOObj;
class Executor;
class CtxtIOObj;
class JobRunner;
class JobDescProv;
class Seis2DDataSet;

/*!\brief Provides job runners and postprocessor for seismic processing.

  If the jobs need to be restartable, fetch an altered copy of the input IOPar
  after you fetched the first runner and store it.

  The sKeySeisOutIDKey is the key in the IOPar that contains the key in the
  IOPar that points to the actual output seismic's IOObj ID. Thus:
  res = iopar.find(sKeySeisOutIDKey); id = iopar.find(res);
  If the IOPar contains no value for this key, "Output.1.Seismic ID" will be
  used. It is needed for 3D, to re-wire the output to temporary storage.

  A SeisJobExecProv object can be used for one job only. But, you may have
  to get a runner more than once to get everything done. When everything is
  done, null will be returned.

  After a runner is finished, you need to execute the postprocessor if it
  is not returned as null.

 */

mExpClass(Seis) SeisJobExecProv
{ mODTextTranslationClass(SeisJobExecProv);
public:

			SeisJobExecProv(const char* prognm,const IOPar&);
    virtual		~SeisJobExecProv();

    bool		isRestart() const	{ return isRestart(iopar_); }
    inline uiString	errMsg() const		{ return errmsg_; }
    const IOPar&	pars() const		{ return iopar_; }

    JobRunner*		getRunner( int nrinlperjob );
    Executor*		getPostProcessor();
    bool		removeTempSeis();

    bool		emitLSFile(const char*) const;
    			//!< Not usable in 'normal' work.
    void		preparePreSet(IOPar&,const char*) const;

    const MultiID&	outputID() const	{ return seisoutid_; }

    static BufferString	getDefTempStorDir(const char* storpth=0);
    static const char*	outputKey(const IOPar&);

    static const char*	sKeySeisOutIDKey();
    static const char*	sKeyOutputLS();
    static const char*	sKeyWorkLS();

    static bool		isRestart(const IOPar&);

protected:

    IOPar&		iopar_;
    IOPar&		outioobjpars_;
    CtxtIOObj&		ctio_;
    bool		is2d_;
    BufferString	seisoutkey_;
    MultiID		seisoutid_;
    MultiID		tmpstorid_;
    const BufferString	progname_;
    mutable uiString	errmsg_;
    int			nrrunners_;
    StepInterval<int>	todoinls_;
    Seis2DDataSet*	outds_;


    JobDescProv*	mk2DJobProv();
    JobDescProv*	mk3DJobProv(int ninlperjob);
    void		getMissingLines(TypeSet<int>&) const;
    MultiID		tempStorID() const;

};

#ifndef seisjobexecprov_h
#define seisjobexecprov_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		April 2002
 RCS:		$Id: seisjobexecprov.h,v 1.1 2004-10-26 12:37:18 bert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "multiid.h"
#include "sets.h"
#include <iosfwd>
class IOPar;
class IOObj;
class Executor;
class CtxtIOObj;
class JobRunner;
class JobDescProv;

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

class SeisJobExecProv
{
public:

			SeisJobExecProv(const char* prognm,const IOPar&);

    const char*		errMsg() const		{ return errmsg_.buf(); }

    const IOPar&	pars() const		{ return iopar_; }
    JobRunner*		getRunner();
    Executor*		getPostProcessor();

    static BufferString	getDefTempStorDir();
    static const char*	sKeySeisOutIDKey;

protected:

    IOPar&		iopar_;
    IOPar&		outioobjpars_;
    CtxtIOObj&		ctio_;
    bool		is2d_;
    BufferString	seisoutkey_;
    MultiID		seisoutid_;
    const BufferString	progname_;
    mutable BufferString errmsg_;
    int			nrrunners_;

    JobDescProv*	mk2DJobProv();
    JobDescProv*	mk3DJobProv();
    void		getMissingLines(TypeSet<int>&,const char*) const;
    MultiID		tempStorID() const;

};


#endif

#ifndef seisjobexecprov_h
#define seisjobexecprov_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		April 2002
 RCS:		$Id: seisjobexecprov.h,v 1.16 2012-08-03 13:00:36 cvskris Exp $
________________________________________________________________________

-*/

#include "seismod.h"
#include "bufstring.h"
#include "multiid.h"
#include "ranges.h"
#include "sets.h"
#include <iosfwd>
class IOPar;
class IOObj;
class Executor;
class CtxtIOObj;
class JobRunner;
class JobDescProv;
class Seis2DLineSet;
template <class T> class TypeSet;

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

mClass(Seis) SeisJobExecProv
{
public:

			SeisJobExecProv(const char* prognm,const IOPar&);
    virtual		~SeisJobExecProv();

    bool		isRestart() const;
    const char*		errMsg() const		{ return errmsg_.str(); }
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

protected:

    IOPar&		iopar_;
    IOPar&		outioobjpars_;
    CtxtIOObj&		ctio_;
    bool		is2d_;
    BufferString	seisoutkey_;
    MultiID		seisoutid_;
    MultiID		tmpstorid_;
    const BufferString	progname_;
    mutable BufferString errmsg_;
    int			nrrunners_;
    StepInterval<int>	todoinls_;
    Seis2DLineSet*	outls_;


    JobDescProv*	mk2DJobProv();
    JobDescProv*	mk3DJobProv(int ninlperjob);
    void		getMissingLines(TypeSet<int>&) const;
    MultiID		tempStorID() const;

};


#endif


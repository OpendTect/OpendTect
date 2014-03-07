#ifndef uiseismmproc_h
#define uiseismmproc_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uimmbatchjobdispatch.h"

class IOObj;
class JobRunner;
class uiGenInput;
class uiIOFileSelect;
class uiSeisIOObjInfo;
class SeisJobExecProv;


mExpClass(uiSeis) uiSeisMMProc : public uiMMBatchJobDispatcher
{
public:

                        uiSeisMMProc(uiParent*,const IOPar&,const char* parfnm);
			~uiSeisMMProc();

protected:

    SeisJobExecProv*	jobprov_;
    uiSeisIOObjInfo*	outioobjinfo_;
    bool		is2d_;
    bool		lsfileemitted_;
    int			nrinlperjob_;
    const BufferString	parfnm_;

    uiIOFileSelect*	tmpstordirfld_;
    uiGenInput*		inlperjobfld_;

    virtual bool	initWork(bool);
    virtual bool	prepareCurrentJob();
    virtual Executor*	getPostProcessor() const;
    virtual bool	removeTmpProcFiles();
    virtual bool	needConfirmEarlyStop() const;
    virtual bool	haveTmpProcFiles() const;

    bool		isRestart() const	{ return !tmpstordirfld_; }

};

#endif


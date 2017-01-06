#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          April 2002
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uimmbatchjobdispatch.h"

class uiCheckBox;
class uiGenInput;
class uiFileInput;
class uiSeisIOObjInfo;
class SeisJobExecProv;


mExpClass(uiSeis) uiSeisMMProc : public uiMMBatchJobDispatcher
{ mODTextTranslationClass(uiSeisMMProc);
public:

			uiSeisMMProc(uiParent*,const IOPar&);
			~uiSeisMMProc();

protected:

    SeisJobExecProv*	jobprov_;
    uiSeisIOObjInfo*	outioobjinfo_;
    const bool		is2d_;
    const BufferString	parfnm_;
    bool		lsfileemitted_;
    int			nrinlperjob_;

    uiFileInput*	tmpstordirfld_;
    uiGenInput*		inlperjobfld_;
    uiCheckBox*		saveasdeffld_;

    virtual bool	initWork(bool);
    virtual bool	prepareCurrentJob();
    virtual Executor*	getPostProcessor() const;
    virtual bool	removeTmpProcFiles();
    virtual bool	needConfirmEarlyStop() const;
    virtual bool	haveTmpProcFiles() const;

    bool		isRestart() const	{ return !tmpstordirfld_; }

};

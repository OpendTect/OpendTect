#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uimmbatchjobdispatch.h"

class uiGenInput;
class uiIOFileSelect;
class uiSeisIOObjInfo;
class SeisJobExecProv;
class uiCheckBox;


mExpClass(uiSeis) uiSeisMMProc : public uiMMBatchJobDispatcher
{ mODTextTranslationClass(uiSeisMMProc);
public:

                        uiSeisMMProc(uiParent*,const IOPar&);
			~uiSeisMMProc();

protected:

    SeisJobExecProv*	jobprov_    = nullptr;
    uiSeisIOObjInfo*	outioobjinfo_ = nullptr;
    const bool		is2d_	    = false;
    const BufferString	parfnm_;
    bool		lsfileemitted_ = false;
    int			nrinlperjob_;

    uiIOFileSelect*	tmpstordirfld_	= nullptr;
    uiGenInput*		inlperjobfld_	= nullptr;
    uiCheckBox*		saveasdeffld_	= nullptr;

    bool	initWork(bool) override;
    bool	prepareCurrentJob() override;
    Executor*	getPostProcessor() const override;
    bool	removeTmpProcFiles() override;
    bool	needConfirmEarlyStop() const override;
    bool	haveTmpProcFiles() const override;

    bool	isRestart() const	{ return !tmpstordirfld_; }

};

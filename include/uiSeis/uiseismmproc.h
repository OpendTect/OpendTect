#pragma once
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

    SeisJobExecProv*	jobprov_;
    uiSeisIOObjInfo*	outioobjinfo_;
    const bool		is2d_;
    const BufferString	parfnm_;
    bool		lsfileemitted_;
    int			nrinlperjob_;

    uiIOFileSelect*	tmpstordirfld_;
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


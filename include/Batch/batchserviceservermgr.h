#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "batchmod.h"

#include "serverservicebase.h"

class BatchProgram;

/*!\brief
  Service manager for all OpendTect batch programs.
  Allows informing od_main on the program of the batch program,
  and getting data required to perform the work from the main application
 */

mExpClass(Batch) BatchServiceServerMgr : public ServiceServerMgr
{ mODTextTranslationClass(BatchServiceServerMgr)
public:

    virtual		~BatchServiceServerMgr();

    static BatchServiceServerMgr&	getMgr();

    bool		isOK() const;

protected:
			BatchServiceServerMgr();

    bool		canParseAction(const char*,uiRetVal&) override;
    bool		canParseRequest(const OD::JSON::Object&,
					uiRetVal&) override;

    uiRetVal		doHandleAction(const char* action) override;
    uiRetVal		doHandleRequest(const OD::JSON::Object&) override;

    void		doPyEnvChange(CallBacker*) override final   {}
    void		doAppClosing(CallBacker*) override;
    void		closeApp() override;

    void		workStartedCB(CallBacker*);
    void		pausedCB(CallBacker*);
    void		resumedCB(CallBacker*);
    void		killedCB(CallBacker*);
    void		workEnded(CallBacker*);

    uiRetVal		lastreport_;

private:

    uiRetVal		sendActionRequest_(const char* action,
					   const OD::JSON::Object* =nullptr);
    void		reportToCheckCB(CallBacker*) override;

    BatchProgram&	bp_;
    friend class BatchProgram;

};

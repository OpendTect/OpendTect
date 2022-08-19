#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"

#include "serverservicebase.h"

class uiMainWin;

namespace sKey
{
    inline StringView NN3D()	{ return "NeuralNetwork3D"; }
    inline StringView NN2D()	{ return "NeuralNetwork2D"; }
    inline StringView UVQ3D()	{ return "NNQuickUVQ3D"; }
    inline StringView UVQ2D()	{ return "NNQuickUVQ2D"; }
};


/*!\
  brief Base class for a GUI service potentially managed by od_main
 */

mExpClass(uiTools) uiServiceServerMgr : public ServiceServerMgr
{ mODTextTranslationClass(uiServiceServerMgr)
public:

    virtual		~uiServiceServerMgr();

    void		setBackground(bool yn);

    static const char*	sKeyHideEv()		{ return "hide"; }
    static const char*	sKeyRaiseEv()		{ return "raise"; }
    static const char*	sKeyStart()		{ return "start"; }
    static const char*	sKeyLogsChanged()	{ return "logschanged"; }

protected:
			uiServiceServerMgr(const char* servicenm,uiMainWin&);
			uiServiceServerMgr(const char* servicenm,uiMainWin&,
					   bool assignport,
					   Network::SpecAddr=Network::Any);

    bool		canParseAction(const char*,uiRetVal&) override;
    bool		canParseRequest(const OD::JSON::Object&,
				       uiRetVal&) override;

    uiRetVal		doHandleAction(const char* action) override;
    uiRetVal		doHandleRequest(const OD::JSON::Object&) override;

    bool		reportingAppIsAlive() const override;
    void		doAppClosing(CallBacker*) override;
    void		closeApp() override;

};

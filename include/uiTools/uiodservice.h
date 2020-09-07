#pragma once
/*+
 * ________________________________________________________________________
 *
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * Author:	Wayne Mogg
 * Date:	Oct 2019
 * RCS:		$Id$
 * ________________________________________________________________________
 *
 * -*/

#include "uitoolsmod.h"

#include "odservicebase.h"

class Timer;
class uiMainWin;

namespace sKey
{
    inline FixedString NN3D()	{ return "NeuralNetwork3D"; }
    inline FixedString NN2D()	{ return "NeuralNetwork2D"; }
    inline FixedString UVQ3D()	{ return "NNQuickUVQ3D"; }
    inline FixedString UVQ2D()	{ return "NNQuickUVQ2D"; }
};


/*!\brief Base class for a GUI service potentially managed by od_main */

mExpClass(uiTools) uiODService : public ODServiceBase
{ mODTextTranslationClass(uiODService)
public:

    typedef int ID;

    virtual		~uiODService();

    bool		isODMainSlave() const;
    bool		isMasterAlive() const;
    void		setBackground(bool yn);

protected:

		    uiODService(uiMainWin&, const char* hostname,
							bool assignport=true);
		    explicit uiODService(uiMainWin&, bool islocal,
			    const char* servernm=nullptr,bool assignport=true);

    uiRetVal		sendAction(const char* act) const;
    uiRetVal		sendRequest(const char* reqkey,
				    const OD::JSON::Object&) const;
    bool		doParseAction(const char*,uiRetVal&) override;
    bool		doParseRequest(const OD::JSON::Object&,
				       uiRetVal&) override;

    void		doAppClosing(CallBacker*) override;

private:
			uiODService(const uiODService&) = delete;
			uiODService(uiODService&&) = delete;

    uiODService&	operator=(const uiODService&) = delete;
    uiODService&	operator=(uiODService &&) = delete;

    void		initService(uiMainWin&,bool);

    uiRetVal		doRegister();
    uiRetVal		doDeRegister();
    void		handleMasterCheckTimer(bool start);
    void		closeApp() override;

    void		doPyEnvChange(CallBacker*) override;
    void		masterCheckCB(CallBacker*);

    Network::Authority	odauth_;
    ID			servid_ = 0;

    Timer*		mastercheck_ = nullptr;

};

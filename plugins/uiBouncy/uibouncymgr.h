#ifndef uibouncymgr_h
#define uibouncymgr_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Aug 2009
 * ID       : $Id: uibouncymgr.h,v 1.2 2009-09-09 07:57:55 cvskarthika Exp $
-*/

#include "callback.h"

class uiODMain;
namespace visBase { class BeachBall; }
namespace Bouncy { class BouncyController; }

namespace uiBouncy
{
class uiBouncyMain;
class uiBouncySettingsDlg;

mClass uiBouncyMgr : public CallBacker
{
public:

    				uiBouncyMgr(uiODMain*);
				~uiBouncyMgr();
    void			doWork(CallBacker*);
    void			addBeachBall();
    int				removeBeachBall();

    // callbacker to show preview of settings change
    void			propertyChangeCB(CallBacker*);
    // callbacker to update ball's position and orientation
    void			newPosAvailableCB(CallBacker*);

protected:

    void			removeAllBeachBalls(CallBacker*);
    void			zScaleCB(CallBacker*);
    void			setBallScale();
    void			startGame();
    void			stopGame();

    void                        sessionSaveCB(CallBacker*);
    void                        sessionRestoreCB(CallBacker*);
    void                        surveyToBeChangedCB(CallBacker*);
    void                        surveyChangeCB(CallBacker*);
    void                        shutdownCB(CallBacker*);

    visBase::BeachBall*		vbb_;
    uiODMain*			appl_;
    int				sceneid_;
    uiBouncyMain*		maindlg_;
    uiBouncySettingsDlg*        settingsdlg_;
    Bouncy::BouncyController*	gamectr_;
};

}; // namespace

#endif

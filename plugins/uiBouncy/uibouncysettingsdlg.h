#ifndef uibouncysettingsdlg_h
#define uibouncysettingsdlg_h
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Karthika
 * DATE     : Sep 2009
 * ID       : $Id$
-*/

#include "uibouncymod.h"
#include "uigroup.h"

class uiGenInput;
class uiColorInput;

namespace visBeachBall { class BallProperties; }

namespace uiBouncy
{

mClass(uiBouncy) uiBouncySettingsDlg : public uiGroup
{
public:

				uiBouncySettingsDlg(uiParent*,
					visBeachBall::BallProperties* = 0);
				~uiBouncySettingsDlg();

    visBeachBall::BallProperties getBallProperties() const;
    void			setBallProperties( 
				    const visBeachBall::BallProperties&);
    bool			isOK();
    Notifier<uiBouncySettingsDlg> propertyChanged;

protected:

    uiGenInput*			radiusfld_;
    uiColorInput*		color1sel_;
    uiColorInput*		color2sel_;
    uiGenInput*         	inlfld_;
    uiGenInput*         	crlfld_;
    uiGenInput*         	xfld_;
    uiGenInput*         	yfld_;

    void			changeCB(CallBacker*);
    void			inl_crlChangedCB(CallBacker*);
    void			x_yChangedCB(CallBacker*);
    bool			binIDOK();

};

};

#endif


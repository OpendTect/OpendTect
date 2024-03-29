#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"

#include "uigroup.h"
#include "odcommonenums.h"

mFDQtclass(QButtonGroup)
class uiButton;
class i_ButtonGroupMessenger;

/*\brief Group managing buttons, and their position.

If the buttons have an on/off state, then you should consider the
setExclusive() switch, which is by default ON.

*/


mExpClass(uiBase) uiButtonGroup : public uiGroup
{
public:
			uiButtonGroup(uiParent*,const char* nm,
				      OD::Orientation);
			~uiButtonGroup();
			mOD_DisableCopy(uiButtonGroup)

    void		selectButton(int id);
    int			selectedId() const;
    int			nrButtons() const;
    void		setSensitive(int id,bool yn);

    void		displayFrame(bool);
    bool		isFrameDisplayed() const;
    void		setExclusive(bool);
    bool		isExclusive() const;
    void		nextButtonOnNewRowCol();

    int			addButton(uiButton*);
			//!< Only use if you need ID. Then, set 0 as parent
			//!< when constructing the button.

    Notifier<uiButtonGroup> valueChanged;

protected:

    mQtclass(QButtonGroup*)	qbuttongrp_;
    ObjectSet<uiButton>		uibuts_;
    OD::Orientation		orientation_;
    bool			newrowcol_;
    i_ButtonGroupMessenger*	msgr_;
};

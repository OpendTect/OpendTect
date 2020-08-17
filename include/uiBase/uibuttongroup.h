#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          18/08/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uigroup.h"

mFDQtclass(QButtonGroup)
class uiButton;

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

    void		selectButton(int id);
    int			selectedId() const;
    int			nrButtons() const;
    void		setSensitive(int id,bool yn=true);

    void		displayFrame(bool);
    bool		isFrameDisplayed() const;
    void		setExclusive(bool);
    bool		isExclusive() const;
    void		nextButtonOnNewRowCol();

    int			addButton(uiButton*);
			//!< Only use if you need ID. Then, set 0 as parent
			//!< when constructing the button.

protected:

    mQtclass(QButtonGroup*)	qbuttongrp_;
    ObjectSet<uiButton>		uibuts_;
    OD::Orientation		orientation_;
    bool			newrowcol_;

};


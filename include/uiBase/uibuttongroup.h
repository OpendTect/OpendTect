#ifndef uibuttongroup_h
#define uibuttongroup_h

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

/* Group managing buttons, and their position.

If the buttons have an on/off state, then you should consider the setExclusive() switch, which is by default ON.

   */


mExpClass(uiBase) uiButtonGroup : public uiGroup
{ 	
public:
			uiButtonGroup(uiParent*,const char* nm="uiButtonGrp",
				      bool vertical=true);
			~uiButtonGroup();

    void		selectButton(int id);
    int			selectedId() const;
    int			nrButtons() const;
    void		setSensitive(int id,bool yn=true);

    void		displayFrame(bool);
    bool		isFrameDisplayed() const;
    void		setExclusive(bool);
    bool		isExclusive() const;

    int			addButton(uiButton*);
    			//!< Only use if you need ID. Then, set 0 as parent
    			//!< when constructing teh button.

protected:

    mQtclass(QButtonGroup*)	qbuttongrp_;
    ObjectSet<uiButton>		uibuts_;
    bool			vertical_;

};

#endif


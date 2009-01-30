#ifndef uibuttongroup_h
#define uibuttongroup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          18/08/2001
 RCS:           $Id: uibuttongroup.h,v 1.11 2009-01-30 05:08:31 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uigroup.h"

class QButtonGroup;
class uiButton;


mClass uiButtonGroup : public uiGroup
{ 	
public:
			uiButtonGroup(uiParent*,const char* nm="uiButtonGrp",
				      bool vertical=true);
			~uiButtonGroup();

    void		addButton(uiButton*);
    void		selectButton(int id);
    int			selectedId() const;
    int			nrButtons() const;
    void		setSensitive(int id,bool yn=true);

    void		displayFrame(bool);
    bool		isFrameDisplayed() const;
    void		setExclusive(bool);
    bool		isExclusive() const;

protected:

    QButtonGroup*	qbuttongrp_;
    uiButton*		prevbutton_;
    bool		vertical_;
};

#endif

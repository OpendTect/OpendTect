#ifndef uibuttongroup_h
#define uibuttongroup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          18/08/2001
 RCS:           $Id: uibuttongroup.h,v 1.13 2009-02-10 14:16:35 cvsbert Exp $
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

    int			addButton(uiButton*);
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
    ObjectSet<uiButton>	uibuts_;
    bool		vertical_;

};

#endif

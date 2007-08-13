#ifndef uibuttongroup_h
#define uibuttongroup_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          18/08/2001
 RCS:           $Id: uibuttongroup.h,v 1.9 2007-08-13 12:48:58 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiobj.h"
#include "uiparent.h"
#include "callback.h"

class uiButtonGroupBody;

class uiButtonGroup;
class uiButtonGroupObjBody;
class uiButtonGroupParentBody;

class uiButtonGroupObj : public uiObject
{ 	
friend class uiButtonGroup;
protected:
			uiButtonGroupObj( uiButtonGroup*, uiParent*, 
					  const char* nm, bool vertical=true, 
					  int strips=1 );
public:

    virtual		~uiButtonGroupObj();
    
    const ObjectSet<uiObjHandle>* childList() const;

protected:
    uiButtonGroupObjBody*	body_;
    uiButtonGroup*		uibutgrp_;

    void		bodyDel( CallBacker* );
    void		grpDel( CallBacker* );
};


class uiButtonGroup : public uiParent
{ 	
friend class uiButtonGroupObj;

public:
			uiButtonGroup( uiParent*, const char* nm="uiButtonGrp",
				       bool vertical=true, int strips=1 ); 

    virtual		~uiButtonGroup();

    void		selectButton(int id);
    int			selectedId() const;
    int			nrButtons() const;
    void		setSensitive(int id,bool yn=true);

    void		displayFrame(bool);
    bool		isFrameDisplayed() const;
    void		setRadioButtonExclusive(bool);
    bool		isRadioButtonExclusive() const;

protected:

    uiButtonGroupObj*		grpobj_;
    uiButtonGroupParentBody*	body_;

    virtual uiObject*		mainobject()	{ return grpobj_; }
};

#endif

#ifndef uibuttongroup_H
#define uibuttongroup_H

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          18/08/2001
 RCS:           $Id: uibuttongroup.h,v 1.4 2003-11-07 12:21:54 bert Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <uiparent.h>
#include <callback.h>

class uiButtonGroupBody;

class uiButtonGroup;
class uiButtonGroupObjBody;
class uiButtonGroupParentBody;

class uiButtonGroupObj : public uiObject
{ 	
friend class uiButtonGroup;
protected:
			uiButtonGroupObj( uiParent*, const char* nm,
					  bool vertical = true, int strips= 1 );
private:

//    uiButtonGroupObjBody*	mkbody(uiParent*,const char*);
    uiButtonGroupObjBody*	body_;
};


class uiButtonGroup : public uiParent
{ 	
public:
			uiButtonGroup( uiParent*, const char* nm="uiButtonGrp",
				    bool vertical = true, int strips = 1 ); 

protected:

    uiButtonGroupObj*		grpobj_;
    uiButtonGroupParentBody*	body_;

    virtual uiObject*		mainobject()	{ return grpobj_; }

};

#endif

#ifndef uibuttongroup_H
#define uibuttongroup_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          18/08/2001
 RCS:           $Id: uibuttongroup.h,v 1.1 2001-08-23 15:02:41 windev Exp $
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

    inline uiButtonGroupObj*	uiObj()			    { return grpobj_; }
    inline const uiButtonGroupObj* uiObj() const	    { return grpobj_; }

    void		attach( constraintType t, int margin=-1)
			    { grpobj_->attach(t,margin); }
    void		attach( constraintType t, uiObject* oth, int margin=-1)
			    { grpobj_->attach(t,oth,margin); }
    void		attach( constraintType t, uiGroup* oth, int margin=-1)
			    { grpobj_->attach(t,oth,margin); }
    void		attach( constraintType t, uiButtonGroup* oth, int m=-1)
			    { grpobj_->attach(t,oth,m); }

protected:

    uiButtonGroupObj*		grpobj_;
    uiButtonGroupParentBody*	body_;
};

#endif

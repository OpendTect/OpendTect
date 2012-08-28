#ifndef uigroup_h
#define uigroup_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uigroup.h,v 1.46 2012-08-28 05:13:40 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uiparent.h"
#include "callback.h"

class IOPar;

class uiGroupBody;
class uiParentBody;

class uiGroup;
class uiGroupObjBody;
class uiGroupParentBody;

mFDQtclass(QWidget)


class uiGroupObj : public uiObject
{ 	
friend class uiGroup;
protected:
			uiGroupObj(uiGroup*,uiParent*,const char*,bool);
public:

    virtual		~uiGroupObj();

    uiGroup*		group() const		{ return uigrp_; }

    const ObjectSet<uiBaseObject>* childList() const;

protected:

    uiGroupObjBody*	body_;
    uiGroup*		uigrp_;

    void		bodyDel( CallBacker* );
    void		grpDel( CallBacker* );
};


mClass(uiBase) uiGroup : public uiParent
{ 	
friend class		uiGroupObjBody;
friend class		uiGroupParentBody;
friend class		uiGroupObj;
friend class		uiMainWin;
friend class		uiTabStack;
public:
			uiGroup( uiParent* , const char* nm="uiGroup", 
				 bool manage=true );
    virtual		~uiGroup();

    inline operator	const uiGroupObj*() const { return grpobj_; }
    inline operator	uiGroupObj*() 		{ return grpobj_; }
    inline operator	const uiObject&() const	{ return *grpobj_; }
    inline operator	uiObject&()		{ return *grpobj_; }
    inline uiObject*	attachObj()		{ return grpobj_; }
    inline const uiObject* attachObj() const	{ return grpobj_; }
    inline uiParent*	parent()		{ return grpobj_->parent(); }
    inline const uiParent* parent() const	{ return grpobj_->parent(); }

    void		setHSpacing( int ); 
    void		setVSpacing( int ); 
    void		setSpacing( int s=0 )	
			{ setHSpacing(s); setVSpacing(s); }
    void		setBorder( int ); 

    void		setFrame( bool yn=true );
    void		setNoBackGround();

    uiObject*		hAlignObj();
    void		setHAlignObj( uiObject* o );
    void		setHAlignObj( uiGroup* o )
			    { setHAlignObj(o->mainObject()); }
    uiObject*		hCenterObj();
    void		setHCenterObj( uiObject* o );
    void		setHCenterObj( uiGroup* o )
			    { setHCenterObj(o->mainObject()); }

    //! internal use only. Tells the layout manager it's a toplevel mngr.
    void		setIsMain( bool ); 
    virtual uiMainWin*	mainwin()
			    { return mainObject() ? mainObject()->mainwin() :0;}

    static uiGroup*	gtDynamicCastToGrp( mQtclass(QWidget*) );

    void		setChildrenSensitive(bool);

    virtual Notifier<uiBaseObject>& preFinalise()
				{ return mainObject()->preFinalise(); }
    virtual Notifier<uiBaseObject>& postFinalise()
				{ return mainObject()->postFinalise(); }

protected:

    uiGroupObj*		grpobj_;
    uiGroupParentBody*	body_;

    virtual uiObject*	mainobject()			{ return grpobj_; }
    virtual void	attach_( constraintType, uiObject *oth, int margin=-1,
				bool reciprocal=true);

    virtual void	reDraw_( bool deep )		{}

    void		setShrinkAllowed(bool);
    bool		shrinkAllowed();

    void		bodyDel( CallBacker* );
    void		uiobjDel( CallBacker* );

    void		setFrameStyle(int);

    void		reSizeChildren(const uiObject*,float,float);

public:
    virtual void	setSize(const uiSize&);

};


#endif


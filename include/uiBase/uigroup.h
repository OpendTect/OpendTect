#ifndef uigroup_H
#define uigroup_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uigroup.h,v 1.27 2003-02-17 15:15:19 arend Exp $
________________________________________________________________________

-*/

#include <uiobj.h>
#include <uiparent.h>
#include <callback.h>
class IOPar;

class uiGroupBody;
class uiParentBody;

class uiGroup;
class uiGroupObjBody;
class uiGroupParentBody;

//class uiTabGroup;
//class uiTab;

class QWidget;


class uiGroupObj : public uiObject
{ 	
friend class uiGroup;
protected:
			uiGroupObj( uiGroup*,uiParent*, const char*, bool );
public:

    virtual		~uiGroupObj();

protected:

    uiGroupObjBody*	body_;
    uiGroup*		uigrp_;

    void		bodyDel( CallBacker* );
    void		grpDel( CallBacker* );
};


class uiGroup : public uiParent
{ 	
friend class		uiGroupObjBody;
friend class		uiGroupParentBody;
friend class		uiGroupObj;
friend class		uiMainWin;
friend class		uiTabGroup;
public:
			uiGroup( uiParent* , const char* nm="uiGroup", 
				 bool manage=true );
    virtual		~uiGroup();

    inline uiGroupObj*	uiObj()				    { return grpobj_; }
    inline const uiGroupObj* uiObj() const		    { return grpobj_; }
    inline		operator const uiGroupObj*() const  { return grpobj_; }
    inline		operator uiGroupObj*() 		    { return grpobj_; }
    inline		operator const uiObject&() const    { return *grpobj_; }
    inline		operator uiObject&() 		    { return *grpobj_; }


    void		setHSpacing( int ); 
    void		setVSpacing( int ); 
    void		setSpacing( int s=0 )	
			{ setHSpacing(s); setVSpacing(s); }
    void		setBorder( int ); 

    void		setFrame( bool yn=true );

    uiObject*		hAlignObj();
    void		setHAlignObj( uiObject* o );
    uiObject*		hCentreObj();
    void		setHCentreObj( uiObject* o );

// uiObject methods, delegated to uiObj():

    void		display(bool yn=true,bool shrink=false,
				bool maximize=false);

    void		setFocus();

    Color               backgroundColor() const;
    void                setBackgroundColor(const Color&);
    void		setSensitive(bool yn=true);
    bool		sensitive() const;

    int			prefHNrPics() const;
    void                setPrefWidth( int w );
    void                setPrefWidthInChar( float w );
    int			prefVNrPics() const;
    void		setPrefHeight( int h );
    void		setPrefHeightInChar( float h );


    void                setStretch( int hor, int ver );

    void		attach( constraintType, int margin=-1);
    void		attach( constraintType, uiObject *oth, int margin=-1,
				bool reciprocal=true);
    void		attach( constraintType t, uiGroup *o, int margin=-1,
				bool reciprocal=true)
			    { attach(t, o->uiObj(), margin, reciprocal); } 

    void 		setFont( const uiFont& );
    const uiFont*	font() const;

    virtual bool	fillPar( IOPar& ) const		{ return true; }
    virtual void	usePar( const IOPar& )		{}

    uiSize		actualSize( bool include_border = true) const;

    void		setCaption( const char* );

    void		shallowRedraw( CallBacker* =0 )		{reDraw(false);}
    void		deepRedraw( CallBacker* =0 )		{reDraw(true); }
    void		reDraw( bool deep );

    //! inernal use only. Tells the layout manager it's a toplevel mngr.
    void		setIsMain( bool ); 
    virtual uiMainWin*	mainwin() { return uiObj() ? uiObj()->mainwin() : 0; }

    static uiGroup*	gtDynamicCastToGrp( QWidget* );

protected:

    uiGroupObj*		grpobj_;
    uiGroupParentBody*	body_;

    virtual void	reDraw_( bool deep )			{}

    void		setShrinkAllowed(bool);
    bool		shrinkAllowed();

    void		bodyDel( CallBacker* );
    void		uiobjDel( CallBacker* );

    void		setFrameStyle(int);

};

class NotifierAccess;

class uiGroupCreater
{
public:

    virtual uiGroup*		create(uiParent*,NamedNotifierList* =0)	= 0;

};


#endif

#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          16/05/2001
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uilayout.h"

class MouseCursor;
class uiObjectBody;
class uiObject;
class uiMainWin;
class uiParentBody;


mExpClass(uiBase) uiParent : public uiBaseObject
{
friend class uiParentBody;
friend class uiObjectBody;
public:
    void		addChild(uiBaseObject&);
    void		manageChld(uiBaseObject&,uiObjectBody&);
    void		attachChild(constraintType tp,uiObject* child,
				    uiObject* other,int margin,
				    bool reciprocal);

    const ObjectSet<uiBaseObject>* childList() const;

    virtual uiMainWin*	mainwin()		{ return 0; }

    uiObject*		mainObject()		{ return mainobject(); }
    const uiObject*	mainObject() const
			    { return const_cast<uiParent*>(this)->mainobject();}

    uiParentBody*	pbody();
    const uiParentBody*	pbody() const
			{ return const_cast<uiParent*>(this)->pbody(); }


#define mIfMO()		if ( mainObject() ) mainObject()
#define mRetMO(fn,val)	return mainObject() ? mainObject()->fn() : val;

    void		attach( constraintType t, int margin=-1 )
			{ mIfMO()->attach(t,margin); }
    void		attach( constraintType t, uiParent* oth, int margin=-1,
				bool reciprocal=true)
			{ attach(t,oth->mainObject(),margin,reciprocal); }
    void		attach( constraintType t, uiObject* oth, int margin=-1,
				bool reciprocal=true)
			{ attach_(t,oth,margin,reciprocal); }


    virtual void	display( bool yn, bool shrk=false,
				 bool maximize=false )
			    { finalise(); mIfMO()->display(yn,shrk,maximize); }
    bool		isDisplayed() const	  { mRetMO(isDisplayed,false); }

    void		setFocus()                { mIfMO()->setFocus(); }
    bool		hasFocus() const	  { mRetMO(hasFocus,false); }

    void		setSensitive(bool yn=true){ mIfMO()->setSensitive(yn); }
    bool		sensitive() const	  { mRetMO(sensitive,false); }

    const uiFont*	font() const		  { mRetMO(font,0); }
    void		setFont( const uiFont& f) { mIfMO()->setFont(f); }
    void		setCaption(const uiString& c) { mIfMO()->setCaption(c);}
    void		setCursor(const MouseCursor& c) {mIfMO()->setCursor(c);}

    uiSize		actualSize( bool include_border) const
			{
			    if ( mainObject() )
				return mainObject()->actualSize(include_border);
			    return uiSize();
			}

    int			prefHNrPics() const	  { mRetMO(prefHNrPics, -1 ); }
    int			prefVNrPics() const	  { mRetMO(prefVNrPics,-1); }
    void		setPrefHeight( int h )    { mIfMO()->setPrefHeight(h); }
    void		setPrefWidth( int w )     { mIfMO()->setPrefWidth(w); }
    void		setPrefHeightInChar( int h )
			    { mIfMO()->setPrefWidthInChar(h); }
    void		setPrefHeightInChar( float h )
			    { mIfMO()->setPrefHeightInChar(h); }
    void		setPrefWidthInChar( float w )
			    { mIfMO()->setPrefWidthInChar(w); }
    void		setPrefWidthInChar( int w )
			    { mIfMO()->setPrefWidthInChar(w); }

    virtual void	reDraw( bool deep )	  { mIfMO()->reDraw( deep ); }
    void		shallowRedraw( CallBacker* =0 )         {reDraw(false);}
    void		deepRedraw( CallBacker* =0 )            {reDraw(true); }

    void		setStretch( int h, int v ){ mIfMO()->setStretch(h,v); }
    int			stretch( bool h ) const
			{ return mainObject() ? mainObject()->stretch(h) : 0; }

    Color		backgroundColor() const;
    Color		roBackgroundColor() const;
    void		setBackgroundColor( const Color& c )
			    { mIfMO()->setBackgroundColor(c); }

    void		translateText();

protected:
			uiParent(const char* nm,uiParentBody*);

    virtual void	attach_(constraintType t, uiObject* oth, int margin=-1,
				bool reciprocal=true)
			{ mIfMO()->attach(t,oth,margin,reciprocal); }

#undef mIfMO
#undef mRetMO

    virtual uiObject*	mainobject()	{ return 0; }

public:
    mDeprecated("Use actualSize")
    uiSize		actualsize(bool inclborder=true) const
			{ return actualSize(inclborder); }
};

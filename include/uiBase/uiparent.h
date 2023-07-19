#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uibaseobject.h"

#include "uigeom.h"
#include "uilayout.h"

class MouseCursor;
class uiFont;
class uiMainWin;
class uiObjectBody;
class uiObject;
class uiParentBody;


mExpClass(uiBase) uiParent : public uiBaseObject
{
friend class uiParentBody;
friend class uiObjectBody;
public:
    void		addChild(uiBaseObject&);
    void		manageChild(uiBaseObject&,uiObjectBody&);
    void		attachChild(ConstraintType tp,uiObject* child,
				    uiObject* other,int margin,
				    bool reciprocal);

    const ObjectSet<uiBaseObject>* childList() const;

    virtual uiMainWin*	mainwin()		{ return nullptr; }

    uiObject*		mainObject()		{ return mainobject(); }
    const uiObject*	mainObject() const
			    { return const_cast<uiParent*>(this)->mainobject();}

    uiParentBody*	pbody();
    const uiParentBody*	pbody() const
			{ return const_cast<uiParent*>(this)->pbody(); }

    void		attach(ConstraintType,int margin=-1);
    void		attach(ConstraintType,uiParent* oth,int margin=-1,
				bool reciprocal=true);
    void		attach(ConstraintType,uiObject* oth,int margin=-1,
				bool reciprocal=true);

    virtual void	display(bool yn,bool shrk=false,bool maximize=false);
    bool		isDisplayed() const;

    void		setFocus();
    bool		hasFocus() const;

    void		setSensitive(bool yn=true);
    bool		sensitive() const;

    const uiFont*	font() const;
    void		setFont(const uiFont&);
    void		setCaption(const uiString&);
    void		setCursor(const MouseCursor&);

    uiSize		actualSize( bool include_border) const;

    int			prefHNrPics() const;
    int			prefVNrPics() const;
    void		setPrefHeight(int h);
    void		setPrefWidth(int w);
    void		setPrefHeightInChar(int h);
    void		setPrefHeightInChar(float h);
    void		setPrefWidthInChar(float w);
    void		setPrefWidthInChar(int w);

    virtual void	reDraw(bool deep);
    void		shallowRedraw(CallBacker*);
    void		deepRedraw(CallBacker*);

    void		setStretch(int h,int v);
    int			stretch(bool h) const;

    OD::Color		backgroundColor() const;
    OD::Color		roBackgroundColor() const;
    void		setBackgroundColor(const OD::Color&);

    void		translateText() override;

protected:
			uiParent(const char* nm,uiParentBody*);
			~uiParent();
			mOD_DisableCopy(uiParent)

    virtual void	attach_(ConstraintType,uiObject* oth,int margin=-1,
				bool reciprocal=true);

    virtual uiObject*	mainobject()			{ return nullptr; }

public:
    mDeprecated("Use actualSize")
    uiSize		actualsize(bool inclborder=true) const
			{ return actualSize(inclborder); }
};

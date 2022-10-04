#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibasemod.h"
#include "uiobj.h"
#include "uistring.h"
#include "odiconfile.h"

class uiButtonBody;
class uiCheckBoxBody;
class uiPushButtonBody;
class uiRadioButtonBody;
mFDQtclass(QAbstractButton)

class uiMenu;
class uiPixmap;
mFDQtclass(QEvent)
mFDQtclass(QMenu)


//!\brief is the base class for all buttons.

mExpClass(uiBase) uiButton : public uiObject
{
public:
    virtual		~uiButton();

    virtual void	setText(const uiString&);
    const uiString&	text() const			{ return text_; }
    void		setIcon(const char* icon_identifier);
    void		setPixmap( const uiPixmap& pm ) { setPM(pm); }
    void		setIconScale(float val); /*!< val between [0-1] */
    virtual void	updateIconSize()	{}

    virtual void	click()		= 0;

    Notifier<uiButton>	activated;

			//! Not for casual use
    mQtclass(QAbstractButton*)	qButton();
    const mQtclass(QAbstractButton*)	qButton() const;

    static uiButton*	getStd(uiParent*,OD::StdActionType,const CallBack&,
				bool immediate);
    static uiButton*	getStd(uiParent*,OD::StdActionType,const CallBack&,
				bool immediate,const uiString& nonstd_text);
				//!< will deliver toolbutton is txt is empty

    static bool		haveCommonPBIcons()	{ return havecommonpbics_; }
    static void		setHaveCommonPBIcons( bool yn=true )
						{ havecommonpbics_ = yn; }

protected:
			uiButton(uiParent*,const uiString&,const CallBack*,
				 uiObjectBody&);

    uiString		text_;
    float		iconscale_;
    static bool		havecommonpbics_;

    void		translateText() override;
    virtual void	setPM(const uiPixmap&);

};


/*!\brief Push button. By default, assumes immediate action, not a dialog
  when pushed. The button text will in that case get an added " ..." to the
  text. In principle, it could also get another appearance.
*/

mExpClass(uiBase) uiPushButton : public uiButton
{
public:
			uiPushButton(uiParent*,const uiString& txt,
				     bool immediate);
			uiPushButton(uiParent*,const uiString& txt,
				     const CallBack&,bool immediate);
			uiPushButton(uiParent*,const uiString& txt,
				     const uiPixmap&,bool immediate);
			uiPushButton(uiParent*,const uiString& txt,
				     const uiPixmap&,const CallBack&,
				     bool immediate);
			~uiPushButton();

    void		setDefault(bool yn=true);
    void		click() override;
    void		setMenu(uiMenu*);
    void		setFlat(bool);
    bool		isFlat() const;

private:

    void		translateText() override;
    void		updateText();
    void		updateIconSize() override;

    bool		immediate_;
    uiPushButtonBody*	pbbody_;
    uiPushButtonBody&	mkbody(uiParent*,const uiString&);

};


mExpClass(uiBase) uiRadioButton : public uiButton
{
public:
			uiRadioButton(uiParent*,const uiString&);
			uiRadioButton(uiParent*,const uiString&,
				      const CallBack&);
			~uiRadioButton();

    bool		isChecked() const;
    virtual void	setChecked(bool yn=true);

    void		click() override;

private:

    uiRadioButtonBody*	rbbody_;
    uiRadioButtonBody&	mkbody(uiParent*,const uiString&);

};


mExpClass(uiBase) uiCheckBox : public uiButton
{
public:

			uiCheckBox(uiParent*,const uiString&);
			uiCheckBox(uiParent*,const uiString&,
				   const CallBack&);
			~uiCheckBox();

    bool		isChecked() const;
    void		setChecked(bool yn=true);
    void		setTriState(bool yn=true);
    void		setCheckState(OD::CheckState);
    OD::CheckState	getCheckState() const;

    void		click() override;

private:

    uiCheckBoxBody*	cbbody_;
    uiCheckBoxBody&	mkbody(uiParent*,const uiString&);

};


//! Button Abstract Base class

mExpClass(uiBase) uiButtonMessenger
{
    friend class	i_ButMessenger;

public:

			uiButtonMessenger();
    virtual		~uiButtonMessenger();

    //! Button signals emitted by Qt.
    enum notifyTp       { clicked, pressed, released, toggled };

protected:

    //! Handler called from Qt.
    virtual void	notifyHandler(notifyTp)		= 0;

};

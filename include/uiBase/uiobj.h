#ifndef uiobj_H
#define uiobj_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id: uiobj.h,v 1.19 2002-01-10 11:14:52 arend Exp $
________________________________________________________________________

-*/


#include "uihandle.h"
#include "uigeom.h"
#include "uilayout.h"
#include "color.h"
#include "errh.h"
#include <sizepolspec.h>

#include <stdlib.h>

#define mTemplTypeDef(fromclass,templ_arg,toclass) \
	typedef fromclass<templ_arg> toclass;
#define mTemplTypeDefT(fromclass,templ_arg,toclass) \
	mTemplTypeDef(fromclass,templ_arg,toclass)

class uiFont;
class uiObjectBody;
class uiParent;
class uiGroup;
class uiMainWin;
class uiButtonGroup;
class i_LayoutItem;

class uiObject : public uiObjHandle
{
    friend class	uiObjectBody;
public:
			uiObject( uiParent* p, const char* nm );
			uiObject( uiParent* p, const char* nm, uiObjectBody& );
			~uiObject()			{}

    void		setSzPol( const SzPolicySpec& );
    SzPolicySpec	szPol() const;

    void		setToolTip(const char*);
    static void		enableToolTips(bool yn=true);
    static bool		toolTipsEnabled();

    void		display( bool yn = true, bool shrink=false );
    void		setFocus();

    Color               backgroundColor() const;
    void                setBackgroundColor(const Color&);
    void		setSensitive(bool yn=true);
    bool		sensitive() const;

    int			prefHNrPics() const;
    virtual void	setPrefWidth( int w );
    void                setPrefWidthInChar( float w );
    int			prefVNrPics() const;
    virtual void	setPrefHeight( int h );
    void		setPrefHeightInChar( float h );

/*! \brief Sets stretch factors for object
    If stretch factor is > 1, then object will already grow at pop-up.
*/
    void                setStretch( int hor, int ver );


/*! \brief attaches object to another
    In case the stretched... options are used, margin=-1 (default) stretches
    the object not to cross the border.
    margin=-2 stretches the object to fill the parent's border. This looks nice
    with separators.
*/
    void		attach( constraintType, int margin=-1);
    void		attach( constraintType, uiObject* oth, int margin=-1);
    void		attach( constraintType, uiGroup* oth, int margin=-1);
    void		attach( constraintType, uiButtonGroup* oth, int mrg=-1);

    void 		setFont( const uiFont& );
    const uiFont*	font() const;

    uiSize		actualSize( bool include_border = true) const;

    void		setCaption( const char* );

			//! setGeometry should be triggered by this's layoutItem
    void 		triggerSetGeometry(const i_LayoutItem*, uiRect&);

    void		shallowRedraw( CallBacker* =0 )		{reDraw(false);}
    void		deepRedraw( CallBacker* =0 )		{reDraw(true); }
    void		reDraw( bool deep );

    uiParent*		parent() { return parent_; }
    uiMainWin*		mainwin();

			/*! \brief 'post' constructor.
			    Will be triggered before an object or its
			    children is/are shown by calling Qt's show().
			*/
    Notifier<uiObject>	finalising;


			/*! \brief triggered when getting a new geometry 
			    A reference to the new geometry is passed 
			    which *can* be manipulated, before the 
			    geometry is actually set to the QWidget.
			*/
    CNotifier<uiObject,uiRect&>	setGeometry;

protected:

                        //! hook. Accepts/denies closing of window.
    virtual bool	closeOK()		{ return true; } 

private:

    uiParent*		parent_;

};

#endif

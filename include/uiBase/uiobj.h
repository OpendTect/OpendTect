#ifndef uiobj_H
#define uiobj_H

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          25/08/1999
 RCS:           $Id: uiobj.h,v 1.24 2002-08-13 15:13:43 arend Exp $
________________________________________________________________________

-*/

/*! \mainpage

  \section intro Introduction

This module is a set of cooperating classes that enable creating User
Interfaces. This layer on top of the already wonderful Qt package was created
because of the following problems:

- Qt provides an enormous set of tools, of which we need only a fraction
- On the other hand, Qt does not offer things like:
   - Selection of widget on basis of data characteristics
   - Automated 'black-box' layouting (enabling full grouping into new classes)
   - Integration with our data types
   - Generalised Callback/Notifier support (instead of Qt's Signal/Slot)
- If we use Qt directly, no isolation is present.

Therefore, as with most external libraries, we chose to make a new layer to
combine the power of Qt with the flexibility of more generalised design
principles.


\section usage Usage

The basic principles are:
- Objects are linked to each other in the window by attaching.
- Events are notified by specifically asking a Notifier in the class.
- All objects can be grouped; every group must be prepared to be attached.
- 'Simple' data input should preferably be done through the uiGenInput
  class (see uiTools module).

The Qt window painting facilities are only used for quick sketching, the
code generation capacity is not used. Example:

<code>
    IntInpSpec spec( lastnrclasses );<br>
    spec.setLimits( Interval<int>( 0, 100 ) );<br>
    nrclassfld = new uiGenInput( this, "Number of classes", spec );<br>
<br>
    FloatInpSpec inpspec( lastratiotst*100 );<br>
    inpspec.setLimits( Interval<float>( 0, 100 ) );<br>
    perctstfld = new uiGenInput( this, "Percentage used for test set",
	    				inpspec );<br>
    perctstfld->attach( alignedBelow, nrclassfld );<br>
    <br>
    defbut = new uiPushButton( this, "Default" ); <br>
    defbut->activated.notify( mCB(this,ThisClass,setPercToDefault) ); <br>
    defbut->attach( rightOf, perctstfld );
</code>

Note that all objects could have been made:
- Conditional (only if a condition is met)
- Iterative (any number that may be necessary)
- As part of a group (e.g. to later treat as one uiObject)
- From a Factory (for example an object transforming a string into a uiObject).


\section design Design

In the uiBase directory, you'll find classes that directly communicate with Qt
to implement (parts of) this strategy. To keep the header files uncoupled
from the Qt header files, there is a mechanism where the 'ui' class has a
'ui'-Body class that is a subclass of a Qt class.

Almost every 'visible' object is a uiObject. Besides the different subclasses,
there is also the uiGroup which is just another uiObject. The windows holding
these are (like uiObjects) uiObjHandle's. The uiMainWin is a subclass, and
the ubiquitous uiDialog.

*/


#include "uihandle.h"
#include "uigeom.h"
#include "uilayout.h"
#include "color.h"
#include "errh.h"

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

/*! \brief How should the object's size behave? 
    undef       : use default.
    small       : 1 base sz.
    medium      : 2* base sz + 1.
    wide        : 4* base sz + 3.
    The xxvar options specify that the element may have a bigger internal
    preferred size. In that case, the maximum is taken.
    The xxmax options specify that the element should take all available
    space ( stretch = 2 )
*/
    enum		SzPolicy{ undef, small, medium, wide,
				  smallvar, medvar, widevar,
				  smallmax, medmax, widemax };


    void		setHSzPol( SzPolicy );
    void		setVSzPol( SzPolicy );
    SzPolicy		szPol( bool hor=true) const;

    void		setToolTip(const char*);
    static void		enableToolTips(bool yn=true);
    static bool		toolTipsEnabled();

    void		display( bool yn = true, bool shrink=false,
				 bool maximised=false );
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

			/*! persists current widget position
			    Only use for main windows, toolbars, etc.
			*/
    void		storePosition(CallBacker* cb=0);


    uiParent*		parent() { return parent_; }
    uiMainWin*		mainwin();

			/*! \brief 'post' constructor.
			    Will be triggered before an object or its
			    children is/are shown by calling Qt's show().
			*/
    Notifier<uiObject>	finalising;

			/*! \brief triggered when object closes.
			*/
    Notifier<uiObject>	close;


			/*! \brief triggered when getting a new geometry 
			    A reference to the new geometry is passed 
			    which *can* be manipulated, before the 
			    geometry is actually set to the QWidget.
			*/
    CNotifier<uiObject,uiRect&>	setGeometry;

protected:

                        //! hook. Accepts/denies closing of window.
    virtual bool	closeOK()	{ close.trigger(); return true; } 

private:

    uiParent*		parent_;

};

#endif

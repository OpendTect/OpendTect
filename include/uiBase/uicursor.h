#ifndef uicursor_h
#define uicursor_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          12/05/2004
 RCS:           $Id: uicursor.h,v 1.2 2004-05-12 12:31:11 kristofer Exp $
________________________________________________________________________

-*/


class uiCursor 
{
public:

    /*! This enum type defines the various cursors that can be used.

	\value ArrowCursor  standard arrow cursor
	\value UpArrowCursor  upwards arrow
	\value CrossCursor  crosshair
	\value WaitCursor  hourglass/watch
	\value IbeamCursor  ibeam/text entry
	\value SizeVerCursor  vertical resize
	\value SizeHorCursor  horizontal resize
	\value SizeFDiagCursor  diagonal resize (\)
	\value SizeBDiagCursor  diagonal resize (/)
	\value SizeAllCursor  all directions resize
	\value BlankCursor  blank/invisible cursor
	\value SplitVCursor  vertical splitting
	\value SplitHCursor  horizontal splitting
	\value PointingHandCursor  a pointing hand
	\value ForbiddenCursor  a slashed circle
	\value WhatsThisCursor  an arrow with a question mark
	\value BitmapCursor

	ArrowCursor is the default for widgets in a normal state.
    */
    enum		CursorShape   { ArrowCursor, UpArrowCursor,
					CrossCursor, WaitCursor,
					IbeamCursor, SizeVerCursor,
					SizeHorCursor, SizeBDiagCursor,
					SizeFDiagCursor, SizeAllCursor,
					BlankCursor, SplitVCursor,
					SplitHCursor, PointingHandCursor,
					ForbiddenCursor, WhatsThisCursor,
					LastCursor      = WhatsThisCursor,
					BitmapCursor    = 24
				      };

/*! \brief Sets another cursor for current application

    example:

    \code

	uiCursor::setOverrideCursor( uiCursor::WaitCursor );
	calculateHugeMandelbrot();              // lunch time...
	uiCursor::restoreOverrideCursor();

    \endcode    

    Application cursors are stored on an internal stack.
    setOverrideCursor() pushes the cursor onto the stack, and
    restoreOverrideCursor() pops the active cursor off the stack.
    Every setOverrideCursor() must eventually be followed by a corresponding
    restoreOverrideCursor(), otherwise the stack will never be emptied.

    If replace is true, the new cursor will replace the last overridecw
    cursor (the stack keeps its depth). If replace is FALSE, the new cursor
    is pushed onto the top of the stack.
*/
    static void		setOverrideCursor( CursorShape, bool replace=false );
    static void		restoreOverrideCursor();
};


/*! Class to automaticly change cursor, and change it back automaticly when
    class is running out of scope.
*/

class uiCursorChanger
{
public:
		uiCursorChanger(uiCursor::CursorShape cs)
			{ uiCursor::setOverrideCursor(cs); }
		~uiCursorChanger() { uiCursor::restoreOverrideCursor(); }
};

#endif

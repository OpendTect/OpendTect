#ifndef uimain_h
#define uimain_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Lammertink
 Date:          03/12/1999
 RCS:           $Id: uimain.h,v 1.16 2009-07-22 16:01:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigeom.h"
class uiMainWin;
class QApplication;
class uiFont;
class QWidget;
class BufferStringSet;


mClass uiMain
{
public:
			uiMain(int& argc,char** argv);
private:
			uiMain(QApplication*);
    void 		init(QApplication*,int& argc,char **argv);

public:

    virtual		~uiMain();

    virtual int		exec();	
    void 		exit(int retcode=0);

    void		getCmdLineArgs(BufferStringSet&) const;
    void		setTopLevel(uiMainWin*);
    uiMainWin*		topLevel()			{ return mainobj_; }
    void		setFont(const uiFont&,bool passtochildren);    

    const uiFont*	font(); 

    static uiMain&	theMain();

    static void		flushX();

    /*!  Processes pending events for maxtime milliseconds
     *   or until there are no more events to process, whichever is shorter.
     *   Only works after themain has been constructed.
     */
    static void		processEvents(int msec=3000);

protected:

    static uiMain*	themain_;
    uiMainWin*		mainobj_;

    static QApplication*  app_;
    static const uiFont*  font_;

			//! necessary for uicMain coin inialisation
    virtual void	init( QWidget* mainwidget )             {}
};

#endif

#ifndef uiwindowgrabber_h
#define uiwindowgrabber_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        J.C. Glas
 Date:          July 2008
 RCS:           $Id: uiwindowgrabber.h,v 1.2 2008-08-01 12:04:30 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uidialog.h"
#include "thread.h"

class uiLabeledComboBox;
class uiFileInput;
class uiSliderExtra;
class uiLabel;
class uiMainWin;


/*!Dialog to specify the grab window and the output image file */

class uiWindowGrabDlg : public uiDialog
{
public:
			uiWindowGrabDlg(uiParent*,bool desktop);

    uiMainWin*		getWindow() const;
    const char*		getFilename() const	{ return filename_.buf(); }
    int			getQuality() const;

protected:
    uiLabeledComboBox*	windowfld_;
    uiFileInput*	fileinputfld_;
    uiSliderExtra*	qualityfld_;
    uiLabel*		infofld_;

    ObjectSet<uiMainWin> windowlist_;

    void		updateFilter();
    void		fileSel(CallBacker*);
    void		addFileExtension(BufferString&);
    bool		filenameOK() const;

    bool		acceptOK(CallBacker*);
    void		surveyChanged(CallBacker*);

    const char*		getExtension() const;

    static BufferString dirname_;
    BufferString	filename_;
};


/*!Grabs the screen area covered by a window or the whole desktop */

class uiWindowGrabber: public CallBacker
{
public:
			uiWindowGrabber(uiParent*);

    void		grabDesktop(bool yn)	{ desktop_ = yn; }
    bool		go();

protected:
    uiParent*		parent_;
    bool		desktop_;
    uiMainWin*		grabwin_;
    BufferString	filename_;
    int			quality_;
    
    void		mkThread(CallBacker*);
    Threads::Thread*	execthr_;
};


#endif

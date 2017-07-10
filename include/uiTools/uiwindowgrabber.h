#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          July 2008
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"
#include "fileformat.h"

class uiLabeledComboBox;
class uiFileSel;
class uiSlider;
class uiLabel;
class uiMainWin;
class Timer;


/*!Dialog to specify the grab window and the output image file */

mExpClass(uiTools) uiWindowGrabDlg : public uiDialog
{ mODTextTranslationClass(uiWindowGrabDlg);
public:
			uiWindowGrabDlg(uiParent*,bool desktop);

    uiMainWin*		getWindow() const;
    const char*		getFilename() const	{ return filename_.buf(); }
    int			getQuality() const;

protected:

    uiLabeledComboBox*	windowfld_;
    uiFileSel*		inpfilefld_;
    uiSlider*		qualityfld_;
    uiLabel*		infofld_;

    ObjectSet<uiMainWin> windowlist_;
    File::FormatList	fileformats_;

    void		updateFilter();
    bool		filenameOK() const;

    bool		acceptOK();
    void		surveyChanged(CallBacker*);

    static BufferString dirname_;
    BufferString	filename_;

};


/*!Grabs the screen area covered by a window or the whole desktop */

mExpClass(uiTools) uiWindowGrabber: public CallBacker
{
public:
			uiWindowGrabber(uiParent*);
			~uiWindowGrabber();

    void		grabDesktop(bool yn)	{ desktop_ = yn; }
    bool		go();
    void		actCB(CallBacker*);

protected:
    uiParent*		parent_;
    bool		desktop_;
    uiMainWin*		grabwin_;
    BufferString	filename_;
    int			quality_;
    Timer*		tmr_;
};

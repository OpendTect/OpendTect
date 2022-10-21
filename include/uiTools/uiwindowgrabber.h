#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uidialog.h"

class Timer;
class uiFileInput;
class uiLabel;
class uiLabeledComboBox;
class uiMainWin;
class uiSlider;


/*!Dialog to specify the grab window and the output image file */

mExpClass(uiTools) uiWindowGrabDlg : public uiDialog
{ mODTextTranslationClass(uiWindowGrabDlg);
public:
			uiWindowGrabDlg(uiParent*,bool desktop);
			~uiWindowGrabDlg();

    uiMainWin*		getWindow() const;
    const char*		getFilename() const	{ return filename_.buf(); }
    int			getQuality() const;

protected:
    uiLabeledComboBox*	windowfld_;
    uiFileInput*	fileinputfld_;
    uiSlider*		qualityfld_;
    uiLabel*		infofld_;

    ObjectSet<uiMainWin> windowlist_;

    void		updateFilter();
    void		fileSel(CallBacker*);
    void		addFileExtension(BufferString&);
    bool		filenameOK() const;

    bool		acceptOK(CallBacker*) override;

    const char*		getExtension() const;

    BufferString	filename_;
};


/*!Grabs the screen area covered by a window or the whole desktop */

mExpClass(uiTools) uiWindowGrabber : public CallBacker
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

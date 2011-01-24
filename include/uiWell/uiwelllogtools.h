#ifndef uiwelllogetool_h
#define uiwelllogetool_h
/*+
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          Jan 2011
RCS:           $Id: uiwelllogtools.h,v 1.2 2011-01-24 16:43:46 cvsbruno Exp $
________________________________________________________________________

-*/


#include "uidialog.h"
#include "bufstringset.h"

class uiListBox;
class uiComboBox;
class uiGenInput;
class uiCheckBox;
class uiMultiWellLogSel;
class uiPushButton;
class uiWellLogDisplay;


namespace Well { class Data; class Log; class LogSet; }


mClass uiWellLogToolWin : public uiMainWin
{
public:	

    mStruct LogData
    {
				LogData(const Well::LogSet&);
				~LogData();

	const char*		wellname_;
	Interval<float>		dahrg_;

	int			setSelectedLogs(BufferStringSet&);
	void			getOutputLogs(Well::LogSet& ls) const;

    protected:

	Well::LogSet&		logs_;

	ObjectSet<Well::Log>	inplogs_;
	ObjectSet<Well::Log>	outplogs_;

	friend class		uiWellLogToolWin;
    };

				uiWellLogToolWin(uiParent*,ObjectSet<LogData>&);
				~uiWellLogToolWin();


    bool                	needSave() const        { return needsave_; }

    void			getLogDatas(ObjectSet<LogData>& lds) const
				{ lds = logdatas_; }

protected:

    uiComboBox*			actionfld_;
    uiCheckBox*			overwritefld_;
    uiGenInput*			savefld_;
    uiPushButton*		applybut_;
    uiPushButton*               okbut_;
    uiPushButton*               cancelbut_;
    Interval<float>		zdisplayrg_;

    ObjectSet<LogData>		logdatas_;
    ObjectSet<uiWellLogDisplay> logdisps_;
    bool			needsave_;

    void			displayLogs();

    void			overWriteCB(CallBacker*);
    void			applyPushedCB(CallBacker*);
    bool			acceptOK(CallBacker*);
    bool			rejectOK(CallBacker*);
};


mClass uiWellLogToolWinMgr : public uiDialog
{
public:
			uiWellLogToolWinMgr(uiParent*);
protected:

    uiMultiWellLogSel*	welllogselfld_;

    void		winClosed(CallBacker*);
    bool		acceptOK(CallBacker*);
};



#endif

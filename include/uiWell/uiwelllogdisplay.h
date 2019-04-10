#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2009
 RCS:           $Id: uiwelllogdisplay.h
________________________________________________________________________

-*/


#include "uiwellmod.h"
#include "uiwelldahdisplay.h"
#include "uidialog.h"

class uiPolyLineItem;
class uiPolygonItem;
namespace Well { class Log; }

/*!
\brief Displays maximum two Well logs.
*/

mExpClass(uiWell) uiWellLogDisplay : public uiWellDahDisplay
{
public:
			//TODO
		       uiWellLogDisplay(uiParent* p, const Setup& su);
			uiWellLogDisplay(uiParent*,const Setup&,
			       const Well::DisplayProperties2D::LogPanelProps&);
			~uiWellLogDisplay();

    mStruct(uiWell) LogData : public uiWellDahDisplay::DahObjData
    {
			LogData();

	void		setLog(const Well::Log*);
	const Well::Log* log() const;

	void		getInfoForDah(float,BufferString&) const;

	Well::LogDispProps disp_;

	Notifier<uiWellLogDisplay::LogData> logSet;

    protected:
			LogData(uiGraphicsScene&,bool isfirst,
				    const uiWellLogDisplay::Setup&);

	virtual void	copySetupFrom( const LogData& ld )
			{
			    xrev_ = ld.xrev_;
			    disp_ = ld.disp_;
			}

	ObjectSet<uiPolygonItem> curvepolyitms_;

	friend class	uiWellLogDisplay;
    };

    LogData&		logData(int);
    const LogData&	logData(int idx) const
				{ return const_cast<uiWellLogDisplay*>(this)
							->logData(idx); }
    int			nrLogs() const;

protected:

    Setup		setup_;

    void		gatherDataInfo(int);
    void		drawData(int);

    void		drawCurve(int);
    void		drawSeismicCurve(int);
    void		drawFilledCurve(int);

    void		setLogDisplayPorp(CallBacker*);
};


/*!
\brief Non-modal dialog displaying a maximum of two Well logs.
*/

mExpClass(uiWell) uiWellLogDispDlg : public uiDialog
{ mODTextTranslationClass(uiWellLogDispDlg)
public:

				uiWellLogDispDlg(uiParent*,
						const uiWellLogDisplay::Setup&);
				~uiWellLogDispDlg();

    void			setLog(const Well::Log*,bool first=true,
						        const char* wellnm=0);
    const Well::Log*		getLog(bool first=true) const;

    uiWellLogDisplay&		logDisplay()		{ return *dispfld_; }
			//!< for detailed work

    Notifier<uiWellLogDispDlg>	logSet;

    static uiWellLogDispDlg*	popupNonModal(uiParent*,const Well::Log*,
					      const Well::Log* wl2=0,
					      const char* wellnm1=0,
					      const char* wellnm2=0);
				    //!< has setDeleteOnClose set

protected:

    ConstRefMan<Well::Log>	log1_;
    ConstRefMan<Well::Log>	log2_;
    BufferString		wellnm1_;
    BufferString		wellnm2_;

    uiWellLogDisplay*		dispfld_;
    void			logSetCB(CallBacker*);

};

#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
			uiWellLogDisplay(uiParent*,const Setup&);
			~uiWellLogDisplay();

    mStruct(uiWell) LogData : public uiWellDahDisplay::DahObjData
    {
			~LogData();

	void		setLog(const Well::Log*);
	const Well::Log* log() const;

	void		getInfoForDah(float,BufferString&) const override;

	Well::DisplayProperties::Log disp_;

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

	friend class 	uiWellLogDisplay;
    };

    LogData&		logData(bool first=true);
    const LogData&	logData(bool first=true) const
				{ return const_cast<uiWellLogDisplay*>(this)
				    			->logData(first); }

protected:

    Setup		setup_;

    void		gatherDataInfo(bool) override;
    void		draw() override;

    void		drawCurve(bool) override;
    void		drawSeismicCurve(bool);
    void		drawFilledCurve(bool);

};


/*!
\brief Non-modal dialog displaying a maximum of two Well logs.
*/

mExpClass(uiWell) uiWellLogDispDlg : public uiDialog
{ mODTextTranslationClass(uiWellLogDispDlg)
public:

   			uiWellLogDispDlg(uiParent*,
					 const uiWellLogDisplay::Setup&,
					 bool make_copy=true);
			~uiWellLogDispDlg();

    void		setLog(const Well::Log*,bool first=true,
			       const char* wellnm=0);
    const Well::Log*	getLog(bool first=true) const;

    uiWellLogDisplay&	logDisplay()	{ return *dispfld_; }
    			//!< for detailed work

    Notifier<uiWellLogDispDlg>	logSet;

    static uiWellLogDispDlg* popupNonModal(uiParent*,const Well::Log*,
					   const Well::Log* wl2=0,
					   const char* wellnm1=0,
					   const char* wellnm2=0);
    			//!< has setDeleteOnClose set

protected:

    bool		logsmine_;
    const Well::Log*	log1_;
    const Well::Log*	log2_;
    BufferString	wellnm1_;
    BufferString	wellnm2_;

    uiWellLogDisplay*	dispfld_;
    void		logSetCB(CallBacker*);

};

#ifndef uiwelllogdisplay_h
#define uiwelllogdisplay_h

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
#include "uiaxishandler.h"
#include "draw.h"

class uiTextItem;
class uiLineItem;
class uiPolyLineItem;
class uiPolygonItem;
class uiGraphicsScene;
class UnitOfMeasure;

namespace Well { class Log; class Marker; }

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

	void		setLog(const Well::Log*);
	const Well::Log* log() const;

	void		getInfoForDah(float,BufferString&) const;

	const UnitOfMeasure* unitmeas_;
	Well::DisplayProperties::Log disp_;

	Notifier<uiWellLogDisplay::LogData> logSet;

    protected:
			LogData(uiGraphicsScene&,bool isfirst,
				    const uiWellLogDisplay::Setup&);

	virtual void	copySetupFrom( const LogData& ld )
			{
			    unitmeas_ = ld.unitmeas_;
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

    void		gatherDataInfo(bool);
    void		draw();

    void		drawCurve(bool);
    void		drawSeismicCurve(bool);
    void		drawFilledCurve(bool);

};


/*!
\brief Non-modal dialog displaying a maximum of two Well logs.
*/

mExpClass(uiWell) uiWellLogDispDlg : public uiDialog
{
public:

   			uiWellLogDispDlg(uiParent*,
					 const uiWellLogDisplay::Setup&,
					 bool make_copy=true);
			~uiWellLogDispDlg();

    void		setLog(const Well::Log*,bool first=true);
    const Well::Log*	getLog(bool first=true) const;

    uiWellLogDisplay&	logDisplay()	{ return *dispfld_; }
    			//!< for detailed work

    Notifier<uiWellLogDispDlg>	logSet;

protected:

    bool		logsmine_;
    const Well::Log*	log1_;
    const Well::Log*	log2_;

    uiWellLogDisplay*	dispfld_;
    void		logSetCB(CallBacker*);

};


#endif

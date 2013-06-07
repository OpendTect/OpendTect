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


#include "uiwelldahdisplay.h"
#include "uiaxishandler.h"
#include "draw.h"

class uiTextItem;
class uiLineItem;
class uiPolyLineItem;
class uiPolygonItem;
class uiGraphicsScene;
class UnitOfMeasure;

namespace Well { class Log; class Marker; }


/*!\brief creates a display of max 2 well logs. */
mClass uiWellLogDisplay : public uiWellDahDisplay
{
public:

			    uiWellLogDisplay(uiParent*,const Setup&);
			    ~uiWellLogDisplay();

    mStruct LogData : public uiWellDahDisplay::DahObjData
    {
	void				setLog(const Well::Log*);
	const Well::Log*		log() const;

	const UnitOfMeasure*    	unitmeas_;
	Well::DisplayProperties::Log 	disp_;

	void			getInfoForDah(float,BufferString&) const;

    protected:
					LogData(uiGraphicsScene&,bool isfirst,
					    const uiWellLogDisplay::Setup&);
					~LogData() {}

	virtual void            	copySetupFrom( const LogData& ld )
					{
					    unitmeas_   = ld.unitmeas_;
					    xrev_       = ld.xrev_;
					    disp_       = ld.disp_;
					}

	ObjectSet<uiPolygonItem> 	curvepolyitms_;

	friend class 			uiWellLogDisplay;
    };

    LogData&                    logData( bool first=true );
    const LogData&              logData( bool first=true ) const;

protected:

    Setup                       setup_;

    void			gatherDataInfo(bool);
    void                        draw();

    void                        drawCurve(bool);
    void                        drawSeismicCurve(bool);
    void                        drawFilledCurve(bool);
};

#endif


#ifndef vissurvwell_h
#define vissurvwell_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viswelldisplay.h,v 1.15 2004-03-31 06:45:16 nanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "multiid.h"
#include "ranges.h"

class LineStyle;
template <class T> class Interval;

namespace visBase { class Well; class Transformation; };


namespace visSurvey
{
class Scene;

/*!\brief Used for displaying welltracks, markers and logs


*/

class WellDisplay :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    static WellDisplay*		create()
				mCreateDataObj(WellDisplay);

    bool			setWellId(const MultiID&);
    const MultiID&		wellId() const 		{ return wellid; }

    const LineStyle&		lineStyle() const;
    void			setLineStyle(const LineStyle&);

    void			showWellName(bool);
    bool			wellNameShown() const;

    void			addMarkers();
    void			showMarkers(bool);
    bool			markersShown() const;
    void			showMarkerName(bool);
    bool			markerNameShown() const;
    void			setMarkerSize(int);
    int				markerSize() const;

    void			displayLog(int idx,int nr,
	    				   const Interval<float>* rg=0);
    				//!< idx: idx in Well::LogSet
    				//!< nr==1: left log; nr==2: right log
    void			displayLog(const char*,const Interval<float>&,
	    				   int nr);
    void			setLogColor(const Color&,int);
    const Color&		logColor(int) const;
    void			setLogLineWidth(float,int);
    float			logLineWidth(int) const;
    void			setLogWidth(int);
    int				logWidth() const;
    void			showLogs(bool);
    bool			logsShown() const;
    void			showLogName(bool);
    bool			logNameShown() const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    void			setTransformation(visBase::Transformation*);
    visBase::Transformation*	getTransformation();

protected:
    virtual			~WellDisplay();
    void			setWell(visBase::Well*);
    void			updateMarkerScale(CallBacker*);

    visBase::Well*		well;

    MultiID			wellid;
    const bool			zistime;
    const bool			zinfeet;

    BufferString		log1nm;
    BufferString		log2nm;
    Interval<float>		log1rg;
    Interval<float>		log2rg;

    static const char*		earthmodelidstr;
    static const char*		wellidstr;
    static const char*		log1nmstr;
    static const char*		log1rgstr;
    static const char*		log2nmstr;
    static const char*		log2rgstr;
    static const char*		log1colorstr;
    static const char*		log2colorstr;
};

};


#endif

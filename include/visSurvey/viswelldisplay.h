#ifndef vissurvwell_h
#define vissurvwell_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viswelldisplay.h,v 1.7 2003-08-22 11:32:40 nanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "multiid.h"

class LineStyle;

namespace visBase { class PolyLine; class DrawStyle; class Text; };


namespace visSurvey
{
class Scene;

/*!\brief


*/

class WellDisplay :	public visBase::VisualObjectImpl,
			public visSurvey::SurveyObject
{
public:
    static WellDisplay*		create()
				mCreateDataObj(WellDisplay);

    bool			setWellId( const MultiID& );
    const MultiID&		wellId() const { return wellid; }

    const LineStyle&		lineStyle() const;
    void			setLineStyle( const LineStyle& lst );

    void			showWellText(bool);
    bool			isWellTextShown() const;

    virtual void                fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int                 usePar( const IOPar& );

    void			setTransformation( visBase::Transformation* );
    visBase::Transformation*	getTransformation();

protected:
    virtual			~WellDisplay();

    MultiID			wellid;
    visBase::PolyLine*		line;
    visBase::DrawStyle*		drawstyle;
    visBase::Text*		welltxt;

    static const char*		ioobjidstr;
    static const char*		earthmodelidstr;
    static const char*		linestylestr;
    static const char*		showwellnmstr;
};

};


#endif

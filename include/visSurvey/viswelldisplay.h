#ifndef vissurvwell_h
#define vissurvwell_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: viswelldisplay.h,v 1.2 2002-05-22 06:17:42 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "vissurvobj.h"
#include "multiid.h"

class LineStyle;

namespace visBase { class PolyLine; class DrawStyle; };


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
				mCreateDataObj0arg(WellDisplay);

    bool			setWellId( const MultiID& );
    const MultiID&		wellId() const { return wellid; }

    int				nrAttribs() const;
    const char*			getAttribName( int ) const;
    void			displayAttrib( int ) {}
    				//!< -1 = none
    				//!< TODO: Implement!!
    int				displayedAttrib() const { return -1; }

    bool			depthIsT() const;

    const LineStyle&		lineStyle() const;
    void			setLineStyle( const LineStyle& lst );

    virtual void                fillPar( IOPar&, TypeSet<int>& ) const;
    virtual int                 usePar( const IOPar& );


protected:
    virtual		~WellDisplay();

    int 		displayedattrib;
    MultiID		wellid;
    visBase::PolyLine*	line;
    visBase::DrawStyle*	drawstyle;

    static const char*	earthmodelidstr;
    static const char*	displayattribstr;
    static const char*	colortableidstr;
    static const char*	linestylestr;
};

};


#endif

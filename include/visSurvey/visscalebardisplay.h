#ifndef visscalebardisplay_h
#define visscalebardisplay_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Rannojay Sen
 Date:		March 2104
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vislocationdisplay.h"

/*!\brief
  ScaleBar Display
*/

namespace visSurvey
{

mExpClass(visSurvey) ScaleBarDisplay : public LocationDisplay
{
public:
    static ScaleBarDisplay*	create()
				mCreateDataObj(ScaleBarDisplay);
    				~ScaleBarDisplay();
    
    void			setOnInlCrl(bool);
    bool			isOnInlCrl() const;
    
    void			setOrientation(int);
    int				getOrientation() const;

    void			setLineWidth(int);
    int				getLineWidth() const;
    
    void			setLength(double);
    double			getLength() const;

    void			setColors(const ::Color&);
    
    void			setDisplayTransformation(const mVisTrans*);
    const mVisTrans*		getDisplayTransformation() const;

    void			fromPar(const IOPar&);
    void			toPar(IOPar&) const;

protected:
      
    visBase::VisualObject*	createLocation() const;

    virtual void		setPosition(int,const Pick::Location&);
    virtual void		removePosition(int);

    virtual int			clickedMarkerIndex(
					const visBase::EventInfo& evi)const;

    void			zScaleCB(CallBacker*);
    void			dispChg(CallBacker*);

    bool			oninlcrl_;
    int				orientation_;
    int				linewidth_;
    double			length_;
    const mVisTrans*		displaytransform_;
    RefMan<visBase::DataObjectGroup>	group_;
};


} // namespace visSurvey

#endif

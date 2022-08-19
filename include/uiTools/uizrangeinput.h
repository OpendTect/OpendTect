#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uigeninput.h"
#include "zdomain.h"

/*! Field that takes care of getting a time or a depth range. */

mExpClass(uiTools) uiZRangeInput : public uiGenInput
{ mODTextTranslationClass(uiZRangeInput)
public:
    				uiZRangeInput(uiParent*,bool depth,bool wstep);

    template <class T> inline
    void			setZRange(T);
    StepInterval<float>		getFZRange() const;
    StepInterval<double>	getDZRange() const;

    void			setIsDepth(bool yn)	{ isdepth_ = yn; }

private:

    bool			isdepth_;
    const bool			withstep_;
};


template <class T> inline
void uiZRangeInput::setZRange( T range )
{
    if ( !isdepth_ && !range.isUdf() )
	range.scale( (float) ZDomain::Time().userFactor() );

    setValue( range );
}

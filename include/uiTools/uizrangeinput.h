#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer
 Date:          Mar 2013
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




#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          May 2011
________________________________________________________________________

-*/

#include "uiprestackprocessingmod.h"
#include "position.h"
#include "uigroup.h"

class uiLabel;

namespace PreStackView
{

mClass(uiPreStackProcessing) uiGatherDisplayInfoHeader : public uiGroup
{ mODTextTranslationClass(uiGatherDisplayInfoHeader)
public:
    				uiGatherDisplayInfoHeader(uiParent*);
    				~uiGatherDisplayInfoHeader() {}

    void			setData(const BinID&,bool inl,bool is2d,
	    				const char* data);
    void			setData(int pos,const char* data);
    void			setOffsetRange(const Interval<float>&);
    const char*			getDataName() const;

protected:
    uiLabel*			datalbl_;
    uiLabel*			poslbl_;
};


}; //namespace


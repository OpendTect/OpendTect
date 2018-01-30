#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          June 2002
________________________________________________________________________

-*/

#include "uitoolsmod.h"
#include "uigroup.h"

class uiComboBox;


/*!<\brief Icon set selector for OpendTect. Shows result immediately. */


mExpClass(uiTools) uiIconSetSel : public uiGroup
{ mODTextTranslationClass(uiIconSetSel);
public:

    static void		getSetNames(BufferStringSet&);
    static bool		canSelect( const BufferStringSet& nms )
			{ return nms.size() > 1; }

			uiIconSetSel(uiParent*,const BufferStringSet&,
				    bool withlabel);

    bool		newSetSelected() const;
    void		revert();
			//!< Makes sure old icon looks are restored

protected:

    BufferString	setnameatentry_;
    BufferStringSet	iconsetnms_;

    uiComboBox*		selfld_;

    void		setSel(CallBacker*);
    void		activateSet(const char*,bool force=false);

public:

    static bool		setODIconSet(const char* nm,bool writesetts);
			//!< Just so you can (but you probably don't want to)
			//!< returns whether this is a change

    static const char*	sKeyIconSetNm();
    static const char*	sKeyDefaultIconSetNm();

};

#ifndef uivolprocattrib_h
#define uivolprocattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		December 2013
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uivolumeprocessingmod.h"
#include "uiattrdesced.h"

namespace Attrib { class Desc; };

class uiIOObjSel;

/*! \brief VolumeProcessing Attribute ui */

mExpClass(uiVolumeProcessing) uiVolProcAttrib : public uiAttrDescEd
{
public:
			uiVolProcAttrib(uiParent*,bool);

protected:

    uiIOObjSel*		setupfld_;

    bool		setParameters(const Attrib::Desc&);
    bool		getParameters(Attrib::Desc&);

			mDeclReqAttribUIFns
};

#endif


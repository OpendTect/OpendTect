#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Jul 2017
________________________________________________________________________

-*/

#include "basicmod.h"
#include "enums.h"


namespace File
{

enum ViewStyle { Text, Table, Log, Bin };
mDeclareNameSpaceEnumUtils(Basic,ViewStyle)


/*!\brief Parameters for the simple basic file viewer */

mClass(Basic) ViewPars
{
public:

				ViewPars( ViewStyle vs=Text )
				    : style_(vs)
				    , editable_(false)
				    , maxnrlines_(vs==Table ? 500 : 10000) {}

    static const char*		sKeyFile()	{ return "file"; }
    static const char*		sKeyMaxLines()	{ return "maxlines"; }
    static const char*		sKeyStyle()	{ return "style"; }
    static const char*		sKeyEdit()	{ return "edit"; }

    ViewStyle			style_;
    int				maxnrlines_;	//!< max nr bytes when Bin
    bool			editable_;	//!< only if possible

};


mGlobal(Basic) bool launchViewer(const char*, const ViewPars& vp=ViewPars());


} // namespace File

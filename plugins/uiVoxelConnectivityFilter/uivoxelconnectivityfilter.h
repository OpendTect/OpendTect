#ifndef uivoxelconnectivityfilter_h
#define uivoxelconnectivityfilter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		May 2011
 RCS:		$Id: uivoxelconnectivityfilter.h,v 1.1 2011-08-11 09:46:19 cvskris Exp $
________________________________________________________________________

-*/

#include "uivolprocstepdlg.h"
#include "enums.h"
#include "factory.h"

namespace VolProc
{
class VoxelConnectivityFilter;

mClass uiVoxelConnectivityFilter : public uiStepDialog
{
public:
    mDefaultFactoryInstanciationBase( "VoxelConnectivityFilter",
				      "Voxel Connection Filter" );
    static uiStepDialog*	createInstance(uiParent*,Step*);

    				uiVoxelConnectivityFilter(uiParent*,
					    VoxelConnectivityFilter*);

    bool			acceptOK(CallBacker*);

protected:

    void		updateFieldsCB(CallBacker*);

    uiGenInput*		rangefld_;
    uiGenInput*		connectivityfld_;
    uiGenInput*		minbodysizefld_;
    uiGenInput*		acceptoutputfld_;
    uiGenInput*		acceptvaluefld_;
    uiGenInput*		rejectoutputudffld_;
    uiGenInput*		rejectoutputvalfld_;
};

}; //namespace

#endif

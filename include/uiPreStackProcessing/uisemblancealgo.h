#ifndef uisemblancealgo_h
#define uisemblancealgo_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		Nov 2010
 RCS:		$Id: uisemblancealgo.h,v 1.1 2010/11/09 22:23:08 cvskris Exp $
________________________________________________________________________


-*/

#include "factory.h"
#include "uidialog.h"


namespace PreStack
{

class SemblanceAlgorithm;

/*! Base class for algorithms that computes semblance along a moveout */
mClass uiSemblanceAlgorithm : public uiDialog
{
public:
    			mDefineFactory1ParamInClass(uiSemblanceAlgorithm,
						    uiParent*,factory);
    virtual bool	populateUI(const SemblanceAlgorithm*) 		= 0;
    virtual bool	populateObject(SemblanceAlgorithm*) const 	= 0;
protected:
    			uiSemblanceAlgorithm(uiParent*,const char* helpid);
};

}; //namespace

#endif

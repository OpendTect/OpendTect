#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2005
________________________________________________________________________

-*/
 
 
#include "geometrymod.h"
#include "transl.h"
#include "ioobjctxt.h"
#include <iosfwd>
class PosVecDataSet;


mExpClass(Geometry) PosVecDataSetTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(PosVecDataSet);
    mODTextTranslationClass(PosVecDataSetTranslatorGroup);
public:
    			mDefEmptyTranslatorGroupConstructor(PosVecDataSet)

    virtual const char*	defExtension() const	 	{ return "pvds"; }
};


mExpClass(Geometry) PosVecDataSetTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(PosVecDataSet)

    virtual bool	read(const IOObj&,PosVecDataSet&)		= 0;
    virtual bool	write(const IOObj&,const PosVecDataSet&)	= 0;

    const uiString&	errMsg() const		{ return errmsg_; }

protected:

    uiString		errmsg_;
};


mExpClass(Geometry) odPosVecDataSetTranslator : public PosVecDataSetTranslator
{			  isTranslator(od,PosVecDataSet)
public:
			mDefEmptyTranslatorConstructor(od,PosVecDataSet)

    virtual bool	read(const IOObj&,PosVecDataSet&);
    virtual bool	write(const IOObj&,const PosVecDataSet&);

};

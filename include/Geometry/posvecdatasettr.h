#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

 
 
#include "geometrymod.h"
#include "transl.h"
#include "ctxtioobj.h"
#include <iosfwd>
class PosVecDataSet;


mExpClass(Geometry) PosVecDataSetTranslatorGroup : public TranslatorGroup
{			     isTranslatorGroup(PosVecDataSet)
public:
			mDefEmptyTranslatorGroupConstructor(PosVecDataSet)

    const char*		defExtension() const override	 { return "pvds"; }
};


mExpClass(Geometry) PosVecDataSetTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(PosVecDataSet)

    virtual bool	read(const IOObj&,PosVecDataSet&)		= 0;
    virtual bool	write(const IOObj&,const PosVecDataSet&)	= 0;

    const char*		errMsg() const		{ return errmsg_.str(); }

protected:

    BufferString	errmsg_;
};


mExpClass(Geometry) odPosVecDataSetTranslator : public PosVecDataSetTranslator
{			  isTranslator(od,PosVecDataSet)
public:
			mDefEmptyTranslatorConstructor(od,PosVecDataSet)

    bool		read(const IOObj&,PosVecDataSet&) override;
    bool		write(const IOObj&,const PosVecDataSet&) override;

};

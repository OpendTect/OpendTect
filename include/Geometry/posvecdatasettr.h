#ifndef posvecdatasettr_h
#define posvecdatasettr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		June 2005
 RCS:		$Id$
________________________________________________________________________

-*/
 
 
#include "geometrymod.h"
#include "transl.h"
#include "ctxtioobj.h"
#include <iosfwd>
class PosVecDataSet;


mClass(Geometry) PosVecDataSetTranslatorGroup : public TranslatorGroup
{			     isTranslatorGroup(PosVecDataSet)
public:
    			mDefEmptyTranslatorGroupConstructor(PosVecDataSet)

    virtual const char*	defExtension() const	 	{ return "pvds"; }
};


mClass(Geometry) PosVecDataSetTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(PosVecDataSet)

    virtual bool	read(const IOObj&,PosVecDataSet&)		= 0;
    virtual bool	write(const IOObj&,const PosVecDataSet&)	= 0;

    const char*		errMsg() const		{ return errmsg_.str(); }

protected:

    BufferString	errmsg_;
};


mClass(Geometry) odPosVecDataSetTranslator : public PosVecDataSetTranslator
{			  isTranslator(od,PosVecDataSet)
public:
			mDefEmptyTranslatorConstructor(od,PosVecDataSet)

    virtual bool	read(const IOObj&,PosVecDataSet&);
    virtual bool	write(const IOObj&,const PosVecDataSet&);

};


#endif


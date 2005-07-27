#ifndef posvecdatasettr_h
#define posvecdatasettr_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		June 2005
 RCS:		$Id: posvecdatasettr.h,v 1.1 2005-07-27 09:23:35 cvsbert Exp $
________________________________________________________________________

-*/
 
 
#include "transl.h"
#include "ctxtioobj.h"
#include <iosfwd>
class PosVecDataSet;


class PosVecDataSetTranslatorGroup : public TranslatorGroup
{			     isTranslatorGroup(PosVecDataSet)
public:
    			mDefEmptyTranslatorGroupConstructor(PosVecDataSet)

    virtual const char*	defExtension() const	 	{ return "pvds"; }
};


class PosVecDataSetTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(PosVecDataSet)

    virtual bool	read(const IOObj&,PosVecDataSet&)		= 0;
    virtual bool	write(const IOObj&,const PosVecDataSet&)	= 0;

    const char*		errMsg() const		{ return errmsg_.buf(); }

protected:

    BufferString	errmsg_;
};


class odPosVecDataSetTranslator : public PosVecDataSetTranslator
{			  isTranslator(od,PosVecDataSet)
public:
			mDefEmptyTranslatorConstructor(od,PosVecDataSet)

    virtual bool	read(const IOObj&,PosVecDataSet&);
    virtual bool	write(const IOObj&,const PosVecDataSet&);

};


#endif

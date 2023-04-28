#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"

#include "coord.h"
#include "transl.h"

class ImageDef;

mExpClass(General) ImageDef
{
public:
			ImageDef();
			~ImageDef();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

    BufferString	filename_;
    Coord3		tlcoord_		= Coord3::udf(); // NW
    Coord3		brcoord_		= Coord3::udf(); // SE
    Coord3		trcoord_		= Coord3::udf();
    Coord3		blcoord_		= Coord3::udf();
};


mExpClass(General) ImageDefTranslatorGroup : public TranslatorGroup
{
isTranslatorGroup(ImageDef)
public:
			mDefEmptyTranslatorGroupConstructor(ImageDef)

};


mExpClass(General) ImageDefTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(ImageDef)

    virtual bool	read(ImageDef&,const IOObj&)		= 0;
    virtual bool	write(const ImageDef&,const IOObj&)	= 0;

    const uiString&	errMsg() const			{ return errmsg_; }

    static const char*	sKeyImageSpecs();

protected:
    uiString		errmsg_;
};


mExpClass(General) ODImageDefTranslator : public ImageDefTranslator
{
isTranslator(OD,ImageDef)
mODTextTranslationClass(ODImageDefTranslator)
public:
			mDefEmptyTranslatorConstructor(OD,ImageDef)

    const char*		defExtension() const override	{ return "imagedef"; }

    bool		read(ImageDef&,const IOObj&) override;
    bool		write(const ImageDef&,const IOObj&) override;

    bool		implRename(const IOObj*,const char*) const override;
    bool		implRemove(const IOObj*,bool) const override;

    static bool		readDef(ImageDef&,const IOObj&);
    static bool		writeDef(const ImageDef&,const IOObj&);

};

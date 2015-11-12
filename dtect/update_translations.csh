#!/bin/csh -f
#_______________________________________________________________________________
#
# (C) dGB Beheer B.V.
# $Id$
#
#_______________________________________________________________________________

# The purpose of this script is to make a .pro file to tell lupdate which
# files to scan and for which languages a ts file has to be created.
# It will:
# - make  a copy of an od directory
# - replace mExpClass and mClass by class
# - replace *_static_tr( "myfunc", ... ) with static_func_myfunc( ... )
# - create lupdate.pro
# - run lupdate for plural (*en-us.ts) and non-plural translations (all others).
# - copy the new translations into the work-directory.
# - remove the temporary copy

if (  $#argv < 5 ) then
    echo "Usage : $0 <sourcedir> <tsbasedir> <binarydir> <application> <lupdate>"
    exit 1
endif


if ( $?DTECT_SCRIPT_VERBOSE ) then
    echo "$0  ++++++"
    echo "args: $*"
    set verbose=yes
    set echo=on
endif

set scriptdir=`dirname $0`
set nrcpu = `${scriptdir}/GetNrProc`

set sourcedir=$1
set tsbasedir=$2
set binarydir=$3
set application=$4
set lupdate=$5
set tmpoddir=/tmp/lupdate_tmp_$$

set kernel=`uname -a | awk '{print $1}'`
if ( "${kernel}" == "Darwin" ) then
    set lupdate_dir=`dirname ${lupdate}`
    setenv DYLD_LIBRARY_PATH ${lupdate_dir}/../lib
endif

if ( -e $tmpoddir ) then
    \rm -rf $tmpoddir
endif

mkdir $tmpoddir
if ( -e ${sourcedir}/src ) cp -a ${sourcedir}/src ${tmpoddir}/.
if ( -e ${sourcedir}/include ) cp -a ${sourcedir}/include ${tmpoddir}/.
if ( -e ${sourcedir}/plugins ) cp -a ${sourcedir}/plugins ${tmpoddir}/.

set projectdir=${tmpoddir}/data/localizations/source
mkdir -p ${projectdir}

#Copy existing ts-files ot project dir
cp -a ${tsbasedir}/data/localizations/source/${application}*.ts ${projectdir}

set profnm=${projectdir}/normaltrans.pro

set olddir=`pwd`
cd ${projectdir}

set headers = `find $tmpoddir -path "*.h"`
set dirfile = ${tmpoddir}/dirs
foreach header ( ${headers} )
    dirname ${header} >> ${dirfile}
end

set dirs = `sort -u ${dirfile}`

#Create file with all files
set filelist="filelist.txt"

echo "" >> ${filelist}

echo -n "HEADERS = " >> ${filelist}
foreach fnm ( $headers )
    echo " \" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
end

echo "" >> ${filelist}

set sources = `find ${tmpoddir} -path "*.cc"`

echo -n "SOURCES = " >> ${filelist}
foreach fnm ( $sources )
    echo " \" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
end

# Create a list of target .ts files for normal ts files
echo -n "TRANSLATIONS = " > ${profnm}

echo " \" >> ${profnm}
echo -n "    ${application}_template.ts" >> ${profnm}

set nonomatch=1
foreach fnm ( ${application}*.ts )
    if ( "${fnm}" =~ "*en-us.ts" ) then
	continue
    endif

    if ( "${fnm}" =~ "*_template.ts" ) then
	continue
    endif

    echo " \" >> ${profnm}
    echo -n "    $fnm" >> ${profnm}
end

cat ${filelist} >> ${profnm}

echo "" >> ${profnm}
echo -n "INCLUDEPATH += " >> ${profnm}

foreach dir ( ${dirs} )
    echo " \" >> ${profnm}
    echo -n "	${dir}" >>${profnm}
end

#Create a list of .ts files for plural operations
set pluralpro=$projectdir/plural.pro
if ( -e ${application}_en-us.ts ) then
    echo -n "TRANSLATIONS = " > ${pluralpro}
    echo " \" >> ${pluralpro}
    echo -n "    ${application}_en-us.ts" >> ${pluralpro}
    cat ${filelist} >> ${pluralpro}

    echo "" >> ${pluralpro}
    echo -n "INCLUDEPATH += " >> ${pluralpro}

    foreach dir ( ${dirs} )
	    echo " \" >> ${pluralpro}
	    echo -e "	${dir}" >>${pluralpro}
    end
endif

#Remove the filelist
\rm -rf ${filelist}

#Filter the sources for patterns
echo ${sources} | xargs -P ${nrcpu} sed \
	-e 's/mODTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mdGBTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mDefineEMObjFuncs(.*)/Q_OBJECT/g' \
	-e 's/mExpClass(.*)/class /g' \
	-e 's/mClass(.*)/class /g' \
	-e 's/mStruct(.*)/struct /g' \
	-e 's/mExpStruct(.*)/struct /g' \
	-e 's/mEnumTr/EnumDefImpl::tr /g' \
	-e 's/"static_func_"/""/g' \
	-e 's/[^( \t]*_static_tr([ \t]*/static_func___begquote/g' \
	-e 's/static_func_[^,]*/&__endquote::tr(/g' \
	-e 's/"[\t ]*__endquote/"__endquote/g' \
	-e 's/::tr(,/::tr(/g' \
	-e 's/__begquote"//g' \
	-e 's/"__endquote//g' -iTMP

#Filter the headers for patterns
echo ${headers} | xargs -P ${nrcpu} sed \
	-e 's/mExpClass(.*)/class /g' \
	-e 's/mClass(.*)/class /g' \
	-e 's/mStruct(.*)/struct /g' \
	-e 's/mExpStruct(.*)/struct /g' \
	-e 's/mODTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mdGBTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mEnumTr/EnumDefImpl::tr /g' \
	-e 's/"static_func_"/""/g' \
	-e 's/[^( \t]*_static_tr([ \t]*/static_func___begquote/g' \
        -e 's/static_func_[^,]*/&__endquote::tr(/g' \
	-e 's/"[\t ]*__endquote/"__endquote/g' \
        -e 's/::tr(,/::tr(/g' \
        -e 's/__begquote"//g' \
        -e 's/"__endquote//g' -iTMP

#Run lupdate
${lupdate} -silent -locations relative ${profnm}
if ( -e ${pluralpro} ) then
    ${lupdate} -silent -locations relative -pluralonly ${pluralpro}
endif

#Copy results back
rsync --checksum *.ts ${binarydir}/data/localizations/generated

#Remvoe temporary dir
\rm -rf  ${tmpoddir}

#Go back to starting dir
cd ${olddir}


exit 0

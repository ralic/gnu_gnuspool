#! /bin/sh
#
# Generate gspool.h from header files keeping extraneous stuff at bay
#

CONFIG=${1:-../../config.h}
HDRS=${2:-../hdrs}

rm -f gspool.h

cp gspool.pre gspool.h
chmod 644 gspool.h

cat - <<\% >>gspool.h

/* This section built from config.h */

%
cat $CONFIG >>gspool.h
cat - <<\% >>gspool.h

/* Copied from defaults.h */

%
sed -n '/MAXTITLE/p
/NAMESIZE/p
/LINESIZE/p
/MAXFORM/p
/MAXFLAGS/p
/UIDSIZE/p
/HOSTNSIZE/p
/PFEEDBACK/p
/ALLOWFORMSIZE/p
/COMMENTSIZE/p
/^typedef/p' $HDRS/defaults.h >>gspool.h

cat - <<\% >>gspool.h

/* Copied from spq.h */

%

sed '1,/APISTART/d
s/spq/apispq/g
s/SPQ/APISPQ/g
s/spptr/apispptr/g
s/spp_/apispp_/g
s/SPP_/APISPP_/g
/APIEND/,$d' $HDRS/spq.h >>gspool.h
sed '1,/APISTART/d
s/pages/apipages/
/APIEND/,$d' $HDRS/pages.h >>gspool.h
sed -n '/PV_ADMIN/,/ALLPRIVS/p' $HDRS/spuser.h >>gspool.h
sed '1,/APISTART/d
s/spdet/apispdet/
s/sphdr/apisphdr/
/APIEND/,$d' $HDRS/spuser.h >>gspool.h

cat - <<\% >>gspool.h

/* Printer state codes */

%

sed '1,/APIEND/d
/^#define[ 	]*SPP_/!d
s/SPP_/API_PR/g' $HDRS/spq.h >>gspool.h
cat - <<\% >>gspool.h

/* Printer op codes */

%
sed -n '1,/APIEND/d
/^#define[ 	]*SO_/!d
s/SO_/PRINOP_/
/PRINOP_RSP/p
/PRINOP_PHLT/p
/PRINOP_PSTP/p
/PRINOP_PGO/p
/PRINOP_OYES/p
/PRINOP_ONO/p
/PRINOP_INTER/p
/PRINOP_PJAB/p' $HDRS/spq.h >>gspool.h

sed '1,/ERRSTART/d
s/XT_/GSPOOL_/g
/ERREND/,$d' $HDRS/xtapi_int.h >>gspool.h

cat gspool.post >>gspool.h

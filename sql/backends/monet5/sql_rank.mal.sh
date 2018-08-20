# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# Copyright 1997 - July 2008 CWI, August 2008 - 2018 MonetDB B.V.

sed '/^$/q' $0			# copy copyright from this file

cat <<EOF
# This file was generated by using the script ${0##*/}.

module sql;

EOF

cat <<EOF
pattern sql.diff(b:any_1) :bit
address SQLdiff
comment "return true if cur != prev row";

pattern batsql.diff(b:bat[:any_1]) :bat[:bit]
address SQLdiff
comment "return true if cur != prev row";

pattern sql.diff(p:bit, b:any_1) :bit
address SQLdiff
comment "return true if cur != prev row";

pattern batsql.diff(p:bat[:bit], b:bat[:any_1]) :bat[:bit]
address SQLdiff
comment "return true if cur != prev row";

pattern sql.row_number(b:any_1, p:bit, o:bit) :int
address SQLrow_number
comment "return the row_numer-ed groups";

pattern batsql.row_number(b:bat[:any_1], p:any_2, o:any_3) :bat[:int]
address SQLrow_number
comment "return the row_numer-ed groups";

pattern sql.rank(b:any_1, p:bit, o:bit) :int
address SQLrank
comment "return the ranked groups";

pattern batsql.rank(b:bat[:any_1], p:any_2, o:any_3) :bat[:int]
address SQLrank
comment "return the ranked groups";

pattern sql.dense_rank(b:any_1, p:bit, o:bit) :int
address SQLdense_rank
comment "return the densely ranked groups";

pattern batsql.dense_rank(b:bat[:any_1], p:any_2, o:any_3) :bat[:int]
address SQLdense_rank
comment "return the densely ranked groups";

pattern sql.percent_rank(b:any_1, p:bit, o:bit) :dbl
address SQLpercent_rank
comment "return the percentage into the total number of groups for each row";

pattern batsql.percent_rank(b:bat[:any_1], p:any_2, o:any_3) :bat[:dbl]
address SQLpercent_rank
comment "return the percentage into the total number of groups for each row";

pattern sql.min(b:any_1, p:bit, o:bit, unit:int, s:int, e:int, excl:int) :any_1
address SQLmin
comment "return the minimum of groups";

pattern batsql.min(b:bat[:any_1], p:any_2, o:any_3, unit:int, s:int, e:int, exl:int) :bat[:any_1]
address SQLmin
comment "return the minimum of groups";

pattern sql.max(b:any_1, p:bit, o:bit, unit:int, s:int, e:int, excl:int) :any_1
address SQLmax
comment "return the maximum of groups";

pattern batsql.max(b:bat[:any_1], p:any_2, o:any_3, unit:int, s:int, e:int, exl:int) :bat[:any_1]
address SQLmax
comment "return the maximum of groups";

pattern sql.count(p:bit, o:bit, unit:int, s:int, e:int, excl:int) :lng
address SQLcount
comment "return count of groups";

pattern batsql.count(p:any_1, o:any_2, unit:int, s:int, e:int, exl:int) :bat[:lng]
address SQLcount
comment "return count of groups";

pattern sql.count(b:any_1, p:bit, o:bit, unit:int, s:int, e:int, excl:int) :lng
address SQLcount_no_nil
comment "return count of groups";

pattern batsql.count(b:bat[:any_1], p:any_2, o:any_3, unit:int, s:int, e:int, exl:int) :bat[:lng]
address SQLcount_no_nil
comment "return count of groups";

EOF

for tp1 in 1:bte 2:sht 4:int 8:lng; do
    for tp2 in 8:lng; do
	if [ ${tp1%:*} -le ${tp2%:*} -o ${tp1#*:} = ${tp2#*:} ]; then
	    cat <<EOF
pattern sql.sum(b:${tp1#*:}, p:bit, o:bit, unit:int, s:int, e:int, excl:int) :${tp2#*:}
address SQLscalarsum
comment "return the sum of groups";

pattern batsql.sum(b:bat[:${tp1#*:}], p:any_1, o:any_2, unit:int, s:int, e:int, exl:int) :bat[:${tp2#*:}]
address SQLvectorsum_${tp2#*:}
comment "return the sum of groups";

pattern sql.prod(b:${tp1#*:}, p:bit, o:bit, unit:int, s:int, e:int, excl:int) :${tp2#*:}
address SQLscalarprod
comment "return the product of groups";

pattern batsql.prod(b:bat[:${tp1#*:}], p:any_1, o:any_2, unit:int, s:int, e:int, exl:int) :bat[:${tp2#*:}]
address SQLvectorprod_${tp2#*:}
comment "return the product of groups";

EOF
	fi
    done
done

for tp1 in 4:flt 8:dbl; do
    for tp2 in 4:flt 8:dbl; do
	if [ ${tp1%:*} -le ${tp2%:*} ]; then
	    cat <<EOF
pattern sql.sum(b:${tp1#*:}, p:bit, o:bit, unit:int, s:int, e:int, excl:int) :${tp2#*:}
address SQLscalarsum
comment "return the sum of groups";

pattern batsql.sum(b:bat[:${tp1#*:}], p:any_1, o:any_2, unit:int, s:int, e:int, exl:int) :bat[:${tp2#*:}]
address SQLvectorsum_${tp2#*:}
comment "return the sum of groups";

pattern sql.prod(b:${tp1#*:}, p:bit, o:bit, unit:int, s:int, e:int, excl:int) :${tp2#*:}
address SQLscalarprod
comment "return the product of groups";

pattern batsql.prod(b:bat[:${tp1#*:}], p:any_1, o:any_2, unit:int, s:int, e:int, exl:int) :bat[:${tp2#*:}]
address SQLvectorprod_${tp2#*:}
comment "return the product of groups";

EOF
	fi
    done
done

for tp1 in 1:bte 2:sht 4:int 8:lng 4:flt 8:dbl; do
	cat <<EOF
pattern sql.avg(b:${tp1#*:}, p:bit, o:bit, unit:int, s:int, e:int, excl:int) :dbl
address SQLavg
comment "return the average of groups";

pattern batsql.avg(b:bat[:${tp1#*:}], p:any_1, o:any_2, unit:int, s:int, e:int, exl:int) :bat[:dbl]
address SQLavg
comment "return the average of groups";

EOF
done

cat <<EOF
command aggr.exist(b:bat[:any_2], h:any_1):bit
address ALGexist;

command aggr.exist(b:bat[:any_2]):bit
address SQLexist;

pattern aggr.exist(v:any_2):bit
address SQLexist_val;

EOF

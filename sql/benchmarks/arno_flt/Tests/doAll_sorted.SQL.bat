@prompt # $t $g  

%SQL_CLIENT% < "%TSTSRCDIR%\check0.sql"

%SQL_CLIENT% < "%TSTSRCBASE%\%TSTDIR%\queries_sorted.sql"

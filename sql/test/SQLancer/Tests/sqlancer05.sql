CREATE TABLE "sys"."t1" (
	"c0" INTERVAL SECOND NOT NULL,
	CONSTRAINT "t1_c0_pkey" PRIMARY KEY ("c0"),
	CONSTRAINT "t1_c0_unique" UNIQUE ("c0")
);
INSERT INTO t1(c0) VALUES(INTERVAL '1474617942' SECOND), (NULL);
INSERT INTO t1(c0) VALUES(INTERVAL '1427502482' SECOND), (INTERVAL '-1598854979' SECOND); 
DELETE FROM t1 WHERE CASE r'FALSE' WHEN r'879628043' THEN TRUE ELSE ((((t1.c0)>=(t1.c0)))OR(((TRUE)OR(TRUE)))) END;
INSERT INTO t1(c0) VALUES(INTERVAL '236620278' SECOND);
INSERT INTO t1(c0) VALUES(INTERVAL '1448897349' SECOND);
DELETE FROM t1 WHERE CAST(TRUE AS BOOLEAN);
	-- all rows deleted
DROP TABLE sys.t1;

! Generated from ../../../git/cloog/test/yosrf.cloog by CLooG 0.14.0-91-g5d3da4b gmp bits in 0.01s.
IF (n >= 2) THEN
  DO j=2, n
    S1(i = 1)
  END DO
END IF
DO proc=2, n-1
  DO i=1, proc-1
    DO j=i+1, n
      S2(k = proc)
    END DO
  END DO
  DO j=proc+1, n
    S1(i = proc)
  END DO
END DO
IF (n >= 2) THEN
  DO i=1, n-1
    DO j=i+1, n
      S2(k = n)
    END DO
  END DO
END IF

/* Generated from ../../../git/cloog/test/test.cloog by CLooG 0.14.0-91-g5d3da4b gmp bits in 0.01s. */
for (i=1;i<=2;i++) {
  for (j=1;j<=M;j++) {
    S1 ;
  }
}
for (i=3;i<=M-1;i++) {
  for (j=1;j<=i-1;j++) {
    S1 ;
  }
  S1(j = i) ;
  S2(j = i) ;
  for (j=i+1;j<=M;j++) {
    S1 ;
  }
}
for (j=1;j<=M-1;j++) {
  S1(i = M) ;
}
S1(i = M,j = M) ;
S2(i = M,j = M) ;
for (i=M+1;i<=N;i++) {
  for (j=1;j<=M;j++) {
    S1 ;
  }
  S2(j = i) ;
}

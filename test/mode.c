/* Generated from ../../../git/cloog/test/mode.cloog by CLooG 0.14.0-91-g5d3da4b gmp bits in 0.01s. */
for (i=0;i<=min(M,N-1);i++) {
  for (j=0;j<=i;j++) {
    S1 ;
    S2 ;
  }
  for (j=i+1;j<=N;j++) {
    S2 ;
  }
}
if ((M >= N) && (N >= 0)) {
  for (j=0;j<=N;j++) {
    S1(i = N) ;
    S2(i = N) ;
  }
}
if (N >= 0) {
  for (i=N+1;i<=M;i++) {
    for (j=0;j<=N;j++) {
      S1 ;
      S2 ;
    }
    for (j=N+1;j<=i;j++) {
      S1 ;
    }
  }
}
if (N <= -1) {
  for (i=0;i<=M;i++) {
    for (j=0;j<=i;j++) {
      S1 ;
    }
  }
}

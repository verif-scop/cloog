/* Generated from ../../../git/cloog/test/logopar.cloog by CLooG 0.14.0-91-g5d3da4b gmp bits in 0.02s. */
for (j=0;j<=m;j++) {
  S1(i = 1) ;
}
if (m >= n+1) {
  for (i=2;i<=n;i++) {
    for (j=0;j<=i-2;j++) {
      S2 ;
    }
    for (j=i-1;j<=n;j++) {
      S1 ;
      S2 ;
    }
    for (j=n+1;j<=m;j++) {
      S1 ;
    }
  }
}
if (m == n) {
  for (i=2;i<=n;i++) {
    for (j=0;j<=i-2;j++) {
      S2 ;
    }
    for (j=i-1;j<=n;j++) {
      S1 ;
      S2 ;
    }
  }
}
for (i=n+1;i<=m+1;i++) {
  for (j=i-1;j<=m;j++) {
    S1 ;
  }
}

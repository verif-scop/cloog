/* Generated from ../../../git/cloog/test/./reservoir/pingali1.cloog by CLooG 0.14.0-91-g5d3da4b gmp bits in 0.02s. */
if (N >= 2) {
  for (c2=1;c2<=M;c2++) {
    for (c4=1;c4<=2;c4++) {
      if ((c4+1)%2 == 0) {
        j = (c4+1)/2 ;
        S2(i = c2) ;
      }
    }
    for (c4=3;c4<=2*N-1;c4++) {
      for (c6=max(1,c4-N);c6<=floord(c4-1,2);c6++) {
        j = c4-c6 ;
        S1(i = c2,k = c6) ;
      }
      if ((c4+1)%2 == 0) {
        j = (c4+1)/2 ;
        S2(i = c2) ;
      }
    }
  }
}
if (N == 1) {
  for (c2=1;c2<=M;c2++) {
    S2(i = c2,j = 1) ;
  }
}

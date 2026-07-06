/*    0.0 */       exec
/*    3   */          vfetch_full r0.xy01, r0.x, vf0, DataFormat=FMT_32_32_FLOAT, Stride=2, Signed=true, NumFormat=integer
/*    0.1 */       alloc position
/*    1.0 */       exec
/*    4   */          max oPos, r0, r0
/*    1.1 */       alloc interpolators
/*    2.0 */       exece
/*    2.1 */       cnop

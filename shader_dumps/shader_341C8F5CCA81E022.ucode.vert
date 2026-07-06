/*    0.0 */       exec
/*    3   */          vfetch_full r2.xyz1, r0.x, vf0, DataFormat=FMT_32_32_32_32_FLOAT, Stride=7, Signed=true, NumFormat=integer, PrefetchCount=7
/*    4   */          vfetch_mini r1.xy0_, Offset=5, DataFormat=FMT_32_32_FLOAT, Signed=true, NumFormat=integer
/*    5   */          vfetch_mini r0.zyxw, Offset=4, DataFormat=FMT_8_8_8_8
/*    0.1 */       alloc position
/*    1.0 */       exec
/*    6   */          max oPos.__zw, r2.zzzw, r2.zzzw
/*    7   */          add r2.xy__, r2.xyyy, c255.yyyy
/*    8   */          mad oPos.x___, r2.xxxx, c0.xxxx, c255.zzzz
/*    9   */          mad oPos._y__, -r2.yyyy, c1.xxxx, c255.xxxx
/*    1.1 */       alloc interpolators
/*    2.0 */       exece
/*   10   */          max r0._, c0, c0
/*   11   */          max r0._, c0, c0
/*   12   */          max r0._, c0, c0
/*   13   */          max r0._, c0, c0
/*   14   */          min o0, r0, c255.xxxx
/*    2.1 */       cnop

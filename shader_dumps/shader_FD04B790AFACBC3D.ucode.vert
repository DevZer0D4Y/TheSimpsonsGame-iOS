/*    0.0 */       exec
/*    3   */          vfetch_full r1.xyz1, r0.x, vf0, DataFormat=FMT_32_32_32_32_FLOAT, Stride=7, Signed=true, NumFormat=integer, PrefetchCount=5
/*    4   */          vfetch_mini r0.zyxw, Offset=4, DataFormat=FMT_8_8_8_8
/*    0.1 */       alloc position
/*    1.0 */       exec
/*    5   */          max oPos.__zw, r1.zzzw, r1.zzzw
/*    6   */          add r1.xy__, r1.xyyy, c255.yyyy
/*    7   */          mad oPos.x___, r1.xxxx, c0.xxxx, c255.zzzz
/*    8   */          mad oPos._y__, -r1.yyyy, c1.xxxx, c255.xxxx
/*    1.1 */       alloc interpolators
/*    2.0 */       exece
/*    9   */          max r0._, c0, c0
/*   10   */          max r0._, c0, c0
/*   11   */          max r0._, c0, c0
/*   12   */          max r0._, c0, c0
/*   13   */          min o0, r0, c255.xxxx
/*    2.1 */       cnop

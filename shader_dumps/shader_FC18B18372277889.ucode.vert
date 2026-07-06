/*    0.0 */       exec
/*    3   */          vfetch_full r2.xyz1, r0.x, vf0, DataFormat=FMT_32_32_32_FLOAT, Stride=7, Signed=true, NumFormat=integer, PrefetchCount=5
/*    4   */          vfetch_mini r0.xy__, Offset=3, DataFormat=FMT_32_32_FLOAT, Signed=true, NumFormat=integer
/*    0.1 */       alloc position
/*    1.0 */       exec
/*    5   */          dp4 r1.__z_, c3.zxyw, r2.zxyw
/*    6   */          dp4 r1._y__, c1.zxyw, r2.zxyw
/*    7   */          dp4 r1.x___, c0.zxyw, r2.zxyw
/*    8   */          max oPos, r1.xyzz, r1.xyzz
/*    1.1 */       alloc interpolators
/*    2.0 */       exece
/*    9   */          mad r2, c46, c22.xxxx, r0.xyxy
/*   10   */          max o1.xy__, r2.xyyy, r2.xyyy
/*   11   */          max o2.xy__, r2.zwww, r2.zwww
/*   12   */          max o3, r1.xyzz, r1.xyzz
/*   13   */          mad o0.xy__, c47.xyyy, c22.xxxx, r0.xyyy
/*    2.1 */       cnop

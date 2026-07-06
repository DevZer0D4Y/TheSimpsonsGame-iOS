/*    0.0 */       exec
/*    3   */          vfetch_full r1.xyz1, r0.x, vf0, DataFormat=FMT_32_32_32_FLOAT, Stride=9, Signed=true, NumFormat=integer, PrefetchCount=7
/*    4   */          vfetch_mini r0._xyz, Offset=3, DataFormat=FMT_2_10_10_10, Signed=true
/*    5   */          vfetch_mini r2.zyxw, Offset=4, DataFormat=FMT_8_8_8_8
/*    6   */          vfetch_mini r3.xy__, Offset=5, DataFormat=FMT_32_32_FLOAT, Signed=true, NumFormat=integer
/*    0.1 */       alloc position
/*    1.0 */       exec
/*    7   */          dp4 oPos.x___, c0.zxyw, r1.zxyw
/*    8   */          dp4 oPos._y__, c1.zxyw, r1.zxyw
/*    9   */          dp4 oPos.__z_, c2.zxyw, r1.zxyw
/*   10   */          dp4 oPos.___w, c3.zxyw, r1.zxyw
/*    1.1 */       alloc interpolators
/*    2.0 */       exec
/*   11   */          max o0.xy__, r3.xyyy, r3.xyyy
/*   12   */          max o3, r2, r2
/*   13   */          dp4 o1.x___, c12.zxyw, r1.zxyw
/*   14   */          dp4 o1._y__, c13.zxyw, r1.zxyw
/*   15   */          dp4 o1.__z_, c14.zxyw, r1.zxyw
/*   16   */          dp4 o1.___w, c15.zxyw, r1.zxyw
/*    2.1 */       exece
/*   17   */          dp3 o2.x___, r0.wyzz, c12.zxyy
/*   18   */          dp3 o2._y__, r0.wyzz, c13.zxyy
/*   19   */          dp3 o2.__z_, r0.wyzz, c14.zxyy

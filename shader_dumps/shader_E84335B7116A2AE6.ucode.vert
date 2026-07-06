/*    0.0 */       exec
/*    4   */          vfetch_full r4.xyz1, r0.x, vf0, DataFormat=FMT_32_32_32_FLOAT, Stride=9, Signed=true, NumFormat=integer, PrefetchCount=7
/*    5   */          vfetch_mini r1.xyz_, Offset=3, DataFormat=FMT_2_10_10_10, Signed=true
/*    6   */          vfetch_mini r2.zyxw, Offset=4, DataFormat=FMT_8_8_8_8
/*    7   */          vfetch_mini r3.__xy, Offset=5, DataFormat=FMT_32_32_FLOAT, Signed=true, NumFormat=integer
/*    8   */          vfetch_full r3.xy__, r0.x, vf0, Offset=7, DataFormat=FMT_32_32_FLOAT, Stride=9, Signed=true, NumFormat=integer
/*    0.1 */       alloc position
/*    1.0 */       exec
/*    9   */          dp4 oPos.x___, c0.zxyw, r4.zxyw
/*   10   */          dp4 oPos._y__, c1.zxyw, r4.zxyw
/*   11   */          dp4 oPos.__z_, c2.zxyw, r4.zxyw
/*   12   */          dp4 oPos.___w, c3.zxyw, r4.zxyw
/*    1.1 */       alloc interpolators
/*    2.0 */       exec
/*   13   */          dp4 r0.x___, c15.zxyw, r4.zxyw
/*   14   */          dp4 r0._y__, c14.zxyw, r4.zxyw
/*   15   */          dp4 r0.__z_, c12.zxyw, r4.zxyw
/*   16   */          dp4 r0.___w, c13.zxyw, r4.zxyw
/*   17   */          max o0.xy__, r3.zwww, r3.zwww
/*   18   */          max o1.xy__, r3.xyyy, r3.xyyy
/*    2.1 */       exec
/*   19   */          max o5, r2, r2
/*   20   */          dp3 o4.x___, r1.zxyy, c12.zxyy
/*   21   */          dp3 o4._y__, r1.zxyy, c13.zxyy
/*   22   */          dp3 o4.__z_, r1.zxyy, c14.zxyy
/*   23   */          dp4 o2.x___, r0, c26.wzxy
/*   24   */          dp4 o2._y__, r0, c27.wzxy
/*    3.0 */       exece
/*   25   */          dp4 o2.__z_, r0, c28.wzxy
/*   26   */          dp4 o2.___w, r0, c29.wzxy
/*   27   */          dp4 o3.x___, r0, c22.wzxy
/*   28   */          dp4 o3._y__, r0, c23.wzxy
/*   29   */          dp4 o3.__z_, r0, c24.wzxy
/*   30   */          dp4 o3.___w, r0, c25.wzxy
/*    3.1 */       cnop

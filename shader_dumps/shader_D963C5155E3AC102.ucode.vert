/*    0.0 */       exec
/*    5   */          vfetch_full r1.xyz1, r0.x, vf0, DataFormat=FMT_32_32_32_FLOAT, Stride=10, Signed=true, NumFormat=integer, PrefetchCount=8
/*    6   */          vfetch_mini r5.xyz_, Offset=3, DataFormat=FMT_2_10_10_10, Signed=true
/*    7   */          vfetch_mini r2.xyz_, Offset=4, DataFormat=FMT_2_10_10_10, Signed=true
/*    8   */          vfetch_mini r3.zyxw, Offset=5, DataFormat=FMT_8_8_8_8
/*    9   */          vfetch_mini r4.xy__, Offset=6, DataFormat=FMT_32_32_FLOAT, Signed=true, NumFormat=integer
/*   10   */          vfetch_full r4.__xy, r0.x, vf0, Offset=8, DataFormat=FMT_32_32_FLOAT, Stride=10, Signed=true, NumFormat=integer
/*    0.1 */       alloc position
/*    1.0 */       exec
/*   11   */          dp4 oPos.x___, c0.zxyw, r1.zxyw
/*   12   */          dp4 oPos._y__, c1.zxyw, r1.zxyw
/*   13   */          dp4 oPos.__z_, c2.zxyw, r1.zxyw
/*   14   */          dp4 oPos.___w, c3.zxyw, r1.zxyw
/*    1.1 */       alloc interpolators
/*    2.0 */       exec
/*   15   */          dp3 r0._y__, r2.zxyy, c14.zxyy
/*   16   */          dp3 r0.__z_, r2.zxyy, c12.zxyy
/*   17   */          dp3 r0.___w, r2.zxyy, c13.zxyy
/*   18   */          dp4 r2.x___, c15.zxyw, r1.zxyw
/*   19   */          dp4 r2._y__, c14.zxyw, r1.zxyw
/*   20   */          dp4 r2.__z_, c12.zxyw, r1.zxyw
/*    2.1 */       exec
/*   21   */          dp4 r2.___w, c13.zxyw, r1.zxyw
/*   22   */          dp3 r1.x___, r5.zxyy, c13.zxyy
/*   23   */          dp3 r1._y__, r5.zxyy, c12.zxyy
/*   24   */          dp3 r1.__z_, r5.zxyy, c14.zxyy
/*   25   */          max o0, r4, r4
/*   26   */          max o7, r3, r3
/*    3.0 */       exec
/*   27   */          max o4.xyz_, r1.yxzz, r1.yxzz
/*   28   */          max o1, r2.zwyx, r2.zwyx
/*   29   */          dp4 o2.x___, r2, c26.wzxy
/*   30   */          dp4 o2._y__, r2, c27.wzxy
/*   31   */          dp4 o2.__z_, r2, c28.wzxy
/*   32   */          dp4 o2.___w, r2, c29.wzxy
/*    3.1 */       exec
/*   33   */          dp4 o3.x___, r2, c22.wzxy
/*   34   */          dp4 o3._y__, r2, c23.wzxy
/*   35   */          dp4 o3.__z_, r2, c24.wzxy
/*   36   */          dp4 o3.___w, r2, c25.wzxy
/*   37   */          dp3 r0.x___, r0.yzww, r0.yzww
/*   38   */          rsq r0.x___, r_abs[0].x
/*    4.0 */       exec
/*   39   */          mul o5.xyz_, r0.zwyy, r0.xxxx
/*   40   */          mul r2.xyz_, r0.yzww, r0.xxxx
/*   41   */          mul r0.xy__, r2.zyyy, r1.zzzz
/*   42   */          mul r1, r2.xyxz, r1.xxyy
/*   43   */          add r0.__z_, r1.xxxx, -r0.xxxx
/*   44   */          add r0.___w, r0.yyyy, -r1.zzzz
              +       subs r0._y__, r1.wy
/*    4.1 */       exece
/*   45   */          dp3 r0.x___, r0.yzww, r0.yzww
/*   46   */          rsq r0.x___, r_abs[0].x
/*   47   */          mul o6.xyz_, r0.zwyy, r0.xxxx

/*    0.0 */       exec
/*    5   */          vfetch_full r1.xyz1, r0.x, vf0, DataFormat=FMT_32_32_32_FLOAT, Stride=10, Signed=true, NumFormat=integer, PrefetchCount=8
/*    6   */          vfetch_mini r3.xyz_, Offset=3, DataFormat=FMT_2_10_10_10, Signed=true
/*    7   */          vfetch_mini r5.xyz_, Offset=4, DataFormat=FMT_2_10_10_10, Signed=true
/*    8   */          vfetch_mini r4.zyxw, Offset=5, DataFormat=FMT_8_8_8_8
/*    9   */          vfetch_mini r2.xy__, Offset=6, DataFormat=FMT_32_32_FLOAT, Signed=true, NumFormat=integer
/*   10   */          vfetch_full r2.__xy, r0.x, vf0, Offset=8, DataFormat=FMT_32_32_FLOAT, Stride=10, Signed=true, NumFormat=integer
/*    0.1 */       alloc position
/*    1.0 */       exec
/*   11   */          mul r0.x___, c22.xxxx, c254.wwww
/*   12   */          mul r0.xy__, r0.xxxx, c45.ywww
/*   13   */          mul r0.xy__, r0.xyyy, r1.xzzz
/*   14   */          mad r0.xy__, r0.xyyy, c254.zzzz, c254.xxxx
/*   15   */          frc r0.xy__, r0.xyyy
/*   16   */          mad r0._yz_, r0.xxyy, c254.wwww, c255.xxxx
/*    1.1 */       exec
/*   17   */          dp3 r6._y__, r3.zxyy, c12.zxyy
              +       sin r0.x___, r0.y
/*   18   */          sge r0.___w, r_abs[0].xxxx, c254.yyyy
              +       cos r0._y__, r0.z
/*   19   */          dp2add r0.x___, r0.xyyy, c45.xzzz, c254.yyyy
/*   20   */          mad r0.xyz_, r0.xxxx, r3.xyzz, r1.xyzz
/*   21   */          dp4 oPos.x___, c0.zxyw, r0.zxyw
/*   22   */          dp4 oPos._y__, c1.zxyw, r0.zxyw
/*    2.0 */       exec
/*   23   */          dp4 oPos.__z_, c2.zxyw, r0.zxyw
/*   24   */          dp4 oPos.___w, c3.zxyw, r0.zxyw
/*    2.1 */       alloc interpolators
/*    3.0 */       exec
/*   25   */          dp3 r6.x___, r3.zxyy, c13.zxyy
/*   26   */          dp3 r0._y__, r5.zxyy, c14.zxyy
/*   27   */          dp3 r0.__z_, r5.zxyy, c12.zxyy
/*   28   */          dp3 r0.___w, r5.zxyy, c13.zxyy
/*   29   */          dp3 r0.x___, r0.yzww, r0.yzww
/*   30   */          dp3 r6.__z_, r3.zxyy, c14.zxyy
              +       rsq r0.x___, r_abs[0].x
/*    3.1 */       exec
/*   31   */          mul r5.xyz_, r0.zwyy, r0.xxxx
/*   32   */          mul r7, r5.zxzy, r6.xxyy
/*   33   */          mul r3.x_z_, r5.yxxx, r6.zzzz
              +       subs r3._y__, r7.wy
/*   34   */          mul r0, c47, c22.xxxx
              +       maxs r0._, r7.xx
/*   35   */          add r3.___w, r3.zzzz, -r7.zzzz
              +       adds_prev r3.__z_, -r3.x
/*   36   */          dp3 r3.x___, r3.yzww, r3.yzww
              +       maxs r5.___w, r2.zz
/*    4.0 */       exec
/*   37   */          max r5.___w, r5.wwww, r5.wwww
              +       rsq r3.x___, r_abs[3].x
/*   38   */          max o2.xyz_, r6.yxzz, r6.yxzz
/*   39   */          max o3, r5, r5
/*   40   */          max o5, r4, r4
/*   41   */          mul o4.xyz_, r3.zwyy, r3.xxxx
              +       maxs o4.___w, r2.ww
/*   42   */          mad o0, r2.xyxy, c46, r0
/*    4.1 */       exece
/*   43   */          dp4 o1.x___, c12.zxyw, r1.zxyw
/*   44   */          dp4 o1._y__, c13.zxyw, r1.zxyw
/*   45   */          dp4 o1.__z_, c14.zxyw, r1.zxyw
/*   46   */          dp4 o1.___w, c15.zxyw, r1.zxyw
